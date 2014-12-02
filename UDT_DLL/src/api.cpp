#include "api.h"
#include "parser_context.hpp"
#include "common.hpp"
#include "utils.hpp"
#include "file_system.hpp"
#include "timer.hpp"
#include "scoped_stack_allocator.hpp"
#include "multi_threaded_processing.hpp"
#include "analysis_splitter.hpp"
#include "analysis_cut_by_chat.hpp"

// For malloc and free.
#include <stdlib.h>
#if defined(UDT_MSVC)
#	include <malloc.h>
#endif

// For the placement new operator.
#include <new>


#define UDT_API UDT_API_DEF

static const char* VersionString = "0.0.0";

#define UDT_PROTOCOL_ITEM(Enum, Ext) Ext,
static const char* DemoFileExtensions[udtProtocol::AfterLastProtocol + 1] =
{
	UDT_PROTOCOL_LIST(UDT_PROTOCOL_ITEM)
	".after_last"
};
#undef UDT_PROTOCOL_ITEM



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

UDT_API(const char*) udtGetVersionString()
{
	return VersionString;
}

UDT_API(s32) udtIsValidProtocol(udtProtocol::Id protocol)
{
	return (protocol >= udtProtocol::AfterLastProtocol || (s32)protocol < (s32)1) ? 0 : 1;
}

UDT_API(u32) udtGetSizeOfIdEntityState(udtProtocol::Id protocol)
{
	if(protocol == udtProtocol::Dm68)
	{
		return sizeof(idEntityState68);
	}

	if(protocol == udtProtocol::Dm73)
	{
		return sizeof(idEntityState73);
	}

	if(protocol == udtProtocol::Dm90)
	{
		return sizeof(idEntityState90);
	}

	return 0;
}

UDT_API(u32) udtGetSizeOfidClientSnapshot(udtProtocol::Id protocol)
{
	if(protocol == udtProtocol::Dm68)
	{
		return sizeof(idClientSnapshot68);
	}

	if(protocol == udtProtocol::Dm73)
	{
		return sizeof(idClientSnapshot73);
	}

	if(protocol == udtProtocol::Dm90)
	{
		return sizeof(idClientSnapshot90);
	}

	return 0;
}

UDT_API(const char*) udtGetFileExtensionByProtocol(udtProtocol::Id protocol)
{
	if(!udtIsValidProtocol(protocol))
	{
		return NULL;
	}

	return DemoFileExtensions[protocol];
}

UDT_API(udtProtocol::Id) udtGetProtocolByFilePath(const char* filePath)
{
	for(s32 i = (s32)udtProtocol::Invalid + 1; i < (s32)udtProtocol::AfterLastProtocol; ++i)
	{
		if(StringEndsWith_NoCase(filePath, DemoFileExtensions[i]))
		{
			return (udtProtocol::Id)i;
		}
	}
	
	return udtProtocol::Invalid;
}

static bool CreateDemoFileSplit(udtContext& context, udtStream& file, const char* filePath, const char* outputFolderPath, u32 index, u32 startOffset, u32 endOffset)
{
	if(endOffset <= startOffset)
	{
		return false;
	}

	if(file.Seek((s32)startOffset, udtSeekOrigin::Start) != 0)
	{
		return false;
	}

	const udtProtocol::Id protocol = udtGetProtocolByFilePath(filePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	udtVMLinearAllocator& tempAllocator = context.TempAllocator;
	udtVMScopedStackAllocator scopedTempAllocator(tempAllocator);

	char* fileName = NULL;
	if(!GetFileNameWithoutExtension(fileName, tempAllocator, filePath))
	{
		fileName = AllocateString(tempAllocator, "NEW_UDT_SPLIT_DEMO");
	}
	
	char* outputFilePathStart = NULL;
	if(outputFolderPath == NULL)
	{
		char* inputFolderPath = NULL;
		GetFolderPath(inputFolderPath, tempAllocator, filePath);
		StringPathCombine(outputFilePathStart, tempAllocator, inputFolderPath, fileName);
	}
	else
	{
		StringPathCombine(outputFilePathStart, tempAllocator, outputFolderPath, fileName);
	}

	char* newFilePath = AllocateSpaceForString(tempAllocator, UDT_MAX_PATH_LENGTH);
	sprintf(newFilePath, "%s_SPLIT_%u%s", outputFilePathStart, index + 1, udtGetFileExtensionByProtocol(protocol));

	context.LogInfo("Writing demo %s...", newFilePath);

	udtFileStream outputFile;
	if(!outputFile.Open(newFilePath, udtFileOpenMode::Write))
	{
		context.LogError("Could not open file");
		return false;
	}

	const bool success = CopyFileRange(file, outputFile, tempAllocator, startOffset, endOffset);
	if(success)
	{
		context.LogInfo("Done!");
	}
	else
	{
		context.LogError("File copy failed");
	}

	return success;
}

static bool CreateDemoFileSplit(udtContext& context, udtStream& file, const char* filePath, const char* outputFolderPath, const u32* fileOffsets, const u32 count)
{
	if(fileOffsets == NULL || count == 0)
	{
		return true;
	}

	// Exactly one gamestate message starting with the file.
	if(count == 1 && fileOffsets[0] == 0)
	{
		return true;
	}

	const u32 fileLength = (u32)file.Length();

	bool success = true;

	u32 start = 0;
	u32 end = 0;
	u32 indexOffset = 0;
	for(u32 i = 0; i < count; ++i)
	{
		end = fileOffsets[i];
		if(start == end)
		{
			++indexOffset;
			start = end;
			continue;
		}

		success = success && CreateDemoFileSplit(context, file, filePath, outputFolderPath, i - indexOffset, start, end);

		start = end;
	}

	end = fileLength;
	success = success && CreateDemoFileSplit(context, file, filePath, outputFolderPath, count - indexOffset, start, end);

	return success;
}

UDT_API(s32) udtSplitDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath)
{
	if(context == NULL || info == NULL || demoFilePath == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	context->Reset();

	if(!context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	udtFileStream file;
	if(!file.Open(demoFilePath, udtFileOpenMode::Read))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	if(!context->Parser.Init(&context->Context, protocol))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	context->Parser.SetFilePath(demoFilePath);

	udtVMScopedStackAllocator tempAllocScope(context->Context.TempAllocator);

	DemoSplitterAnalyzer analyzer;
	context->Parser.AddPlugIn(&analyzer);
	if(!RunParser(context->Parser, file))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	if(analyzer.GamestateFileOffsets.GetSize() <= 1)
	{
		return (s32)udtErrorCode::None;
	}

	if(!CreateDemoFileSplit(context->Context, file, demoFilePath, info->OutputFolderPath, &analyzer.GamestateFileOffsets[0], analyzer.GamestateFileOffsets.GetSize()))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtCutDemoFileByTime(udtParserContext* context, const udtParseArg* info, const udtCutByTimeArg* cutInfo, const char* demoFilePath)
{
	if(context == NULL || info == NULL || demoFilePath == NULL || 
	   cutInfo == NULL || cutInfo->Cuts == NULL || cutInfo->CutCount == 0)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	context->Reset();
	if(!context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	udtFileStream file;
	if(!file.Open(demoFilePath, udtFileOpenMode::Read))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	if(!context->Parser.Init(&context->Context, protocol))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	CallbackCutDemoFileStreamCreationInfo streamInfo;
	streamInfo.OutputFolderPath = info->OutputFolderPath;

	context->Parser.SetFilePath(demoFilePath);

	for(u32 i = 0; i < cutInfo->CutCount; ++i)
	{
		const udtCut& cut = cutInfo->Cuts[i];
		if(cut.StartTimeMs < cut.EndTimeMs)
		{
			context->Parser.AddCut(cut.StartTimeMs, cut.EndTimeMs, &CallbackCutDemoFileStreamCreation, &streamInfo);
		}
	}

	context->Context.LogInfo("Processing for a timed cut: %s", demoFilePath);

	if(!RunParser(context->Parser, file))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	return (s32)udtErrorCode::None;
}

static bool GetCutByChatMergedSections(udtParserContext* context, udtParserPlugInCutByChat& plugIn, udtProtocol::Id protocol, const udtParseArg* info, const char* filePath)
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
	context->Parser.AddPlugIn(&plugIn);

	context->Context.LogInfo("Processing for chat analysis: %s", filePath);

	udtVMScopedStackAllocator tempAllocScope(context->Context.TempAllocator);

	if(!RunParser(context->Parser, file))
	{
		return false;
	}

	return true;
}

// @TODO: Move this.
bool CutByChat(udtParserContext* context, const udtParseArg* info, const udtCutByChatArg* chatInfo, const char* demoFilePath)
{
	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	udtParserPlugInCutByChat plugIn(*chatInfo);
	if(!GetCutByChatMergedSections(context, plugIn, protocol, info, demoFilePath))
	{
		return false;
	}

	if(plugIn.Analyzer.MergedCutSections.IsEmpty())
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

	if(!context->Parser.Init(&context->Context, protocol))
	{
		return false;
	}

	context->Parser.SetFilePath(demoFilePath);

	CallbackCutDemoFileStreamCreationInfo cutCbInfo;
	cutCbInfo.OutputFolderPath = info->OutputFolderPath;

	const udtCutByChatAnalyzer::CutSectionVector& sections = plugIn.Analyzer.MergedCutSections;
	for(u32 i = 0, count = sections.GetSize(); i < count; ++i)
	{
		context->Parser.AddCut(sections[i].StartTimeMs, sections[i].EndTimeMs, &CallbackCutDemoFileStreamCreation, &cutCbInfo);
	}

	context->Context.LogInfo("Processing for chat cut(s): %s", demoFilePath);

	udtVMScopedStackAllocator tempAllocScope(context->Context.TempAllocator);

	context->Context.SetCallbacks(info->MessageCb, NULL, NULL);
	const bool result = RunParser(context->Parser, file);
	context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext);

	return result;
}

UDT_API(s32) udtCutDemoFileByChat(udtParserContext* context, const udtParseArg* info, const udtCutByChatArg* chatInfo, const char* demoFilePath)
{
	if(context == NULL || info == NULL || demoFilePath == NULL ||
	   chatInfo == NULL || chatInfo->Rules == NULL || chatInfo->RuleCount == 0)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	if(info->OutputFolderPath != NULL && !IsValidDirectory(info->OutputFolderPath))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	if(!CutByChat(context, info, chatInfo, demoFilePath))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	return (s32)udtErrorCode::None;
}

UDT_API(udtParserContext*) udtCreateContext(udtCrashCallback crashCb)
{
	// @NOTE: We don't use the standard operator new approach to avoid C++ exceptions.
	udtParserContext* const context = (udtParserContext*)malloc(sizeof(udtParserContext));
	if(context == NULL)
	{
		return NULL;
	}

	new (context) udtParserContext;
	context->Context.SetCrashCallback(crashCb);

	return context;
}

UDT_API(s32) udtDestroyContext(udtParserContext* context)
{
	if(context == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	// @NOTE: We don't use the standard operator new approach to avoid C++ exceptions.
	context->~udtParserContext();
	free(context);

	return (s32)udtErrorCode::None;
}

// @TODO: Move this.
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
	if(!RunParser(context->Parser, file))
	{
		return false;
	}

	return true;
}

UDT_API(s32) udtParseDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath)
{
	if(context == NULL || info == NULL || demoFilePath == 0 ||
	   info->PlugInCount == 0 || info->PlugIns == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return udtErrorCode::InvalidArgument;
	}

	if(!ParseDemoFile(context, info, demoFilePath, true))
	{
		return udtErrorCode::OperationFailed;
	}

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtGetDemoDataInfo(udtParserContext* context, u32 demoIdx, u32 plugInId, void** buffer, u32* count)
{
	if(context == NULL || plugInId >= (u32)udtParserPlugIn::Count || buffer == NULL || count == NULL ||
	   demoIdx >= context->DemoCount)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	if(!context->GetDataInfo(demoIdx, plugInId, buffer, count))
	{
		return udtErrorCode::OperationFailed;
	}

	return (s32)udtErrorCode::None;
}

struct udtParserContextGroup
{
	udtParserContext* Contexts;
	u32 ContextCount;
};

static bool CreateContextGroup(udtParserContextGroup** contextGroupPtr, u32 contextCount)
{
	if(contextCount == 0)
	{
		return false;
	}

	const size_t byteCount = sizeof(udtParserContextGroup) + contextCount * sizeof(udtParserContext);
	udtParserContextGroup* const contextGroup = (udtParserContextGroup*)malloc(byteCount);
	if(contextGroup == NULL)
	{
		return false;
	}

	new (contextGroup) udtParserContextGroup;

	udtParserContext* const contexts = (udtParserContext*)(contextGroup + 1);
	for(u32 i = 0; i < contextCount; ++i)
	{
		new (contexts + i) udtParserContext;
	}

	contextGroup->Contexts = contexts;
	contextGroup->ContextCount = contextCount;
	*contextGroupPtr = contextGroup;

	return true;
}

static void DestroyContextGroup(udtParserContextGroup* contextGroup)
{
	if(contextGroup == NULL)
	{
		return;
	}

	const u32 contextCount = contextGroup->ContextCount;
	for(u32 i = 0; i < contextCount; ++i)
	{
		contextGroup->Contexts[i].~udtParserContext();
	}

	contextGroup->~udtParserContextGroup();
}

UDT_API(s32) udtDestroyContextGroup(udtParserContextGroup* contextGroup)
{
	if(contextGroup == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	DestroyContextGroup(contextGroup);

	return (s32)udtErrorCode::None;
}

static s32 udtParseDemoFiles_SingleThread(udtParserContext* context, const udtParseArg* info, const udtMultiParseArg* extraInfo)
{
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
		const u64 jobByteCount = fileSizes[i];
		progressContext.CurrentJobByteCount = jobByteCount;
		if(!ParseDemoFile(context, &newInfo, extraInfo->FilePaths[i], false))
		{
			return udtErrorCode::OperationFailed;
		}
		progressContext.ProcessedByteCount += jobByteCount;
	}

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtParseDemoFiles(udtParserContextGroup** contextGroup, const udtParseArg* info, const udtMultiParseArg* extraInfo)
{
	if(contextGroup == NULL || info == NULL || extraInfo == NULL ||
	   extraInfo->FileCount == 0 || extraInfo->FilePaths == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	for(u32 i = 0; i < extraInfo->FileCount; ++i)
	{
		const udtProtocol::Id protocol = udtGetProtocolByFilePath(extraInfo->FilePaths[i]);
		if(protocol == udtProtocol::Invalid)
		{
			return udtErrorCode::InvalidArgument;
		}
	}

	udtDemoThreadAllocator threadAllocator;
	const bool threadJob = threadAllocator.Process(extraInfo->FilePaths, extraInfo->FileCount, extraInfo->MaxThreadCount);
	const u32 threadCount = threadJob ? threadAllocator.Threads.GetSize() : 1;
	if(!CreateContextGroup(contextGroup, threadCount))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	if(!threadJob)
	{
		return udtParseDemoFiles_SingleThread((*contextGroup)->Contexts, info, extraInfo);
	}
	
	udtMultiThreadedParsing parser;
	const bool success = parser.Process((*contextGroup)->Contexts, threadAllocator, info, extraInfo, udtParsingJobType::General, NULL);

	return (s32)(success ? udtErrorCode::None : udtErrorCode::OperationFailed);
}

static s32 udtCutDemoFilesByChat_SingleThread(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtCutByChatArg* chatInfo)
{
	udtParserContext* context = udtCreateContext(extraInfo->CrashCb);
	if(context == NULL)
	{
		return (s32)udtErrorCode::OperationFailed;
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
		const u64 jobByteCount = fileSizes[i];
		progressContext.CurrentJobByteCount = jobByteCount;
		const s32 errorCode = udtCutDemoFileByChat(context, &newInfo, chatInfo, extraInfo->FilePaths[i]);
		if(errorCode != (s32)udtErrorCode::None)
		{
			udtDestroyContext(context);
			return errorCode;
		}
		progressContext.ProcessedByteCount += jobByteCount;
	}

	udtDestroyContext(context);

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtCutDemoFilesByChat(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtCutByChatArg* chatInfo)
{
	if(info == NULL || extraInfo == NULL || chatInfo == NULL ||
	   chatInfo->Rules == NULL || chatInfo->RuleCount == 0 || 
	   extraInfo->FileCount == 0 || extraInfo->FilePaths == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	if(info->OutputFolderPath != NULL && !IsValidDirectory(info->OutputFolderPath))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	for(u32 i = 0; i < extraInfo->FileCount; ++i)
	{
		const udtProtocol::Id protocol = udtGetProtocolByFilePath(extraInfo->FilePaths[i]);
		if(protocol == udtProtocol::Invalid)
		{
			return udtErrorCode::InvalidArgument;
		}
	}

	udtDemoThreadAllocator threadAllocator;
	const bool threadJob = threadAllocator.Process(extraInfo->FilePaths, extraInfo->FileCount, extraInfo->MaxThreadCount);
	if(!threadJob)
	{
		return udtCutDemoFilesByChat_SingleThread(info, extraInfo, chatInfo);
	}

	udtParserContextGroup* contextGroup;
	if(!CreateContextGroup(&contextGroup, threadAllocator.Threads.GetSize()))
	{
		return udtCutDemoFilesByChat_SingleThread(info, extraInfo, chatInfo);
	}

	udtMultiThreadedParsing parser;
	const bool success = parser.Process(contextGroup->Contexts, threadAllocator, info, extraInfo, udtParsingJobType::CutByChat, chatInfo);

	DestroyContextGroup(contextGroup);

	return (s32)(success ? udtErrorCode::None : udtErrorCode::OperationFailed);
}

UDT_API(s32) udtGetContextCountFromGroup(udtParserContextGroup* contextGroup, u32* count)
{
	if(contextGroup == NULL || count == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	*count = contextGroup->ContextCount;

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtGetContextFromGroup(udtParserContextGroup* contextGroup, u32 contextIdx, udtParserContext** context)
{
	if(contextGroup == NULL || context == NULL || contextIdx >= contextGroup->ContextCount)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	*context = &contextGroup->Contexts[contextIdx];

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtGetDemoCountFromGroup(udtParserContextGroup* contextGroup, u32* count)
{
	if(contextGroup == NULL || count == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	u32 demoCount = 0;
	for(u32 ctxIdx = 0, ctxCount = contextGroup->ContextCount; ctxIdx < ctxCount; ++ctxIdx)
	{
		const udtParserContext& context = contextGroup->Contexts[ctxIdx];
		demoCount += context.GetDemoCount();
	}

	*count = demoCount;

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtGetDemoCountFromContext(udtParserContext* context, u32* count)
{
	if(context == NULL || count == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	*count = context->GetDemoCount();

	return (s32)udtErrorCode::None;
}
