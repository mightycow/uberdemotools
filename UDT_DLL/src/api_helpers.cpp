#include "api_helpers.hpp"
#include "parser_context.hpp"
#include "linear_allocator.hpp"
#include "scoped_stack_allocator.hpp"
#include "utils.hpp"
#include "timer.hpp"
#include "analysis_cut_by_chat.hpp"
#include "analysis_cut_by_frag.hpp"
#include "plug_in_converter_quake_to_udt.hpp"
#include "parser_runner.hpp"
#include "converter_entity_timer_shifter.hpp"
#include "path.hpp"
#include "memory_stream.hpp"


bool InitContextWithPlugIns(udtParserContext& context, const udtParseArg& info, u32 demoCount, udtParsingJobType::Id jobType, const void* jobSpecificInfo)
{
	if(jobType == udtParsingJobType::General)
	{
		for(u32 i = 0; i < info.PlugInCount; ++i)
		{
			const u32 plugIn = info.PlugIns[i];
			if(plugIn >= (u32)udtPrivateParserPlugIn::Count)
			{
				return false;
			}
		}

		context.Init(demoCount, info.PlugIns, info.PlugInCount);
		return true;
	}

	if(jobType == udtParsingJobType::Conversion)
	{
		if(jobSpecificInfo == NULL)
		{
			return false;
		}

		context.Init(demoCount, NULL, 0);
		return true;
	}

	if(jobType == udtParsingJobType::TimeShift)
	{
		if(jobSpecificInfo == NULL)
		{
			return false;
		}

		const u32 plugInId = udtPrivateParserPlugIn::ConvertToUDT;
		context.Init(demoCount, &plugInId, 1);

		udtBaseParserPlugIn* plugInBase = NULL;
		context.GetPlugInById(plugInBase, plugInId);
		if(plugInBase == NULL)
		{
			return false;
		}
		
		return true;
	}
	
	// Remaining case: udtParsingJobType::CutByPattern

	if(jobSpecificInfo == NULL)
	{
		return false;
	}

	const u32 plugInId = udtPrivateParserPlugIn::CutByPattern;
	context.Init(demoCount, &plugInId, 1);

	udtBaseParserPlugIn* plugInBase = NULL;
	context.GetPlugInById(plugInBase, plugInId);
	if(plugInBase == NULL)
	{
		return false;
	}

	const udtCutByPatternArg* const patternInfo = (const udtCutByPatternArg*)jobSpecificInfo;
	udtCutByPatternPlugIn& plugIn = *(udtCutByPatternPlugIn*)plugInBase;
	plugIn.SetPatternInfo(*patternInfo);
	for(u32 i = 0; i < patternInfo->PatternCount; ++i)
	{
		const udtPatternInfo pi = patternInfo->Patterns[i];
		plugIn.CreateAndAddAnalyzer((udtPatternType::Id)pi.Type, pi.TypeSpecificInfo);
	}

	plugIn.InitAnalyzerAllocators(demoCount);

	return true;
}

static bool ParseDemoFile(udtProtocol::Id protocol, udtParserContext* context, const udtParseArg* info, const char* demoFilePath, bool clearPlugInData)
{
	context->ResetForNextDemo(!clearPlugInData);
	if(!context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext))
	{
		return false;
	}

	udtFileStream file;
	if(!file.Open(demoFilePath, udtFileOpenMode::Read))
	{
		return false;
	}

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
	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	return ParseDemoFile(protocol, context, info, demoFilePath, clearPlugInData);
}

static bool CutByPattern(udtParserContext* context, const udtParseArg* info, const char* demoFilePath)
{
	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	if(!ParseDemoFile(protocol, context, info, demoFilePath, false))
	{
		return false;
	}

	udtBaseParserPlugIn* plugInBase = NULL;
	context->GetPlugInById(plugInBase, udtPrivateParserPlugIn::CutByPattern);
	udtCutByPatternPlugIn& plugIn = *(udtCutByPatternPlugIn*)plugInBase;

	if(plugIn.CutSections.IsEmpty())
	{
		return true;
	}

	context->ResetForNextDemo(true);
	if(!context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext))
	{
		return false;
	}

	udtFileStream file;
	if(!file.Open(demoFilePath, udtFileOpenMode::Read))
	{
		return false;
	}

	const s32 gsIndex = plugIn.CutSections[0].GameStateIndex;
	const u32 fileOffset = context->Parser._inGameStateFileOffsets[gsIndex];
	if(fileOffset > 0 && file.Seek((s32)fileOffset, udtSeekOrigin::Start) != 0)
	{
		return false;
	}

	// Save the cut sections in a temporary array.
	udtVMArrayWithAlloc<udtCutSection> sections(1 << 16);
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
			&CallbackCutDemoFileStreamCreation, section.VeryShortDesc, &cutCbInfo);
	}

	context->Context.LogInfo("Processing demo for applying cut(s): %s", demoFilePath);

	context->Context.SetCallbacks(info->MessageCb, NULL, NULL);
	const bool result = RunParser(context->Parser, file, info->CancelOperation);
	context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext);

	return result;
}

static bool ConvertDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath, const udtProtocolConversionArg* conversionInfo)
{
	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
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

	udtFileStream file;
	if(!file.Open(demoFilePath, udtFileOpenMode::Read))
	{
		return false;
	}

	if(!context->Parser.Init(&context->Context, protocol, (udtProtocol::Id)conversionInfo->OutputProtocol))
	{
		return false;
	}

	context->Parser._protocolConverter->ConversionInfo = conversionInfo;

	context->Parser.SetFilePath(demoFilePath);

	CallbackCutDemoFileStreamCreationInfo cutCbInfo;
	cutCbInfo.OutputFolderPath = info->OutputFolderPath;
	context->Parser.AddCut(0, S32_MIN, S32_MAX, &CallbackConvertedDemoFileStreamCreation, "", &cutCbInfo);

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

	const char* outputFilePathParts[] =
	{
		outputFilePathStart.String,
		"_shifted_",
		snapshotCount,
		timeShiftArg->SnapshotCount > 1 ? "_snaps" : "_snap",
		udtGetFileExtensionByProtocol(protocol)
	};

	outputFilePath = udtString::NewFromConcatenatingMultiple(allocator, outputFilePathParts, (u32)UDT_COUNT_OF(outputFilePathParts));
}

static bool TimeShiftDemo(udtParserContext* context, const udtParseArg* info, const char* demoFilePath, const udtTimeShiftArg* timeShiftArg)
{
	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
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

	udtFileStream input;
	if(!input.Open(demoFilePath, udtFileOpenMode::Read))
	{
		return false;
	}

	context->ModifierContext.ResetForNextDemo();

	udtString outputFilePath;
	CreateTimeShiftDemoName(outputFilePath, context->ModifierContext.TempAllocator, udtString::NewConstRef(demoFilePath), info->OutputFolderPath, timeShiftArg, protocol);

	udtFileStream output;
	if(!output.Open(outputFilePath.String, udtFileOpenMode::Write))
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

	context->Context.LogInfo("Writing time-shifted demo: %s", outputFilePath.String);

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

	return runner.WasSuccess() && messageType == udtdMessageType::EndOfFile;
}

bool ProcessSingleDemoFile(udtParsingJobType::Id jobType, udtParserContext* context, const udtParseArg* info, const char* demoFilePath, const void* jobSpecificInfo)
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

		default:
			return false;
	}
}

struct SingleThreadProgressContext
{
	u64 TotalByteCount;
	u64 ProcessedByteCount;
	u64 CurrentJobByteCount;
	udtProgressCallback UserCallback;
	void* UserData;
	udtTimer* Timer;
};

static void SingleThreadProgressCallback(f32 jobProgress, void* userData)
{
	SingleThreadProgressContext* const context = (SingleThreadProgressContext*)userData;
	if(context == NULL || context->Timer == NULL || context->UserCallback == NULL)
	{
		return;
	}

	if(context->Timer->GetElapsedMs() < UDT_MIN_PROGRESS_TIME_MS)
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

	udtTimer timer;
	timer.Start();

	udtVMArrayWithAlloc<u64> fileSizes((uptr)sizeof(u64) * (uptr)extraInfo->FileCount);
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
	progressContext.Timer = &timer;
	progressContext.UserCallback = info->ProgressCb;
	progressContext.UserData = info->ProgressContext;
	progressContext.CurrentJobByteCount = 0;
	progressContext.ProcessedByteCount = 0;
	progressContext.TotalByteCount = totalByteCount;

	udtParseArg newInfo = *info;
	newInfo.ProgressCb = &SingleThreadProgressCallback;
	newInfo.ProgressContext = &progressContext;

	for(u32 i = 0; i < extraInfo->FileCount; ++i)
	{
		if(info->CancelOperation != NULL && *info->CancelOperation != 0)
		{
			break;
		}

		const u64 jobByteCount = fileSizes[i];
		progressContext.CurrentJobByteCount = jobByteCount;

		const bool success = ProcessSingleDemoFile(jobType, context, &newInfo, extraInfo->FilePaths[i], jobSpecificInfo);
		extraInfo->OutputErrorCodes[i] = GetErrorCode(success, info->CancelOperation);

		progressContext.ProcessedByteCount += jobByteCount;
	}

	if((info->Flags & (u32)udtParseArgFlags::PrintAllocStats) != 0)
	{
		udtVMLinearAllocator::Stats allocStats;
		udtVMLinearAllocator::GetThreadStats(allocStats);
		const uptr extraByteCount = (uptr)sizeof(udtParserContext);
		allocStats.CommittedByteCount += extraByteCount;
		allocStats.UsedByteCount += extraByteCount;
		context->Parser._tempAllocator.Clear();
		LogLinearAllocatorStats(1, extraInfo->FileCount, context->Context, context->Parser._tempAllocator, allocStats);
	}

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

	const char* outputFilePathParts[] =
	{
		outputFilePathStart.String,
		"_merged",
		udtGetFileExtensionByProtocol(protocol)
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

		udtVMLinearAllocator tempAllocator;
		if(!tempAllocator.Init(1 << 16))
		{
			return false;
		}

		udtString outputFilePath;
		CreateMergedDemoName(outputFilePath, tempAllocator, udtString::NewConstRef(filePaths[0]), info->OutputFolderPath, protocol);

		udtFileStream output;
		if(!output.Open(outputFilePath.String, udtFileOpenMode::Write))
		{
			return false;
		}

		for(u32 i = 0; i < fileCount; ++i)
		{
			DemoData& demo = _demos[i];

			demo.Context = udtCreateContext();

			const u32 plugInId = udtPrivateParserPlugIn::ConvertToUDT;
			demo.Context->Init(1, &plugInId, 1);

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

			if(!demo.WriteBuffer.Open(1 << 20))
			{
				return false;
			}

			demo.ConverterToUDT->SetOutputStream(&demo.WriteBuffer);
			demo.ConverterToQuake.ResetForNextDemo(demo.ReadBuffer, i == 0 ? &output : NULL, protocol);
		}

		DemoData& firstDemo = _demos[0];
		s32 firstTime = S32_MIN;
		udtdMessageType::Id messageType = udtdMessageType::Invalid;
		udtdConverter::SnapshotInfo snapshotInfo;

		firstDemo.Context->Context.LogInfo("Writing merged demo: %s", outputFilePath.String);

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
		if(firstTime == S32_MIN)
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
	udtVMLinearAllocator allocator;
	allocator.Init(1 << 24);
	udtVMScopedStackAllocator allocatorScope(allocator);
	DemoMerger* const merger = allocatorScope.NewObject<DemoMerger>();

	return merger->MergeDemos(info, filePaths, fileCount, protocol);
}
