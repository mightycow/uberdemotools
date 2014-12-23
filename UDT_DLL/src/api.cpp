#include "api.h"
#include "api_helpers.hpp"
#include "api_arg_checks.hpp"
#include "parser_context.hpp"
#include "common.hpp"
#include "utils.hpp"
#include "file_system.hpp"
#include "timer.hpp"
#include "crash.hpp"
#include "scoped_stack_allocator.hpp"
#include "multi_threaded_processing.hpp"
#include "analysis_splitter.hpp"

// For malloc and free.
#include <stdlib.h>
#if defined(UDT_MSVC)
#	include <malloc.h>
#endif

// For the placement new operator.
#include <new>


#define UDT_API UDT_API_DEF


static const char* VersionString = "0.3.4a";


#define UDT_ERROR_ITEM(Enum, Desc) Desc,
static const char* ErrorCodeStrings[udtErrorCode::AfterLastError + 1] =
{
	UDT_ERROR_LIST(UDT_ERROR_ITEM)
	"invalid error code"
};
#undef UDT_ERROR_ITEM

#define UDT_PROTOCOL_ITEM(Enum, Ext) Ext,
static const char* DemoFileExtensions[udtProtocol::AfterLastProtocol + 1] =
{
	UDT_PROTOCOL_LIST(UDT_PROTOCOL_ITEM)
	".after_last"
};
#undef UDT_PROTOCOL_ITEM

#define UDT_WEAPON_ITEM(Enum, Desc, Bit) Desc,
static const char* WeaponNames[] =
{
	UDT_WEAPON_LIST(UDT_WEAPON_ITEM)
	"after last weapon"
};
#undef UDT_WEAPON_ITEM

#define UDT_POWER_UP_ITEM(Enum, Desc, Bit) Desc,
static const char* PowerUpNames[] =
{
	UDT_POWER_UP_LIST(UDT_POWER_UP_ITEM)
	"after last power-up"
};
#undef UDT_POWER_UP_ITEM

#define UDT_MOD_ITEM(Enum, Desc, Bit) Desc,
static const char* MeansOfDeathNames[] =
{
	UDT_MOD_LIST(UDT_MOD_ITEM)
	"after last MoD"
};
#undef UDT_MOD_ITEM

#define UDT_PLAYER_MOD_ITEM(Enum, Desc, Bit) Desc,
static const char* PlayerMeansOfDeathNames[] =
{
	UDT_PLAYER_MOD_LIST(UDT_PLAYER_MOD_ITEM)
	"after last player MoD"
};
#undef UDT_PLAYER_MOD_ITEM

#define UDT_TEAM_ITEM(Enum, Desc) Desc,
static const char* TeamNames[] =
{
	UDT_TEAM_LIST(UDT_TEAM_ITEM)
	"after last team"
};
#undef UDT_TEAM_ITEM

#define UDT_AWARD_ITEM(Enum, Desc, Bit) Desc,
static const char* AwardNames[] =
{
	UDT_AWARDS_LIST(UDT_AWARD_ITEM)
	"after last award"
};
#undef UDT_AWARD_ITEM


UDT_API(const char*) udtGetVersionString()
{
	return VersionString;
}

UDT_API(const char*) udtGetErrorCodeString(s32 errorCode)
{
	if(errorCode < 0 || errorCode > (s32)udtErrorCode::AfterLastError)
	{
		errorCode = (s32)udtErrorCode::AfterLastError;
	}

	return ErrorCodeStrings[errorCode];
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

UDT_API(s32) udtCrash(udtCrashType::Id crashType)
{
	if((u32)crashType >= (u32)udtCrashType::Count)
	{
		return udtErrorCode::InvalidArgument;
	}

	switch(crashType)
	{
		case udtCrashType::FatalError:
			FatalError(__FILE__, __LINE__, __FUNCTION__, "udtCrash test");
			break;

		case udtCrashType::ReadAccess:
			printf("Bullshit: %d\n", *(int*)0);
			break;

		case udtCrashType::WriteAccess:
			*(int*)0 = 1337;
			break;

		default:
			break;
	}

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtGetStringArray(udtStringArray::Id arrayId, const char*** elements, u32* elementCount)
{
	if(elements == NULL || elementCount == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	switch(arrayId)
	{
		case udtStringArray::Weapons:
			*elements = WeaponNames;
			*elementCount = (u32)(UDT_COUNT_OF(WeaponNames) - 1);
			break;

		case udtStringArray::PowerUps:
			*elements = PowerUpNames;
			*elementCount = (u32)(UDT_COUNT_OF(PowerUpNames) - 1);
			break;

		case udtStringArray::MeansOfDeath:
			*elements = MeansOfDeathNames;
			*elementCount = (u32)(UDT_COUNT_OF(MeansOfDeathNames) - 1);
			break;

		case udtStringArray::PlayerMeansOfDeath:
			*elements = PlayerMeansOfDeathNames;
			*elementCount = (u32)(UDT_COUNT_OF(PlayerMeansOfDeathNames) - 1);
			break;

		case udtStringArray::Teams:
			*elements = TeamNames;
			*elementCount = (u32)(UDT_COUNT_OF(TeamNames) - 1);
			break;

		case udtStringArray::Awards:
			*elements = AwardNames;
			*elementCount = (u32)(UDT_COUNT_OF(AwardNames) - 1);
			break;

		default:
			return (s32)udtErrorCode::InvalidArgument;
	}

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtSetCrashHandler(udtCrashCallback crashHandler)
{
	SetCrashHandler(crashHandler);

	return (s32)udtErrorCode::None;
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
	if(!success)
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
	if(context == NULL || info == NULL || demoFilePath == NULL ||
	   !HasValidOutputOption(*info))
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
	if(!RunParser(context->Parser, file, info->CancelOperation))
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
	if(context == NULL || info == NULL || demoFilePath == NULL || cutInfo == NULL || 
	   !IsValid(*cutInfo))
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

	if(info->FileOffset > 0 && file.Seek((s32)info->FileOffset, udtSeekOrigin::Start) != 0)
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	if(!context->Parser.Init(&context->Context, protocol, info->GameStateIndex))
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
			context->Parser.AddCut(info->GameStateIndex, cut.StartTimeMs, cut.EndTimeMs, &CallbackCutDemoFileStreamCreation, &streamInfo);
		}
	}

	context->Context.LogInfo("Processing for a timed cut: %s", demoFilePath);

	if(!RunParser(context->Parser, file, info->CancelOperation))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtCutDemoFileByPattern(udtParserContext* context, const udtParseArg* info, const udtCutByPatternArg* patternInfo, const char* demoFilePath)
{
	if(context == NULL || info == NULL || demoFilePath == NULL || patternInfo == NULL ||
	   !IsValid(*patternInfo))
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

	if(info->FileOffset > 0 && file.Seek((s32)info->FileOffset, udtSeekOrigin::Start) != 0)
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	if(!context->Parser.Init(&context->Context, protocol, info->GameStateIndex))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	CallbackCutDemoFileStreamCreationInfo streamInfo;
	streamInfo.OutputFolderPath = info->OutputFolderPath;

	context->Parser.SetFilePath(demoFilePath);

	context->Context.LogInfo("Processing for cut by pattern: %s", demoFilePath);

	CutByPattern(context, info, patternInfo, demoFilePath);

	if(!RunParser(context->Parser, file, info->CancelOperation))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	return (s32)udtErrorCode::None;
}

UDT_API(udtParserContext*) udtCreateContext()
{
	// @NOTE: We don't use the standard operator new approach to avoid C++ exceptions.
	udtParserContext* const context = (udtParserContext*)malloc(sizeof(udtParserContext));
	if(context == NULL)
	{
		return NULL;
	}

	new (context) udtParserContext;

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
	if(context == NULL || info == NULL || demoFilePath == NULL ||
	   !HasValidPlugInOptions(*info))
	{
		return (s32)udtErrorCode::InvalidArgument;
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

UDT_API(s32) udtParseDemoFiles(udtParserContextGroup** contextGroup, const udtParseArg* info, const udtMultiParseArg* extraInfo)
{
	if(contextGroup == NULL || info == NULL || extraInfo == NULL ||
	   !IsValid(*extraInfo))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	udtDemoThreadAllocator threadAllocator;
	const bool threadJob = threadAllocator.Process(extraInfo->FilePaths, extraInfo->FileCount, extraInfo->MaxThreadCount);
	const u32 threadCount = threadJob ? threadAllocator.Threads.GetSize() : 1;
	if(!CreateContextGroup(contextGroup, threadCount))
	{
		// We must stop here because we can't store the data the user will later want to retrieve.
		return (s32)udtErrorCode::OperationFailed;
	}

	if(!threadJob)
	{
		return udtParseMultipleDemosSingleThread(udtParsingJobType::General, (*contextGroup)->Contexts, info, extraInfo, NULL);
	}
	
	udtMultiThreadedParsing parser;
	const bool success = parser.Process((*contextGroup)->Contexts, threadAllocator, info, extraInfo, udtParsingJobType::General, NULL);

	return GetErrorCode(success, info->CancelOperation);
}

UDT_API(s32) udtCutDemoFilesByPattern(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtCutByPatternArg* patternInfo)
{
	if(info == NULL || extraInfo == NULL || patternInfo == NULL ||
	   !IsValid(*extraInfo) || !IsValid(*patternInfo) || !HasValidOutputOption(*info))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	udtDemoThreadAllocator threadAllocator;
	const bool threadJob = threadAllocator.Process(extraInfo->FilePaths, extraInfo->FileCount, extraInfo->MaxThreadCount);
	if(!threadJob)
	{
		return udtParseMultipleDemosSingleThread(udtParsingJobType::CutByPattern, NULL, info, extraInfo, patternInfo);
	}

	udtParserContextGroup* contextGroup;
	if(!CreateContextGroup(&contextGroup, threadAllocator.Threads.GetSize()))
	{
		return udtParseMultipleDemosSingleThread(udtParsingJobType::CutByPattern, NULL, info, extraInfo, patternInfo);
	}

	udtMultiThreadedParsing parser;
	const bool success = parser.Process(contextGroup->Contexts, threadAllocator, info, extraInfo, udtParsingJobType::CutByPattern, patternInfo);

	DestroyContextGroup(contextGroup);

	return GetErrorCode(success, info->CancelOperation);
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
	if(contextGroup == NULL || context == NULL || 
	   contextIdx >= contextGroup->ContextCount)
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

UDT_API(s32) udtGetDemoInputIndex(udtParserContext* context, u32 demoIdx, u32* demoInputIdx)
{
	if(context == NULL || demoInputIdx == NULL || 
	   demoIdx >= context->GetDemoCount() || demoIdx >= context->InputIndices.GetSize())
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	*demoInputIdx = context->InputIndices[demoIdx];

	return (s32)udtErrorCode::None;
}
