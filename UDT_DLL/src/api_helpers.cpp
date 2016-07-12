#include "api_helpers.hpp"
#include "parser_context.hpp"
#include "linear_allocator.hpp"
#include "scoped_stack_allocator.hpp"
#include "utils.hpp"
#include "timer.hpp"
#include "analysis_pattern_chat.hpp"
#include "analysis_pattern_frag_run.hpp"
#include "plug_in_pattern_search.hpp"
#include "plug_in_converter_quake_to_udt.hpp"
#include "parser_runner.hpp"
#include "converter_entity_timer_shifter.hpp"
#include "path.hpp"
#include "memory_stream.hpp"
#include "json_export.hpp"
#include "pattern_search_context.hpp"


bool InitContextWithPlugIns(udtParserContext& context, const udtParseArg& info, u32 demoCount, udtParsingJobType::Id jobType, const void* jobSpecificInfo)
{
	if(jobType == udtParsingJobType::General ||
	   jobType == udtParsingJobType::ExportToJSON)
	{
		for(u32 i = 0; i < info.PlugInCount; ++i)
		{
			const u32 plugIn = info.PlugIns[i];
			if(plugIn >= (u32)udtPrivateParserPlugIn::Count)
			{
				return false;
			}
		}

		return context.Init(demoCount, info.PlugIns, info.PlugInCount);
	}

	if(jobType == udtParsingJobType::Conversion)
	{
		if(jobSpecificInfo == NULL)
		{
			return false;
		}

		return context.Init(demoCount, NULL, 0);
	}

	if(jobType == udtParsingJobType::TimeShift)
	{
		if(jobSpecificInfo == NULL)
		{
			return false;
		}

		const u32 plugInId = udtPrivateParserPlugIn::ConvertToUDT;
		if(!context.Init(demoCount, &plugInId, 1))
		{
			return false;
		}

		udtBaseParserPlugIn* plugInBase = NULL;
		context.GetPlugInById(plugInBase, plugInId);
		if(plugInBase == NULL)
		{
			return false;
		}
		
		return true;
	}

	if(jobType == udtParsingJobType::CutByPattern ||
	   jobType == udtParsingJobType::FindPatterns)
	{
		if(jobSpecificInfo == NULL)
		{
			return false;
		}

		const u32 plugInId = udtPrivateParserPlugIn::FindPatterns;
		if(!context.Init(demoCount, &plugInId, 1))
		{
			return false;
		}

		udtBaseParserPlugIn* plugInBase = NULL;
		context.GetPlugInById(plugInBase, plugInId);
		if(plugInBase == NULL)
		{
			return false;
		}

		const udtPatternSearchArg* patternInfo = NULL;
		if(jobType == udtParsingJobType::FindPatterns)
		{
			const udtPatternSearchContext* const searchContext = (const udtPatternSearchContext*)jobSpecificInfo;
			patternInfo = searchContext->PatternInfo;
		}
		else
		{
			patternInfo = (const udtPatternSearchArg*)jobSpecificInfo;
		}

		udtPatternSearchPlugIn& plugIn = *(udtPatternSearchPlugIn*)plugInBase;
		plugIn.SetPatternInfo(*patternInfo);
		for(u32 i = 0; i < patternInfo->PatternCount; ++i)
		{
			const udtPatternInfo pi = patternInfo->Patterns[i];
			plugIn.CreateAndAddAnalyzer((udtPatternType::Id)pi.Type, pi.TypeSpecificInfo);
		}

		plugIn.InitAnalyzerAllocators(demoCount);

		return true;
	}

	return false;
}

static bool ParseDemoFile(udtProtocol::Id protocol, udtParserContext* context, const udtParseArg* info, const char* demoFilePath, bool clearPlugInData)
{
	context->ResetForNextDemo(!clearPlugInData);
	if(!context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext))
	{
		return false;
	}

	UDT_INIT_DEMO_FILE_READER(file, demoFilePath, context);

	if(!context->Parser.Init(&context->Context, protocol, protocol))
	{
		return false;
	}

	context->Parser.SetFilePath(demoFilePath);
	if(!RunParser(context->Parser, file, info->CancelOperation))
	{
		return false;
	}

	return true;
}

static bool ParseDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath, bool clearPlugInData)
{
	const udtProtocol::Id protocol = (udtProtocol::Id)udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	return ParseDemoFile(protocol, context, info, demoFilePath, clearPlugInData);
}

static bool CutByPattern(udtParserContext* context, const udtParseArg* info, const char* demoFilePath)
{
	const udtProtocol::Id protocol = (udtProtocol::Id)udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	if(!ParseDemoFile(protocol, context, info, demoFilePath, false))
	{
		return false;
	}

	udtBaseParserPlugIn* plugInBase = NULL;
	context->GetPlugInById(plugInBase, udtPrivateParserPlugIn::FindPatterns);
	udtPatternSearchPlugIn& plugIn = *(udtPatternSearchPlugIn*)plugInBase;

	if(plugIn.CutSections.IsEmpty())
	{
		return true;
	}

	context->ResetForNextDemo(true);
	if(!context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext))
	{
		return false;
	}

	const s32 gsIndex = plugIn.CutSections[0].GameStateIndex;
	const u32 fileOffset = context->Parser._inGameStateFileOffsets[gsIndex];
	UDT_INIT_DEMO_FILE_READER_AT(file, demoFilePath, context, fileOffset);

	// Save the cut sections in a temporary array.
	udtVMArray<udtCutSection> sections("CutByPattern::SectionsArray");
	for(u32 i = 0, count = plugIn.CutSections.GetSize(); i < count; ++i)
	{
		sections.Add(plugIn.CutSections[i]);
	}
	
	// This will clear the plug-in's section list.
	if(!context->Parser.Init(&context->Context, protocol, protocol, gsIndex, false))
	{
		return false;
	}

	context->Parser.SetFilePath(demoFilePath);

	CallbackCutDemoFileStreamCreationInfo cutCbInfo;
	cutCbInfo.OutputFolderPath = info->OutputFolderPath;

	for(u32 i = 0, count = sections.GetSize(); i < count; ++i)
	{
		const udtCutSection& section = sections[i];
		context->Parser.AddCut(
			section.GameStateIndex, section.StartTimeMs, section.EndTimeMs, 
			&CallbackCutDemoFileNameCreation, section.VeryShortDesc, &cutCbInfo);
	}

	context->Context.LogInfo("Processing demo for applying cut(s): %s", demoFilePath);

	context->Context.SetCallbacks(info->MessageCb, NULL, NULL);
	const bool result = RunParser(context->Parser, file, info->CancelOperation);
	context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext);

	return result;
}

static bool FindPatterns(udtParserContext* context, u32 demoIndex, const udtParseArg* info, const char* demoFilePath, udtPatternSearchContext* searchContext)
{
	const udtProtocol::Id protocol = (udtProtocol::Id)udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	if(!ParseDemoFile(protocol, context, info, demoFilePath, false))
	{
		return false;
	}

	udtBaseParserPlugIn* plugInBase = NULL;
	context->GetPlugInById(plugInBase, udtPrivateParserPlugIn::FindPatterns);
	udtPatternSearchPlugIn& plugIn = *(udtPatternSearchPlugIn*)plugInBase;

	if(plugIn.CutSections.IsEmpty())
	{
		return true;
	}

	udtVMArray<udtPatternMatch>& cuts = searchContext->Matches;
	for(u32 i = 0, count = plugIn.CutSections.GetSize(); i < count; ++i)
	{
		const udtCutSection& cut = plugIn.CutSections[i];
		udtPatternMatch match;
		match.DemoInputIndex = demoIndex;
		match.GameStateIndex = cut.GameStateIndex;
		match.StartTimeMs = cut.StartTimeMs;
		match.EndTimeMs = cut.EndTimeMs;
		match.Patterns = cut.PatternTypes;
		cuts.Add(match);
	}

	return true;
}

static bool ConvertDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath, const udtProtocolConversionArg* conversionInfo)
{
	const udtProtocol::Id protocol = (udtProtocol::Id)udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	if((u32)protocol == conversionInfo->OutputProtocol)
	{
		// Nothing to convert!
		return true;
	}

	context->ResetForNextDemo(false);
	if(!context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext))
	{
		return false;
	}

	UDT_INIT_DEMO_FILE_READER(file, demoFilePath, context);

	if(!context->Parser.Init(&context->Context, protocol, (udtProtocol::Id)conversionInfo->OutputProtocol))
	{
		return false;
	}

	context->Parser._protocolConverter->ConversionInfo = conversionInfo;

	context->Parser.SetFilePath(demoFilePath);

	CallbackCutDemoFileStreamCreationInfo cutCbInfo;
	cutCbInfo.OutputFolderPath = info->OutputFolderPath;
	context->Parser.AddCut(0, UDT_S32_MIN, UDT_S32_MAX, &CallbackConvertedDemoFileNameCreation, "", &cutCbInfo);
	
	if(!RunParser(context->Parser, file, info->CancelOperation))
	{
		return false;
	}

	return true;
}

static void CreateTimeShiftDemoName(udtString& outputFilePath, udtVMLinearAllocator& allocator, const udtString& inputFilePath, const char* outputFolderPath, const udtTimeShiftArg* timeShiftArg, udtProtocol::Id protocol)
{
	udtString inputFileName;
	if(udtString::IsNullOrEmpty(inputFilePath) ||
	   !udtPath::GetFileNameWithoutExtension(inputFileName, allocator, inputFilePath))
	{
		inputFileName = udtString::NewConstRef("NEW_UDT_DEMO");
	}

	udtString outputFilePathStart; // The full output path without the extension.
	if(outputFolderPath != NULL)
	{
		udtPath::Combine(outputFilePathStart, allocator, udtString::NewConstRef(outputFolderPath), inputFileName);
	}
	else
	{
		udtString inputFolderPath;
		udtPath::GetFolderPath(inputFolderPath, allocator, inputFilePath);
		udtPath::Combine(outputFilePathStart, allocator, inputFolderPath, inputFileName);
	}

	char snapshotCount[16];
	sprintf(snapshotCount, "%d", (int)timeShiftArg->SnapshotCount);

	const udtString shifted = udtString::NewConstRef("_shifted_");
	const udtString snapCount = udtString::NewConstRef(snapshotCount);
	const udtString snaps = udtString::NewConstRef(timeShiftArg->SnapshotCount > 1 ? "_snaps" : "_snap");
	const udtString proto = udtString::NewConstRef(udtGetFileExtensionByProtocol((u32)protocol));
	const udtString* outputFilePathParts[] =
	{
		&outputFilePathStart,
		&shifted,
		&snapCount,
		&snaps,
		&proto
	};

	outputFilePath = udtString::NewFromConcatenatingMultiple(allocator, outputFilePathParts, (u32)UDT_COUNT_OF(outputFilePathParts));
}

static bool TimeShiftDemo(udtParserContext* context, const udtParseArg* info, const char* demoFilePath, const udtTimeShiftArg* timeShiftArg)
{
	const udtProtocol::Id protocol = (udtProtocol::Id)udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	udtBaseParserPlugIn* plugInBase = NULL;
	context->GetPlugInById(plugInBase, (u32)udtPrivateParserPlugIn::ConvertToUDT);
	if(plugInBase == NULL)
	{
		return false;
	}

	UDT_INIT_DEMO_FILE_READER(input, demoFilePath, context);

	context->ModifierContext.ResetForNextDemo();

	udtString outputFilePath;
	CreateTimeShiftDemoName(outputFilePath, context->ModifierContext.TempAllocator, udtString::NewConstRef(demoFilePath), info->OutputFolderPath, timeShiftArg, protocol);

	udtFileStream output;
	if(!output.Open(outputFilePath.GetPtr(), udtFileOpenMode::Write))
	{
		return false;
	}

	context->ResetForNextDemo(true);
	if(!context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext))
	{
		return false;
	}

	if(!context->Parser.Init(&context->Context, protocol, protocol))
	{
		return false;
	}
	context->Parser.SetFilePath(demoFilePath);

	udtParserRunner runner;
	if(!runner.Init(context->Parser, input, info->CancelOperation))
	{
		return false;
	}

	udtdConverter& converterToQuake = context->ModifierContext.Converter;
	udtdEntityTimeShifterPlugIn& timeShifter = context->ModifierContext.TimeShifterPlugIn;
	udtVMMemoryStream& tempWrite = context->ModifierContext.WriteStream;

	timeShifter.ResetForNextDemo(*timeShiftArg);
	converterToQuake.ResetForNextDemo(input, NULL, protocol);
	converterToQuake.ClearPlugIns();
	converterToQuake.AddPlugIn(&timeShifter);

	udtdMessageType::Id messageType = udtdMessageType::EndOfFile;
	udtParserPlugInQuakeToUDT& converterToUDT = *(udtParserPlugInQuakeToUDT*)plugInBase;

	if(!converterToUDT.ResetForNextDemo(protocol))
	{
		return false;
	}

	udtReadOnlyMemoryStream tempRead;
	converterToUDT.SetOutputStream(&tempWrite);
	converterToQuake.SetStreams(tempRead, &output);

	context->Context.LogInfo("Writing time-shifted demo: %s", outputFilePath.GetPtr());

	for(;;)
	{
		if(!runner.ParseNextMessage())
		{
			break;
		}

		if(tempWrite.Length() == 0)
		{
			continue;
		}

		if(!tempRead.Open(tempWrite.GetBuffer(), (u32)tempWrite.Length()))
		{
			continue;
		}

		// udtParserRunner::ParseNextMessage() may read more than just a snapshot.
		// It can also read a server command bundled in the same message.
		while(converterToQuake.ProcessNextMessage(messageType))
		{
		}

		tempWrite.Clear();
	}

	runner.FinishParsing();

	return runner.WasSuccess();
}

static void CreateJSONFilePath(udtString& outputFilePath, udtVMLinearAllocator& allocator, const udtString& inputFilePath, const char* outputFolderPath)
{
	udtString inputFileName;
	if(udtString::IsNullOrEmpty(inputFilePath) ||
	   !udtPath::GetFileNameWithoutExtension(inputFileName, allocator, inputFilePath))
	{
		inputFileName = udtString::NewConstRef("UNKNOWN_UDT_DEMO");
	}

	udtString outputFilePathStart; // The full output path without the extension.
	if(outputFolderPath != NULL)
	{
		udtPath::Combine(outputFilePathStart, allocator, udtString::NewConstRef(outputFolderPath), inputFileName);
	}
	else
	{
		udtPath::GetFilePathWithoutExtension(outputFilePathStart, allocator, inputFilePath);
	}

	outputFilePath = udtString::NewFromConcatenating(allocator, outputFilePathStart, udtString::NewConstRef(".json"));
}

static bool ExportToJSON(udtParserContext* context, u32 demoIndex, const udtParseArg* info, const char* demoFilePath, const udtJSONArg* jsonInfo)
{
	if(!ParseDemoFile(context, info, demoFilePath, false))
	{
		return false;
	}

	udtVMLinearAllocator& tempAllocator = context->PlugInTempAllocator;
	tempAllocator.Clear();

	udtVMScopedStackAllocator allocatorScope(tempAllocator);

	const char* outputFilePath = NULL;
	if(jsonInfo->ConsoleOutput == 0)
	{
		udtString jsonFilePath;
		CreateJSONFilePath(jsonFilePath, tempAllocator, udtString::NewConstRef(demoFilePath), info->OutputFolderPath);
		outputFilePath = jsonFilePath.GetPtr();
	}
	
	context->UpdatePlugInBufferStructs();
	if(!ExportPlugInsDataToJSON(context, demoIndex, outputFilePath))
	{
		return false;
	}

	return true;
}

bool ProcessSingleDemoFile(udtParsingJobType::Id jobType, udtParserContext* context, u32 contextDemoIndex, u32 inputDemoIndex, const udtParseArg* info, const char* demoFilePath, const void* jobSpecificInfo)
{
	switch(jobType)
	{
		case udtParsingJobType::General:
			return ParseDemoFile(context, info, demoFilePath, false);

		case udtParsingJobType::CutByPattern:
			return CutByPattern(context, info, demoFilePath);

		case udtParsingJobType::Conversion:
			return ConvertDemoFile(context, info, demoFilePath, (const udtProtocolConversionArg*)jobSpecificInfo);

		case udtParsingJobType::TimeShift:
			return TimeShiftDemo(context, info, demoFilePath, (const udtTimeShiftArg*)jobSpecificInfo);

		case udtParsingJobType::ExportToJSON:
			return ExportToJSON(context, contextDemoIndex, info, demoFilePath, (const udtJSONArg*)jobSpecificInfo);

		case udtParsingJobType::FindPatterns:
			return FindPatterns(context, inputDemoIndex, info, demoFilePath, (udtPatternSearchContext*)jobSpecificInfo);

		default:
			return false;
	}
}

void SingleThreadProgressCallback(f32 jobProgress, void* userData)
{
	SingleThreadProgressContext* const context = (SingleThreadProgressContext*)userData;
	if(context == NULL || context->Timer == NULL || context->UserCallback == NULL)
	{
		return;
	}

	if(context->Timer->GetElapsedMs() < u64(context->MinProgressTimeMs))
	{
		return;
	}

	context->Timer->Restart();

	const u64 jobProcessed = (u64)((f64)context->CurrentJobByteCount * (f64)jobProgress);
	const u64 totalProcessed = context->ProcessedByteCount + jobProcessed;
	const f32 realProgress = udt_clamp((f32)totalProcessed / (f32)context->TotalByteCount, 0.0f, 1.0f);

	(*context->UserCallback)(realProgress, context->UserData);
}

s32 udtParseMultipleDemosSingleThread(udtParsingJobType::Id jobType, udtParserContext* context, const udtParseArg* info, const udtMultiParseArg* extraInfo, const void* jobSpecificInfo)
{
	udtTimer jobTimer;
	jobTimer.Start();
	if(info->PerformanceStats != NULL)
	{
		PerfStatsInit(info->PerformanceStats);
	}

	bool customContext = false;
	if(context == NULL)
	{
		context = udtCreateContext();
		if(context == NULL)
		{
			return (s32)udtErrorCode::OperationFailed;
		}
		customContext = true;
	}

	if(!InitContextWithPlugIns(*context, *info, extraInfo->FileCount, jobType, jobSpecificInfo))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	udtTimer progressTimer;
	progressTimer.Start();

	udtVMArray<u64> fileSizes("ParseMultipleDemosSingleThread::FileSizesArray");
	fileSizes.Resize(extraInfo->FileCount);

	u64 totalByteCount = 0;
	for(u32 i = 0; i < extraInfo->FileCount; ++i)
	{
		const u64 byteCount = udtFileStream::GetFileLength(extraInfo->FilePaths[i]);
		fileSizes[i] = byteCount;
		totalByteCount += byteCount;
	}

	context->InputIndices.Resize(extraInfo->FileCount);
	for(u32 i = 0; i < extraInfo->FileCount; ++i)
	{
		context->InputIndices[i] = i;
		extraInfo->OutputErrorCodes[i] = (s32)udtErrorCode::Unprocessed;
	}

	SingleThreadProgressContext progressContext;
	progressContext.Timer = &progressTimer;
	progressContext.UserCallback = info->ProgressCb;
	progressContext.UserData = info->ProgressContext;
	progressContext.CurrentJobByteCount = 0;
	progressContext.ProcessedByteCount = 0;
	progressContext.TotalByteCount = totalByteCount;
	progressContext.MinProgressTimeMs = info->MinProgressTimeMs;

	udtParseArg newInfo = *info;
	newInfo.ProgressCb = &SingleThreadProgressCallback;
	newInfo.ProgressContext = &progressContext;

	u64 actualProcessedByteCount = 0;
	for(u32 i = 0; i < extraInfo->FileCount; ++i)
	{
		if(info->CancelOperation != NULL && *info->CancelOperation != 0)
		{
			break;
		}

		const u64 jobByteCount = fileSizes[i];
		progressContext.CurrentJobByteCount = jobByteCount;

		const bool success = ProcessSingleDemoFile(jobType, context, i, i, &newInfo, extraInfo->FilePaths[i], jobSpecificInfo);
		extraInfo->OutputErrorCodes[i] = GetErrorCode(success, info->CancelOperation);

		progressContext.ProcessedByteCount += jobByteCount;
		if(success)
		{
			actualProcessedByteCount += jobByteCount;
		}
	}

	if(!customContext)
	{
		context->UpdatePlugInBufferStructs();
	}

	if(info->PerformanceStats != NULL)
	{
		PerfStatsAddCurrentThread(info->PerformanceStats, actualProcessedByteCount);
		PerfStatsFinalize(info->PerformanceStats, 1, jobTimer.GetElapsedUs());
	}

#if defined(UDT_DEBUG) && defined(UDT_LOG_ALLOCATOR_DEBUG_STATS)
	context->Parser._tempAllocator.Clear();
	LogLinearAllocatorDebugStats(context->Context, context->Parser._tempAllocator);
#endif

	if(customContext)
	{
		udtDestroyContext(context);
	}

	return GetErrorCode(true, info->CancelOperation);
}

static void CreateMergedDemoName(udtString& outputFilePath, udtVMLinearAllocator& allocator, const udtString& inputFilePath, const char* outputFolderPath, udtProtocol::Id protocol)
{
	udtString inputFileName;
	if(udtString::IsNullOrEmpty(inputFilePath) ||
	   !udtPath::GetFileNameWithoutExtension(inputFileName, allocator, inputFilePath))
	{
		inputFileName = udtString::NewConstRef("NEW_UDT_DEMO");
	}

	udtString outputFilePathStart; // The full output path without the extension.
	if(outputFolderPath != NULL)
	{
		udtPath::Combine(outputFilePathStart, allocator, udtString::NewConstRef(outputFolderPath), inputFileName);
	}
	else
	{
		udtString inputFolderPath;
		udtPath::GetFolderPath(inputFolderPath, allocator, inputFilePath);
		udtPath::Combine(outputFilePathStart, allocator, inputFolderPath, inputFileName);
	}

	const udtString merged = udtString::NewConstRef("_merged");
	const udtString proto = udtString::NewConstRef(udtGetFileExtensionByProtocol((u32)protocol));
	const udtString* outputFilePathParts[] =
	{
		&outputFilePathStart,
		&merged,
		&proto
	};

	outputFilePath = udtString::NewFromConcatenatingMultiple(allocator, outputFilePathParts, (u32)UDT_COUNT_OF(outputFilePathParts));
}

struct DemoMerger
{
	struct DemoData
	{
		udtFileStream Input;
		udtParserRunner Runner;
		udtVMMemoryStream WriteBuffer;
		udtReadOnlyMemoryStream ReadBuffer;
		udtdConverter ConverterToQuake;
		udtParserContext* Context;
		udtParserPlugInQuakeToUDT* ConverterToUDT;
	};

	DemoMerger()
	{
		for(u32 i = 0; i < UDT_MAX_MERGE_DEMO_COUNT; ++i)
		{
			_demos[i].Context = NULL;
			_demos[i].ConverterToUDT = NULL;
		}

		_fileCount = 0;
	}

	~DemoMerger()
	{
		for(u32 i = 0; i < UDT_MAX_MERGE_DEMO_COUNT; ++i)
		{
			if(_demos[i].Context != NULL)
			{
				udtDestroyContext(_demos[i].Context);
			}
		}
	}

	bool MergeDemos(const udtParseArg* info, const char** filePaths, u32 fileCount, udtProtocol::Id protocol)
	{
		_fileCount = fileCount;

		udtVMLinearAllocator tempAllocator("DemoMerger::MergeDemos::Temp");

		udtString outputFilePath;
		CreateMergedDemoName(outputFilePath, tempAllocator, udtString::NewConstRef(filePaths[0]), info->OutputFolderPath, protocol);

		udtFileStream output;
		if(!output.Open(outputFilePath.GetPtr(), udtFileOpenMode::Write))
		{
			return false;
		}

		for(u32 i = 0; i < fileCount; ++i)
		{
			DemoData& demo = _demos[i];

			demo.Context = udtCreateContext();
			if(demo.Context == NULL)
			{
				return false;
			}

			const u32 plugInId = udtPrivateParserPlugIn::ConvertToUDT;
			if(!demo.Context->Init(1, &plugInId, 1))
			{
				return false;
			}

			udtBaseParserPlugIn* plugInBase = NULL;
			demo.Context->GetPlugInById(plugInBase, plugInId);
			if(plugInBase == NULL)
			{
				return false;
			}
			demo.ConverterToUDT = (udtParserPlugInQuakeToUDT*)plugInBase;
			if(!demo.ConverterToUDT->ResetForNextDemo(protocol))
			{
				return false;
			}

			if(!demo.Context->Context.SetCallbacks(info->MessageCb, i == 0 ? info->ProgressCb : NULL, info->ProgressContext))
			{
				return false;
			}

			if(!demo.Input.Open(filePaths[i], udtFileOpenMode::Read))
			{
				return false;
			}

			if(!demo.Context->Parser.Init(&demo.Context->Context, protocol, protocol))
			{
				return false;
			}

			demo.Context->Parser.SetFilePath(filePaths[i]);

			if(!demo.Runner.Init(demo.Context->Parser, demo.Input, info->CancelOperation))
			{
				return false;
			}

			demo.ConverterToUDT->SetOutputStream(&demo.WriteBuffer);
			demo.ConverterToQuake.ResetForNextDemo(demo.ReadBuffer, i == 0 ? &output : NULL, protocol);
		}

		DemoData& firstDemo = _demos[0];
		s32 firstTime = UDT_S32_MIN;
		udtdMessageType::Id messageType = udtdMessageType::Invalid;
		udtdConverter::SnapshotInfo snapshotInfo;

		firstDemo.Context->Context.LogInfo("Writing merged demo: %s", outputFilePath.GetPtr());

		for(;;)
		{
			if(!firstDemo.Runner.ParseNextMessage())
			{
				break;
			}

			const u64 writeBufferSize = firstDemo.WriteBuffer.Length();
			if(writeBufferSize == 0)
			{
				continue;
			}

			if(!firstDemo.ReadBuffer.Open(firstDemo.WriteBuffer.GetBuffer(), (u32)firstDemo.WriteBuffer.Length()))
			{
				return false;
			}

			firstDemo.ConverterToQuake.ProcessNextMessageRead(messageType, snapshotInfo);
			firstDemo.WriteBuffer.Clear();

			if(messageType == udtdMessageType::Snapshot)
			{
				firstTime = firstDemo.ConverterToQuake.GetServerTime();
			}
			
			SynchronizeDemos(firstTime);
			MergeDemoEntities();

			if(!firstDemo.ConverterToQuake.ProcessNextMessageWrite(messageType, snapshotInfo))
			{
				break;
			}
		}
		
		return true;
	}

	void SynchronizeDemos(s32 firstTime)
	{
		if(firstTime == UDT_S32_MIN)
		{
			// We haven't read the first snapshot yet.
			return;
		}

		for(u32 i = 1; i < _fileCount; ++i)
		{
			SynchronizeDemo(_demos[i], firstTime);
		}
	}

	void SynchronizeDemo(DemoData& demo, s32 firstTime)
	{
		if(demo.ConverterToQuake.GetServerTimeOld() >= firstTime)
		{
			return;
		}

		udtdMessageType::Id messageType = udtdMessageType::Invalid;
		for(;;)
		{
			if(!demo.Runner.ParseNextMessage())
			{
				break;
			}

			const u64 writeBufferSize = demo.WriteBuffer.Length();
			if(writeBufferSize == 0)
			{
				continue;
			}

			if(!demo.ReadBuffer.Open(demo.WriteBuffer.GetBuffer(), (u32)demo.WriteBuffer.Length()))
			{
				break;
			}
			
			// udtParserRunner::ParseNextMessage() may read more than just a snapshot.
			// It can also read a server command bundled in the same message.
			bool stop = false;
			while(demo.ConverterToQuake.ProcessNextMessage(messageType))
			{
				if(messageType == udtdMessageType::Snapshot && demo.ConverterToQuake.GetServerTimeOld() >= firstTime)
				{
					stop = true;
				}
			}

			demo.WriteBuffer.Clear();
			if(stop)
			{
				break;
			}
		}
	}

	void MergeDemoEntities()
	{
		DemoData& firstDemo = _demos[0];
		const s32 firstTime = firstDemo.ConverterToQuake.GetServerTime();
		for(u32 i = 1; i < _fileCount; ++i)
		{
			DemoData& demo = _demos[i];

			const s32 time = demo.ConverterToQuake.GetServerTimeOld();
			if(time == firstTime)
			{
				firstDemo.ConverterToQuake.MergeEntitiesFrom(demo.ConverterToQuake, 0, 1);
			}
		}
	}

	DemoData _demos[UDT_MAX_MERGE_DEMO_COUNT];
	u32 _fileCount;
};

bool MergeDemosNoInputCheck(const udtParseArg* info, const char** filePaths, u32 fileCount, udtProtocol::Id protocol)
{
	udtVMLinearAllocator allocator("MergeDemosNoInputCheck::Temp");
	udtVMScopedStackAllocator allocatorScope(allocator);
	const uptr mergerOffset = allocatorScope.NewObject<DemoMerger>();
	DemoMerger* const merger = (DemoMerger*)allocator.GetAddressAt(mergerOffset);

	return merger->MergeDemos(info, filePaths, fileCount, protocol);
}
