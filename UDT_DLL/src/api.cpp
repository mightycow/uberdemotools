#include "api.h"
#include "parser_context.hpp"
#include "common.hpp"
#include "utils.hpp"
#include "file_system.hpp"
#include "scoped_stack_allocator.hpp"
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

	if(!context->Context.Init(info->MessageCb, info->ProgressCb))
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
	context->StartDemo();
	if(!RunParser(context->Parser, file))
	{
		return (s32)udtErrorCode::OperationFailed;
	}
	context->EndDemo();

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
	if(!context->Context.Init(info->MessageCb, info->ProgressCb))
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

	context->StartDemo();
	if(!RunParser(context->Parser, file))
	{
		return (s32)udtErrorCode::OperationFailed;
	}
	context->EndDemo();

	return (s32)udtErrorCode::None;
}

static bool GetCutByChatMergedSections(udtParserContext* context, udtParserPlugInCutByChat& plugIn, udtProtocol::Id protocol, const udtParseArg* info, const char* filePath)
{
	context->Reset();
	if(!context->Context.Init(info->MessageCb, info->ProgressCb))
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

	context->StartDemo();
	if(!RunParser(context->Parser, file))
	{
		return false;
	}
	context->EndDemo();

	if(plugIn.Analyzer.MergedCutSections.IsEmpty())
	{
		return false;
	}

	return true;
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

	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	udtParserPlugInCutByChat plugIn(*chatInfo);
	if(!GetCutByChatMergedSections(context, plugIn, protocol, info, demoFilePath))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	context->Reset();
	if(!context->Context.Init(info->MessageCb, info->ProgressCb))
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

	CallbackCutDemoFileStreamCreationInfo cutCbInfo;
	cutCbInfo.OutputFolderPath = info->OutputFolderPath;

	const udtCutByChatAnalyzer::CutSectionVector& sections = plugIn.Analyzer.MergedCutSections;
	for(u32 i = 0, count = sections.GetSize(); i < count; ++i)
	{
		context->Parser.AddCut(sections[i].StartTimeMs, sections[i].EndTimeMs, &CallbackCutDemoFileStreamCreation, &cutCbInfo);
	}

	context->Context.LogInfo("Processing for chat cut(s): %s", demoFilePath);

	udtVMScopedStackAllocator tempAllocScope(context->Context.TempAllocator);

	context->StartDemo();
	if(!RunParser(context->Parser, file))
	{
		return (s32)udtErrorCode::OperationFailed;
	}
	context->EndDemo();

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

UDT_API(s32) udtParseDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath)
{
	if(context == NULL || info == NULL || demoFilePath == 0 ||
	   info->PlugInCount == 0 || info->PlugIns == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	context->Reset();

	for(u32 i = 0; i < info->PlugInCount; ++i)
	{
		context->CreateAndAddPlugIn(info->PlugIns[i]);
	}

	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return udtErrorCode::InvalidArgument;
	}

	if(!context->Context.Init(info->MessageCb, info->ProgressCb))
	{
		return udtErrorCode::OperationFailed;
	}

	udtFileStream file;
	if(!file.Open(demoFilePath, udtFileOpenMode::Read))
	{
		return udtErrorCode::OperationFailed;
	}

	if(!context->Parser.Init(&context->Context, protocol))
	{
		return udtErrorCode::OperationFailed;
	}

	udtVMScopedStackAllocator tempAllocScope(context->Context.TempAllocator);

	context->Parser.SetFilePath(demoFilePath);
	context->StartDemo();
	if(!RunParser(context->Parser, file))
	{
		return udtErrorCode::OperationFailed;
	}
	context->EndDemo();

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

	void StartDemo(u32 contextIdx)
	{
		if(contextIdx >= ContextCount)
		{
			return;
		}

		Contexts[contextIdx].StartDemo();
	}

	void EndDemo(u32 contextIdx)
	{
		if(contextIdx >= ContextCount)
		{
			return;
		}

		Contexts[contextIdx].EndDemo();
	}
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

	udtParserContext* it = (udtParserContext*)(contextGroup + 1);
	for(u32 i = 0; i < contextGroup->ContextCount; ++i)
	{
		it->~udtParserContext();
	}

	contextGroup->~udtParserContextGroup();
}

static u32 ComputeFinalThreadCount(const udtMultiParseArg* /*extraInfo*/)
{
	// @TODO:
	return 1;
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

UDT_API(s32) udtParseDemoFiles(udtParserContextGroup** contextGroup, const udtParseArg* info, const udtMultiParseArg* extraInfo)
{
	if(contextGroup == NULL || info == NULL || extraInfo == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	const u32 threadCount = ComputeFinalThreadCount(extraInfo);
	if(threadCount == 1)
	{
		udtParserContext* context = udtCreateContext(extraInfo->CrashCb);
		if(context == NULL)
		{
			return (s32)udtErrorCode::OperationFailed;
		}

		for(u32 i = 0; i < extraInfo->FileCount; ++i)
		{
			udtParseDemoFile(context, info, extraInfo->FilePaths[i]);
		}

		udtDestroyContext(context);

		return (s32)udtErrorCode::None;
	}

	if(!CreateContextGroup(contextGroup, threadCount))
	{
		return (s32)udtErrorCode::OperationFailed;
	}
	// @TODO:

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtCutDemoFilesByChat(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtCutByChatArg* chatInfo)
{
	if(info == NULL || extraInfo == NULL || chatInfo == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	const u32 threadCount = ComputeFinalThreadCount(extraInfo);
	if(threadCount == 1)
	{
		udtParserContext* context = udtCreateContext(extraInfo->CrashCb);
		if(context == NULL)
		{
			return (s32)udtErrorCode::OperationFailed;
		}

		for(u32 i = 0; i < extraInfo->FileCount; ++i)
		{
			udtCutDemoFileByChat(context, info, chatInfo, extraInfo->FilePaths[i]);
		}

		udtDestroyContext(context);

		return (s32)udtErrorCode::None;
	}

	// @TODO:

	return (s32)udtErrorCode::None;
}
