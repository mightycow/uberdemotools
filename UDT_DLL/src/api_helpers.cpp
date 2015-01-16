#include "api_helpers.hpp"
#include "parser_context.hpp"
#include "linear_allocator.hpp"
#include "scoped_stack_allocator.hpp"
#include "utils.hpp"
#include "timer.hpp"
#include "analysis_cut_by_chat.hpp"
#include "analysis_cut_by_frag.hpp"


bool InitContextWithPlugIns(udtParserContext& context, const udtParseArg& info, u32 demoCount, udtParsingJobType::Id jobType, const udtCutByPatternArg* patternInfo)
{
	if(jobType == udtParsingJobType::General)
	{
		context.Init(demoCount, info.PlugIns, info.PlugInCount);
		return true;
	}

	if(patternInfo == NULL)
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

	if(!context->Parser.Init(&context->Context, protocol))
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

bool ParseDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath, bool clearPlugInData)
{
	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	return ParseDemoFile(protocol, context, info, demoFilePath, clearPlugInData);
}

bool CutByPattern(udtParserContext* context, const udtParseArg* info, const char* demoFilePath)
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
	if(!context->Parser.Init(&context->Context, protocol, gsIndex, false))
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

	if(!InitContextWithPlugIns(*context, *info, extraInfo->FileCount, jobType, patternInfo))
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

		bool success = false;
		if(jobType == udtParsingJobType::General)
		{
			success = ParseDemoFile(context, &newInfo, extraInfo->FilePaths[i], false);
		}
		else if(jobType == udtParsingJobType::CutByPattern)
		{
			success = CutByPattern(context, &newInfo, extraInfo->FilePaths[i]);
		}
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
		LogLinearAllocatorStats(context->Context, context->Parser._tempAllocator, allocStats);
	}

	if(customContext)
	{
		udtDestroyContext(context);
	}

	return GetErrorCode(true, info->CancelOperation);
}
