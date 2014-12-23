#include "api_helpers.hpp"
#include "parser_context.hpp"
#include "linear_allocator.hpp"
#include "scoped_stack_allocator.hpp"
#include "utils.hpp"
#include "timer.hpp"
#include "analysis_cut_by_chat.hpp"
#include "analysis_cut_by_frag.hpp"


bool ParseDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath, bool clearPlugInData)
{
	if(clearPlugInData)
	{
		context->Reset();
	}
	else
	{
		context->ResetButKeepPlugInData();
	}

	context->CreateAndAddPlugIns(info->PlugIns, info->PlugInCount);

	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	if(!context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext))
	{
		return false;
	}

	udtFileStream file;
	if(!file.Open(demoFilePath, udtFileOpenMode::Read))
	{
		return false;
	}

	if(!context->Parser.Init(&context->Context, protocol))
	{
		return false;
	}

	udtVMScopedStackAllocator tempAllocScope(context->Context.TempAllocator);

	context->Parser.SetFilePath(demoFilePath);
	if(!RunParser(context->Parser, file, info->CancelOperation))
	{
		return false;
	}

	return true;
}

static bool RunParserWithCutByPattern(
	udtParserContext* context, 
	udtProtocol::Id protocol, 
	const udtParseArg* info, 
	const char* filePath, 
	const udtCutByPatternArg* patternInfo,
	udtCutByPatternPlugIn& plugIn)
{
	context->Reset();
	if(!context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext))
	{
		return false;
	}

	udtFileStream file;
	if(!file.Open(filePath, udtFileOpenMode::Read))
	{
		return false;
	}

	if(!context->Parser.Init(&context->Context, protocol))
	{
		return false;
	}

	context->Parser.SetFilePath(filePath);

	for(u32 i = 0; i < patternInfo->PatternCount; ++i)
	{
		const udtPatternInfo info = patternInfo->Patterns[i];
		plugIn.CreateAndAddAnalyzer((udtPatternType::Id)info.Type, patternInfo, info.TypeSpecificInfo);
	}
	context->Parser.AddPlugIn(&plugIn);

	context->Context.LogInfo("Processing demo for pattern analysis: %s", filePath);

	udtVMScopedStackAllocator tempAllocScope(context->Context.TempAllocator);

	if(!RunParser(context->Parser, file, info->CancelOperation))
	{
		return false;
	}

	return true;
}

bool CutByPattern(udtParserContext* context, const udtParseArg* info, const udtCutByPatternArg* patternInfo, const char* demoFilePath)
{
	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	udtCutByPatternPlugIn plugIn;
	if(!RunParserWithCutByPattern(context, protocol, info, demoFilePath, patternInfo, plugIn))
	{
		return false;
	}

	if(plugIn.CutSections.IsEmpty())
	{
		return true;
	}

	context->Reset();
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

	if(!context->Parser.Init(&context->Context, protocol, gsIndex))
	{
		return false;
	}

	context->Parser.SetFilePath(demoFilePath);

	CallbackCutDemoFileStreamCreationInfo cutCbInfo;
	cutCbInfo.OutputFolderPath = info->OutputFolderPath;

	const udtVMArray<udtCutSection>& sections = plugIn.CutSections;
	for(u32 i = 0, count = sections.GetSize(); i < count; ++i)
	{
		const udtCutSection& section = sections[i];
		context->Parser.AddCut(section.GameStateIndex, section.StartTimeMs, section.EndTimeMs, &CallbackCutDemoFileStreamCreation, &cutCbInfo);
	}

	context->Context.LogInfo("Processing demo for applying cut(s): %s", demoFilePath);

	udtVMScopedStackAllocator tempAllocScope(context->Context.TempAllocator);

	context->Context.SetCallbacks(info->MessageCb, NULL, NULL);
	const bool result = RunParser(context->Parser, file, info->CancelOperation);
	context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext);

	return result;
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

s32 udtParseMultipleDemosSingleThread(udtParsingJobType::Id jobType, udtParserContext* context, const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtCutByPatternArg* patternInfo)
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

	udtTimer timer;
	timer.Start();

	udtVMArray<u64> fileSizes;
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

		bool success = false;
		if(jobType == udtParsingJobType::General)
		{
			success = ParseDemoFile(context, &newInfo, extraInfo->FilePaths[i], false);
		}
		else if(jobType == udtParsingJobType::CutByPattern)
		{
			success = CutByPattern(context, &newInfo, patternInfo, extraInfo->FilePaths[i]);
		}
		extraInfo->OutputErrorCodes[i] = GetErrorCode(success, info->CancelOperation);

		progressContext.ProcessedByteCount += jobByteCount;
	}

	if(customContext)
	{
		udtDestroyContext(context);
	}

	return GetErrorCode(true, info->CancelOperation);
}
