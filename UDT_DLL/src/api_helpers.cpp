#include "api_helpers.hpp"
#include "parser_context.hpp"
#include "linear_allocator.hpp"
#include "scoped_stack_allocator.hpp"
#include "utils.hpp"
#include "timer.hpp"
#include "analysis_cut_by_chat.hpp"
#include "analysis_cut_by_frag.hpp"
#include "analysis_cut_by_awards.hpp"


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

template<class udtParserPlugInCutByWhatever, class udtCutByWhateverArg>
static bool CutByWhatever(udtParserContext* context, const udtParseArg* info, const udtCutByWhateverArg* whateverInfo, const char* demoFilePath, const char* analysisType)
{
	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	udtParserPlugInCutByWhatever plugIn(*whateverInfo);
	if(!RunParserWithPlugIn(context, plugIn, protocol, info, demoFilePath, analysisType))
	{
		return false;
	}

	if(plugIn.Analyzer.CutSections.IsEmpty())
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

	const s32 gsIndex = plugIn.Analyzer.CutSections[0].GameStateIndex;
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

	const udtCutAnalyzerBase::CutSectionVector& sections = plugIn.Analyzer.CutSections;
	for(u32 i = 0, count = sections.GetSize(); i < count; ++i)
	{
		const udtCutAnalyzerBase::CutSection& section = sections[i];
		context->Parser.AddCut(section.GameStateIndex, section.StartTimeMs, section.EndTimeMs, &CallbackCutDemoFileStreamCreation, &cutCbInfo);
	}

	context->Context.LogInfo("Processing for %s cut(s): %s", analysisType, demoFilePath);

	udtVMScopedStackAllocator tempAllocScope(context->Context.TempAllocator);

	context->Context.SetCallbacks(info->MessageCb, NULL, NULL);
	const bool result = RunParser(context->Parser, file, info->CancelOperation);
	context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext);

	return result;
}

bool CutByChat(udtParserContext* context, const udtParseArg* info, const udtCutByChatArg* chatInfo, const char* demoFilePath)
{
	typedef udtCutPlugInBase<udtCutByChatAnalyzer, udtCutByChatArg> T;

	return CutByWhatever<T>(context, info, chatInfo, demoFilePath, "chat");
}

bool CutByFrag(udtParserContext* context, const udtParseArg* info, const udtCutByFragArg* fragInfo, const char* demoFilePath)
{
	typedef udtCutPlugInBase<udtCutByFragAnalyzer, udtCutByFragArg> T;

	return CutByWhatever<T>(context, info, fragInfo, demoFilePath, "frag");
}

bool CutByAward(udtParserContext* context, const udtParseArg* info, const udtCutByAwardArg* awardInfo, const char* demoFilePath)
{
	typedef udtCutPlugInBase<udtCutByAwardAnalyzer, udtCutByAwardArg> T;

	return CutByWhatever<T>(context, info, awardInfo, demoFilePath, "award");
}

bool CutByWhatever(udtParsingJobType::Id jobType, udtParserContext* context, const udtParseArg* info, const void* jobSpecificInfo, const char* demoFilePath)
{
	switch(jobType)
	{
		case udtParsingJobType::CutByChat: return CutByChat(context, info, (const udtCutByChatArg*)jobSpecificInfo, demoFilePath);
		case udtParsingJobType::CutByFrag: return CutByFrag(context, info, (const udtCutByFragArg*)jobSpecificInfo, demoFilePath);
		case udtParsingJobType::CutByAward: return CutByAward(context, info, (const udtCutByAwardArg*)jobSpecificInfo, demoFilePath);
		default: return false;
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
		else
		{
			success = CutByWhatever(jobType, context, &newInfo, jobSpecificInfo, extraInfo->FilePaths[i]);
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
