#include "uberdemotools.h"
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
#include "path.hpp"
#include "thread_local_allocators.hpp"
#include "system.hpp"
#include "custom_context.hpp"
#include "pattern_search_context.hpp"

// For malloc and free.
#include <stdlib.h>
#if defined(UDT_MSVC)
#	include <malloc.h>
#endif

// For the placement new operator.
#include <new>


#define UDT_API UDT_API_DEF


static_assert(sizeof(s8) == 1, "sizeof(s8) must be 1");
static_assert(sizeof(u8) == 1, "sizeof(u8) must be 1");
static_assert(sizeof(s16) == 2, "sizeof(s16) must be 2");
static_assert(sizeof(u16) == 2, "sizeof(u16) must be 2");
static_assert(sizeof(s32) == 4, "sizeof(s32) must be 4");
static_assert(sizeof(u32) == 4, "sizeof(u32) must be 4");
static_assert(sizeof(s64) == 8, "sizeof(s64) must be 8");
static_assert(sizeof(u64) == 8, "sizeof(u64) must be 8");
static_assert(sizeof(f32) == 4, "sizeof(f32) must be 4");
static_assert(sizeof(f64) == 8, "sizeof(f64) must be 8");
#if defined(UDT_X64)
static_assert(sizeof(sptr) == 8, "sizeof(sptr) must be 8");
static_assert(sizeof(uptr) == 8, "sizeof(uptr) must be 8");
static_assert(sizeof(void*) == 8, "sizeof(void*) must be 8");
#else
static_assert(sizeof(sptr) == 4, "sizeof(sptr) must be 4");
static_assert(sizeof(uptr) == 4, "sizeof(uptr) must be 4");
static_assert(sizeof(void*) == 4, "sizeof(void*) must be 4");
#endif


#define UDT_ERROR_ITEM(Enum, Desc) Desc,
static const char* ErrorCodeStrings[udtErrorCode::AfterLastError + 1] =
{
	UDT_ERROR_LIST(UDT_ERROR_ITEM)
	"invalid error code"
};
#undef UDT_ERROR_ITEM

#define UDT_PROTOCOL_ITEM(Enum, Ext) Ext,
static const char* DemoFileExtensions[udtProtocol::Count + 1] =
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
	UDT_MEAN_OF_DEATH_LIST(UDT_MOD_ITEM)
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

#define UDT_PATTERN_ITEM(Enum, Desc, ArgType, AnalyzerType) Desc,
static const char* CutPatternNames[] =
{
	UDT_PATTERN_LIST(UDT_PATTERN_ITEM)
	"after last cut pattern"
};
#undef UDT_PATTERN_ITEM

#define UDT_GAME_TYPE_ITEM(Enum, ShortDesc, Desc, Flags) Desc,
static const char* GameTypeNames[] =
{
	UDT_GAME_TYPE_LIST(UDT_GAME_TYPE_ITEM)
	"after last game type"
};
#undef UDT_GAME_TYPE_ITEM

#define UDT_GAME_TYPE_ITEM(Enum, ShortDesc, Desc, Flags) ShortDesc,
static const char* ShortGameTypeNames[] =
{
	UDT_GAME_TYPE_LIST(UDT_GAME_TYPE_ITEM)
	"after last game type"
};
#undef UDT_GAME_TYPE_ITEM

#define UDT_MOD_NAME_ITEM(Enum, Name) Name,
static const char* ModNames[] =
{
	UDT_MOD_NAME_LIST(UDT_MOD_NAME_ITEM)
	"after last mod"
};
#undef UDT_MOD_NAME_ITEM

#define UDT_GAMEPLAY_ITEM(Enum, ShortName, LongName) LongName,
static const char* GamePlayNames[] =
{
	UDT_GAMEPLAY_LIST(UDT_GAMEPLAY_ITEM)
	"after last long gameplay name"
};
#undef UDT_GAMEPLAY_ITEM

#define UDT_GAMEPLAY_ITEM(Enum, ShortName, LongName) ShortName,
static const char* ShortGamePlayNames[] =
{
	UDT_GAMEPLAY_LIST(UDT_GAMEPLAY_ITEM)
	"after last short gameplay name"
};
#undef UDT_GAMEPLAY_ITEM

#define UDT_OVERTIME_TYPE_ITEM(Enum, Name) Name,
static const char* OverTimeTypes[] =
{
	UDT_OVERTIME_TYPE_LIST(UDT_OVERTIME_TYPE_ITEM)
	"after last overtime type"
};
#undef UDT_OVERTIME_TYPE_ITEM

#define UDT_PLAYER_STATS_ITEM(Enum, Desc, Comp, Type) Desc,
static const char* PlayerStatsFieldNames[]
{
	UDT_PLAYER_STATS_LIST(UDT_PLAYER_STATS_ITEM)
	"after last player stats field"
};
#undef UDT_PLAYER_STATS_ITEM

#define UDT_TEAM_STATS_ITEM(Enum, Desc, Comp, Type) Desc,
static const char* TeamStatsFieldNames[]
{
	UDT_TEAM_STATS_LIST(UDT_TEAM_STATS_ITEM)
	"after last team stats field"
};
#undef UDT_TEAM_STATS_ITEM

#define UDT_PLUG_IN_ITEM(Enum, Desc, Type, OutputType) Desc,
static const char* PlugInNamesArray[]
{
	UDT_PLUG_IN_LIST(UDT_PLUG_IN_ITEM)
	"after last plug-in"
};
#undef UDT_PLUG_IN_ITEM

#define UDT_PERF_STATS_ITEM(Enum, Desc, Type) Desc,
static const char* PerfStatsFieldNames[]
{
	UDT_PERF_STATS_LIST(UDT_PERF_STATS_ITEM)
	"after last perf stats field"
};
#undef UDT_PERF_STATS_ITEM

#define UDT_PLAYER_STATS_ITEM(Enum, Desc, Comp, Type) (u8)udtStatsCompMode::Comp,
static const u8 PlayerStatsCompModesArray[]
{
	UDT_PLAYER_STATS_LIST(UDT_PLAYER_STATS_ITEM)
	0
};
#undef UDT_PLAYER_STATS_ITEM

#define UDT_TEAM_STATS_ITEM(Enum, Desc, Comp, Type) (u8)udtStatsCompMode::Comp,
static const u8 TeamStatsCompModesArray[]
{
	UDT_TEAM_STATS_LIST(UDT_TEAM_STATS_ITEM)
	0
};
#undef UDT_TEAM_STATS_ITEM

#define UDT_PLAYER_STATS_ITEM(Enum, Desc, Comp, Type) (u8)udtMatchStatsDataType::Type,
static const u8 PlayerStatsDataTypesArray[]
{
	UDT_PLAYER_STATS_LIST(UDT_PLAYER_STATS_ITEM)
	0
};
#undef UDT_PLAYER_STATS_ITEM

#define UDT_TEAM_STATS_ITEM(Enum, Desc, Comp, Type) (u8)udtMatchStatsDataType::Type,
static const u8 TeamStatsDataTypesArray[]
{
	UDT_TEAM_STATS_LIST(UDT_TEAM_STATS_ITEM)
	0
};
#undef UDT_TEAM_STATS_ITEM

#define UDT_PERF_STATS_ITEM(Enum, Desc, Type) (u8)udtPerfStatsDataType::Type,
static const u8 PerfStatsDataTypesArray[]
{
	UDT_PERF_STATS_LIST(UDT_PERF_STATS_ITEM)
	0
};
#undef UDT_PERF_STATS_ITEM

#define UDT_GAME_TYPE_ITEM(Enum, ShortDesc, Desc, Flags) (u8)(Flags),
static const u8 GameTypeFlagsArray[] =
{
	UDT_GAME_TYPE_LIST(UDT_GAME_TYPE_ITEM)
	0
};
#undef UDT_GAME_TYPE_ITEM


UDT_API(s32) udtGetVersionNumbers(u32* major, u32* minor, u32* revision)
{
	if(major == NULL || minor == NULL || revision == NULL)
	{
		return 0;
	}

	*major = UDT_VERSION_MAJOR;
	*minor = UDT_VERSION_MINOR;
	*revision = UDT_VERSION_REVISION;

	return 1;
}

UDT_API(const char*) udtGetVersionString()
{
	return UDT_VERSION_STRING;
}

UDT_API(const char*) udtGetErrorCodeString(s32 errorCode)
{
	if(errorCode < 0 || errorCode > (s32)udtErrorCode::AfterLastError)
	{
		errorCode = (s32)udtErrorCode::AfterLastError;
	}

	return ErrorCodeStrings[errorCode];
}

UDT_API(s32) udtIsValidProtocol(u32 protocol)
{
	return (protocol >= (u32)udtProtocol::Count) ? 0 : 1;
}

UDT_API(s32) udtIsProtocolWriteSupported(u32 protocol)
{
	if(!udtIsValidProtocol(protocol))
	{
		return 0;
	}

	return protocol >= (u32)udtProtocol::Dm66 ? 1 : 0;
}

UDT_API(u32) udtGetSizeOfIdEntityState(u32 protocol)
{
	switch((udtProtocol::Id)protocol)
	{
		case udtProtocol::Dm3: return (u32)sizeof(idEntityState3);
		case udtProtocol::Dm48: return (u32)sizeof(idEntityState48);
		case udtProtocol::Dm66: return (u32)sizeof(idEntityState66);
		case udtProtocol::Dm67: return (u32)sizeof(idEntityState67);
		case udtProtocol::Dm68: return (u32)sizeof(idEntityState68);
		case udtProtocol::Dm73: return (u32)sizeof(idEntityState73);
		case udtProtocol::Dm90: return (u32)sizeof(idEntityState90);
		case udtProtocol::Dm91: return (u32)sizeof(idEntityState91);
		default: return 0;
	}
}

UDT_API(u32) udtGetSizeOfIdPlayerState(u32 protocol)
{
	switch((udtProtocol::Id)protocol)
	{
		case udtProtocol::Dm3: return (u32)sizeof(idPlayerState3);
		case udtProtocol::Dm48: return (u32)sizeof(idPlayerState48);
		case udtProtocol::Dm66: return (u32)sizeof(idPlayerState66);
		case udtProtocol::Dm67: return (u32)sizeof(idPlayerState67);
		case udtProtocol::Dm68: return (u32)sizeof(idPlayerState68);
		case udtProtocol::Dm73: return (u32)sizeof(idPlayerState73);
		case udtProtocol::Dm90: return (u32)sizeof(idPlayerState90);
		case udtProtocol::Dm91: return (u32)sizeof(idPlayerState91);
		default: return 0;
	}
}

UDT_API(u32) udtGetSizeOfidClientSnapshot(u32 protocol)
{
	switch((udtProtocol::Id)protocol)
	{
		case udtProtocol::Dm3: return (u32)sizeof(idClientSnapshot3);
		case udtProtocol::Dm48: return (u32)sizeof(idClientSnapshot48);
		case udtProtocol::Dm66: return (u32)sizeof(idClientSnapshot66);
		case udtProtocol::Dm67: return (u32)sizeof(idClientSnapshot67);
		case udtProtocol::Dm68: return (u32)sizeof(idClientSnapshot68);
		case udtProtocol::Dm73: return (u32)sizeof(idClientSnapshot73);
		case udtProtocol::Dm90: return (u32)sizeof(idClientSnapshot90);
		case udtProtocol::Dm91: return (u32)sizeof(idClientSnapshot91);
		default: return 0;
	}
}

UDT_API(const char*) udtGetFileExtensionByProtocol(u32 protocol)
{
	if(!udtIsValidProtocol(protocol))
	{
		return NULL;
	}

	return DemoFileExtensions[protocol];
}

UDT_API(u32) udtGetProtocolByFilePath(const char* filePath)
{
	const udtString filePathString = udtString::NewConstRef(filePath);
	for(u32 i = 0; i < (u32)udtProtocol::Count; ++i)
	{
		if(udtString::EndsWithNoCase(filePathString, DemoFileExtensions[i]))
		{
			return i;
		}
	}
	
	return (u32)udtProtocol::Invalid;
}

UDT_API(s32) udtCrash(u32 crashType)
{
	if(crashType >= (u32)udtCrashType::Count)
	{
		return udtErrorCode::InvalidArgument;
	}

	switch((udtCrashType::Id)crashType)
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

UDT_API(s32) udtGetStringArray(u32 arrayId, const char*** elements, u32* elementCount)
{
	if(elements == NULL || elementCount == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	switch((udtStringArray::Id)arrayId)
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

		case udtStringArray::CutPatterns:
			*elements = CutPatternNames;
			*elementCount = (u32)(UDT_COUNT_OF(CutPatternNames) - 1);
			break;

		case udtStringArray::GameTypes:
			*elements = GameTypeNames;
			*elementCount = (u32)(UDT_COUNT_OF(GameTypeNames) - 1);
			break;

		case udtStringArray::ShortGameTypes:
			*elements = ShortGameTypeNames;
			*elementCount = (u32)(UDT_COUNT_OF(ShortGameTypeNames) - 1);
			break;

		case udtStringArray::ModNames:
			*elements = ModNames;
			*elementCount = (u32)(UDT_COUNT_OF(ModNames) - 1);
			break;

		case udtStringArray::GamePlayNames:
			*elements = GamePlayNames;
			*elementCount = (u32)(UDT_COUNT_OF(GamePlayNames) - 1);
			break;

		case udtStringArray::ShortGamePlayNames:
			*elements = ShortGamePlayNames;
			*elementCount = (u32)(UDT_COUNT_OF(ShortGamePlayNames) - 1);
			break;

		case udtStringArray::OverTimeTypes:
			*elements = OverTimeTypes;
			*elementCount = (u32)(UDT_COUNT_OF(OverTimeTypes) - 1);
			break;

		case udtStringArray::TeamStatsNames:
			*elements = TeamStatsFieldNames;
			*elementCount = (u32)(UDT_COUNT_OF(TeamStatsFieldNames) - 1);
			break;

		case udtStringArray::PlayerStatsNames:
			*elements = PlayerStatsFieldNames;
			*elementCount = (u32)(UDT_COUNT_OF(PlayerStatsFieldNames) - 1);
			break;

		case udtStringArray::PlugInNames:
			*elements = PlugInNamesArray;
			*elementCount = (u32)(UDT_COUNT_OF(PlugInNamesArray) - 1);
			break;

		case udtStringArray::PerfStatsNames:
			*elements = PerfStatsFieldNames;
			*elementCount = (u32)(UDT_COUNT_OF(PerfStatsFieldNames) - 1);
			break;

		default:
			return (s32)udtErrorCode::InvalidArgument;
	}

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtGetByteArray(u32 arrayId, const u8** elements, u32* elementCount)
{
	if(elements == NULL || elementCount == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	switch((udtByteArray::Id)arrayId)
	{
		case udtByteArray::TeamStatsCompModes:
			*elements = TeamStatsCompModesArray;
			*elementCount = (u32)(UDT_COUNT_OF(TeamStatsCompModesArray) - 1);
			break;

		case udtByteArray::PlayerStatsCompModes:
			*elements = PlayerStatsCompModesArray;
			*elementCount = (u32)(UDT_COUNT_OF(PlayerStatsCompModesArray) - 1);
			break;

		case udtByteArray::TeamStatsDataTypes:
			*elements = TeamStatsDataTypesArray;
			*elementCount = (u32)(UDT_COUNT_OF(TeamStatsDataTypesArray) - 1);
			break;

		case udtByteArray::PlayerStatsDataTypes:
			*elements = PlayerStatsDataTypesArray;
			*elementCount = (u32)(UDT_COUNT_OF(PlayerStatsDataTypesArray) - 1);
			break;

		case udtByteArray::PerfStatsDataTypes:
			*elements = PerfStatsDataTypesArray;
			*elementCount = (u32)(UDT_COUNT_OF(PerfStatsDataTypesArray) - 1);
			break;

		case udtByteArray::GameTypeFlags:
			*elements = GameTypeFlagsArray;
			*elementCount = (u32)(UDT_COUNT_OF(GameTypeFlagsArray) - 1);
			break;

		default:
			return (s32)udtErrorCode::InvalidArgument;
	}

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtGetStatsConstants(u32* playerMaskByteCount, u32* teamMaskByteCount, u32* playerFieldCount, u32* teamFieldCount, u32* perfFieldCount)
{
	if(playerMaskByteCount == NULL ||
	   teamMaskByteCount == NULL ||
	   playerFieldCount == NULL ||
	   teamFieldCount == NULL ||
	   perfFieldCount == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	*playerMaskByteCount = UDT_PLAYER_STATS_MASK_BYTE_COUNT;
	*teamMaskByteCount = UDT_TEAM_STATS_MASK_BYTE_COUNT;
	*playerFieldCount = (u32)udtPlayerStatsField::Count;
	*teamFieldCount = (u32)udtTeamStatsField::Count;
	*perfFieldCount = (u32)udtPerfStatsField::Count;

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtMergeBatchPerfStats(u64* destPerfStats, const u64* sourcePerfStats)
{
	if(destPerfStats == NULL || sourcePerfStats == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	destPerfStats[udtPerfStatsField::ThreadCount] = sourcePerfStats[udtPerfStatsField::ThreadCount];
	destPerfStats[udtPerfStatsField::AllocatorCount] = sourcePerfStats[udtPerfStatsField::AllocatorCount];
	destPerfStats[udtPerfStatsField::DataProcessed] += sourcePerfStats[udtPerfStatsField::DataProcessed];
	destPerfStats[udtPerfStatsField::Duration] += sourcePerfStats[udtPerfStatsField::Duration];
	destPerfStats[udtPerfStatsField::DataThroughput] = (destPerfStats[udtPerfStatsField::Duration] > 0) ?
		((1000000 * destPerfStats[udtPerfStatsField::DataProcessed]) / destPerfStats[udtPerfStatsField::Duration]) : 0;

	if(sourcePerfStats[udtPerfStatsField::MemoryReserved] > destPerfStats[udtPerfStatsField::MemoryReserved])
	{
		destPerfStats[udtPerfStatsField::MemoryReserved] = sourcePerfStats[udtPerfStatsField::MemoryReserved];
		destPerfStats[udtPerfStatsField::MemoryCommitted] = sourcePerfStats[udtPerfStatsField::MemoryCommitted];
		destPerfStats[udtPerfStatsField::MemoryUsed] = sourcePerfStats[udtPerfStatsField::MemoryUsed];
		destPerfStats[udtPerfStatsField::MemoryEfficiency] = (destPerfStats[udtPerfStatsField::MemoryCommitted] > 0) ?
			((1000 * destPerfStats[udtPerfStatsField::MemoryUsed]) / destPerfStats[udtPerfStatsField::MemoryCommitted]) : 0;
	}

	destPerfStats[udtPerfStatsField::ResizeCount] += sourcePerfStats[udtPerfStatsField::ResizeCount];

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtAddThreadPerfStats(u64* destPerfStats, const u64* sourcePerfStats)
{
	if(destPerfStats == NULL || sourcePerfStats == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	destPerfStats[udtPerfStatsField::ThreadCount] += sourcePerfStats[udtPerfStatsField::ThreadCount];
	destPerfStats[udtPerfStatsField::AllocatorCount] += sourcePerfStats[udtPerfStatsField::AllocatorCount];
	destPerfStats[udtPerfStatsField::DataProcessed] += sourcePerfStats[udtPerfStatsField::DataProcessed];
	destPerfStats[udtPerfStatsField::Duration] = udt_max(destPerfStats[udtPerfStatsField::Duration], sourcePerfStats[udtPerfStatsField::Duration]);
	destPerfStats[udtPerfStatsField::MemoryReserved] += sourcePerfStats[udtPerfStatsField::MemoryReserved];
	destPerfStats[udtPerfStatsField::MemoryCommitted] += sourcePerfStats[udtPerfStatsField::MemoryCommitted];
	destPerfStats[udtPerfStatsField::MemoryUsed] += sourcePerfStats[udtPerfStatsField::MemoryUsed];
	destPerfStats[udtPerfStatsField::ResizeCount] += sourcePerfStats[udtPerfStatsField::ResizeCount];
	destPerfStats[udtPerfStatsField::DataThroughput] = (destPerfStats[udtPerfStatsField::Duration] > 0) ?
		((1000000 * destPerfStats[udtPerfStatsField::DataProcessed]) / destPerfStats[udtPerfStatsField::Duration]) : 0;
	destPerfStats[udtPerfStatsField::MemoryEfficiency] = (destPerfStats[udtPerfStatsField::MemoryCommitted] > 0) ?
		((1000 * destPerfStats[udtPerfStatsField::MemoryUsed]) / destPerfStats[udtPerfStatsField::MemoryCommitted]) : 0;

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtGetProcessorCoreCount(u32* cpuCoreCount)
{
	if(cpuCoreCount == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	u32 count;
	if(!GetProcessorCoreCount(count))
	{
		return (s32)udtErrorCode::OperationFailed;
	}
	
	*cpuCoreCount = count;

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtInitLibrary()
{
	udtThreadLocalAllocators::Init();
	BuildLookUpTables();

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtShutDownLibrary()
{
	udtThreadLocalAllocators::Destroy();

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtSetCrashHandler(udtCrashCallback crashHandler)
{
	SetCrashHandler(crashHandler);

	return (s32)udtErrorCode::None;
}

static bool CreateDemoFileSplit(udtVMLinearAllocator& tempAllocator, udtContext& context, udtStream& file, const char* filePath, const char* outputFolderPath, u32 index, u32 startOffset, u32 endOffset)
{
	if(endOffset <= startOffset)
	{
		return false;
	}

	if(file.Seek((s32)startOffset, udtSeekOrigin::Start) != 0)
	{
		return false;
	}

	const udtProtocol::Id protocol = (udtProtocol::Id)udtGetProtocolByFilePath(filePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	udtVMScopedStackAllocator scopedTempAllocator(tempAllocator);

	const udtString filePathString = udtString::NewConstRef(filePath);

	udtString fileName;
	if(!udtPath::GetFileNameWithoutExtension(fileName, tempAllocator, filePathString))
	{
		fileName = udtString::NewConstRef("NEW_UDT_SPLIT_DEMO");
	}
	
	udtString outputFilePathStart;
	if(outputFolderPath == NULL)
	{
		udtString inputFolderPath;
		udtPath::GetFolderPath(inputFolderPath, tempAllocator, filePathString);
		udtPath::Combine(outputFilePathStart, tempAllocator, inputFolderPath, fileName);
	}
	else
	{
		udtPath::Combine(outputFilePathStart, tempAllocator, udtString::NewConstRef(outputFolderPath), fileName);
	}

	udtString newFilePath = udtString::NewEmpty(tempAllocator, UDT_MAX_PATH_LENGTH);
	sprintf(newFilePath.GetWritePtr(), "%s_SPLIT_%u%s", outputFilePathStart.GetPtr(), index + 1, udtGetFileExtensionByProtocol((u32)protocol));

	context.LogInfo("Writing demo %s...", newFilePath.GetPtr());

	udtFileStream outputFile;
	if(!outputFile.Open(newFilePath.GetPtr(), udtFileOpenMode::Write))
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

static bool CreateDemoFileSplit(udtVMLinearAllocator& tempAllocator, udtContext& context, udtStream& file, const char* filePath, const char* outputFolderPath, const u32* fileOffsets, const u32 count)
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

		success = success && CreateDemoFileSplit(tempAllocator, context, file, filePath, outputFolderPath, i - indexOffset, start, end);

		start = end;
	}

	end = fileLength;
	success = success && CreateDemoFileSplit(tempAllocator, context, file, filePath, outputFolderPath, count - indexOffset, start, end);

	return success;
}

UDT_API(s32) udtSplitDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath)
{
	if(context == NULL || info == NULL || demoFilePath == NULL ||
	   !HasValidOutputOption(*info))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	const udtProtocol::Id protocol = (udtProtocol::Id)udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	context->ResetForNextDemo(false);

	if(!context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	udtFileStream file;
	if(!file.Open(demoFilePath, udtFileOpenMode::Read))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	if(!context->Parser.Init(&context->Context, protocol, protocol))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	context->Parser.SetFilePath(demoFilePath);

	// TODO: Move this to api_helpers.cpp and implement it the same way Cut by Pattern is?
	udtParserPlugInSplitter plugIn;
	plugIn.Init(1, context->PlugInTempAllocator);
	context->Parser.AddPlugIn(&plugIn);
	if(!RunParser(context->Parser, file, info->CancelOperation))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	if(plugIn.GamestateFileOffsets.GetSize() <= 1)
	{
		return (s32)udtErrorCode::None;
	}

	udtVMLinearAllocator& tempAllocator = context->Parser._tempAllocator;
	tempAllocator.Clear();
	if(!CreateDemoFileSplit(tempAllocator, context->Context, file, demoFilePath, info->OutputFolderPath, &plugIn.GamestateFileOffsets[0], plugIn.GamestateFileOffsets.GetSize()))
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

	const udtProtocol::Id protocol = (udtProtocol::Id)udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	udtTimer progressTimer;
	progressTimer.Start();

	SingleThreadProgressContext progressContext;
	progressContext.Timer = &progressTimer;
	progressContext.UserCallback = info->ProgressCb;
	progressContext.UserData = info->ProgressContext;
	progressContext.CurrentJobByteCount = 0;
	progressContext.ProcessedByteCount = 0;
	progressContext.TotalByteCount = udtFileStream::GetFileLength(demoFilePath);
	progressContext.MinProgressTimeMs = info->MinProgressTimeMs;

	context->ResetForNextDemo(false);
	if(!context->Context.SetCallbacks(info->MessageCb, &SingleThreadProgressCallback, &progressContext))
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

	if(!context->Parser.Init(&context->Context, protocol, protocol, info->GameStateIndex))
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
			if(cut.FilePath != NULL)
			{
				context->Parser.AddCut(info->GameStateIndex, cut.StartTimeMs, cut.EndTimeMs, cut.FilePath);
			}
			else
			{
				context->Parser.AddCut(info->GameStateIndex, cut.StartTimeMs, cut.EndTimeMs, &CallbackCutDemoFileNameCreation, NULL, &streamInfo);
			}
		}
	}

	context->Context.LogInfo("Processing for a timed cut: %s", demoFilePath);

	if(!RunParser(context->Parser, file, info->CancelOperation))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtMergeDemoFiles(const udtParseArg* info, const char** filePaths, u32 fileCount)
{
	if(info == NULL || filePaths == NULL || fileCount == 0 ||
	   !HasValidOutputOption(*info))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	fileCount = udt_min(fileCount, (u32)UDT_MAX_MERGE_DEMO_COUNT);

	for(u32 i = 0; i < fileCount; ++i)
	{
		if(filePaths[i] == NULL)
		{
			return (s32)udtErrorCode::InvalidArgument;
		}
	}

	const udtProtocol::Id protocol = (udtProtocol::Id)udtGetProtocolByFilePath(filePaths[0]);
	if(protocol == udtProtocol::Invalid)
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	// Make sure we're not trying to merge demos with different protocols.
	for(u32 i = 1; i < fileCount; ++i)
	{
		const udtProtocol::Id tempProtocol = (udtProtocol::Id)udtGetProtocolByFilePath(filePaths[i]);
		if(tempProtocol != protocol)
		{
			return (s32)udtErrorCode::InvalidArgument;
		}
	}

	if(!MergeDemosNoInputCheck(info, filePaths, fileCount, protocol))
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

UDT_API(s32) udtGetContextPlugInBuffers(udtParserContext* context, u32 plugInId, void* buffersStruct)
{
	if(context == NULL || plugInId >= (u32)udtParserPlugIn::Count || buffersStruct == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	if(!context->CopyBuffersStruct(plugInId, buffersStruct))
	{
		return udtErrorCode::OperationFailed;
	}

	return (s32)udtErrorCode::None;
}

struct udtParserContextGroup_s
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

	free(contextGroup);
}

static s32 RunJobWithLocalContextGroup(udtParsingJobType::Id jobType, const udtParseArg* info, const udtMultiParseArg* extraInfo, const void* jobSpecificArg)
{
	udtTimer jobTimer;
	jobTimer.Start();

	udtDemoThreadAllocator threadAllocator;
	const bool threadJob = threadAllocator.Process(extraInfo->FilePaths, extraInfo->FileCount, extraInfo->MaxThreadCount);
	if(!threadJob)
	{
		return udtParseMultipleDemosSingleThread(jobType, NULL, info, extraInfo, jobSpecificArg);
	}

	udtParserContextGroup* contextGroup;
	if(!CreateContextGroup(&contextGroup, threadAllocator.Threads.GetSize()))
	{
		return udtParseMultipleDemosSingleThread(jobType, NULL, info, extraInfo, jobSpecificArg);
	}

	udtMultiThreadedParsing parser;
	const bool success = parser.Process(jobTimer, contextGroup->Contexts, threadAllocator, info, extraInfo, jobType, jobSpecificArg);

	DestroyContextGroup(contextGroup);

	return GetErrorCode(success, info->CancelOperation);
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
	   !IsValid(*extraInfo) || !HasValidPlugInOptions(*info))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	udtTimer jobTimer;
	jobTimer.Start();

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
	const bool success = parser.Process(jobTimer, (*contextGroup)->Contexts, threadAllocator, info, extraInfo, udtParsingJobType::General, NULL);

	return GetErrorCode(success, info->CancelOperation);
}

UDT_API(s32) udtCutDemoFilesByPattern(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtPatternSearchArg* patternInfo)
{
	if(info == NULL || extraInfo == NULL || patternInfo == NULL ||
	   !IsValid(*extraInfo) || !IsValid(*patternInfo) || !HasValidOutputOption(*info))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	return RunJobWithLocalContextGroup(udtParsingJobType::CutByPattern, info, extraInfo, patternInfo);
}

UDT_API(s32) udtFindPatternsInDemoFiles(udtPatternSearchContext** contextPtr, const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtPatternSearchArg* patternInfo)
{
	if(contextPtr == NULL || info == NULL || extraInfo == NULL || patternInfo == NULL ||
	   !IsValid(*extraInfo) || !IsValid(*patternInfo))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	udtPatternSearchContext_s* context = (udtPatternSearchContext_s*)malloc(sizeof(udtPatternSearchContext_s));
	if(context == NULL)
	{
		return (s32)udtErrorCode::OperationFailed;
	}
	new (context) udtPatternSearchContext_s(patternInfo);
	
	const s32 result = RunJobWithLocalContextGroup(udtParsingJobType::FindPatterns, info, extraInfo, context);
	if(result == (s32)udtErrorCode::None || 
	   result == (s32)udtErrorCode::OperationCanceled)
	{
		*contextPtr = context;
	}

	return result;
}

UDT_API(s32) udtGetSearchResults(udtPatternSearchContext* context, udtPatternSearchResults* results)
{
	if(context == NULL || results == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	results->Matches = context->Matches.GetStartAddress();
	results->MatchCount = context->Matches.GetSize();

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtDestroySearchContext(udtPatternSearchContext* context)
{
	if(context == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	context->~udtPatternSearchContext_s();
	free(context);

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtConvertDemoFiles(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtProtocolConversionArg* conversionArg)
{
	if(info == NULL || extraInfo == NULL || conversionArg == NULL ||
	   !IsValid(*extraInfo) || !HasValidOutputOption(*info) || !IsValid(*conversionArg))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	return RunJobWithLocalContextGroup(udtParsingJobType::Conversion, info, extraInfo, conversionArg);
}

UDT_API(s32) udtTimeShiftDemoFiles(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtTimeShiftArg* timeShiftArg)
{
	if(info == NULL || extraInfo == NULL || timeShiftArg == NULL ||
	   !IsValid(*extraInfo) || !HasValidOutputOption(*info))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	return RunJobWithLocalContextGroup(udtParsingJobType::TimeShift, info, extraInfo, timeShiftArg);
}

UDT_API(s32) udtSaveDemoFilesAnalysisDataToJSON(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtJSONArg* jsonInfo)
{
	if(info == NULL || extraInfo == NULL || jsonInfo == NULL ||
	   !IsValid(*extraInfo) || !HasValidOutputOption(*info) || !HasValidPlugInOptions(*info))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	return RunJobWithLocalContextGroup(udtParsingJobType::ExportToJSON, info, extraInfo, jsonInfo);
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

UDT_API(udtCuContext*) udtCuCreateContext()
{
	// @NOTE: We don't use the standard operator new approach to avoid C++ exceptions.
	udtCuContext* const context = (udtCuContext*)malloc(sizeof(udtCuContext));
	if(context == NULL)
	{
		return NULL;
	}

	new (context) udtCuContext;

	if(!context->Context.Init(1, NULL, 0))
	{
		udtCuDestroyContext(context);
		return NULL;
	}

	context->PlugIn.InitAllocators(0);

	return context;
}

UDT_API(s32) udtCuSetMessageCallback(udtCuContext* context, udtMessageCallback callback)
{
	if(context == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	if(!context->Context.Context.SetCallbacks(callback, NULL, NULL))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtCuStartParsing(udtCuContext* context, u32 protocol)
{
	if(context == NULL || udtIsValidProtocol(protocol) == 0)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	const udtProtocol::Id protocolId = (udtProtocol::Id)protocol;
	context->Context.ResetForNextDemo(false);
	udtMessage& message = context->InMessage;
	message.InitContext(&context->Context.Context);
	message.InitProtocol(protocolId);
	if(!context->Context.Parser.Init(&context->Context.Context, protocolId, protocolId, 0, true))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtCuParseMessage(udtCuContext* context, udtCuMessageOutput* messageOutput, u32* continueParsing, const udtCuMessageInput* messageInput)
{
	if(context == NULL || messageOutput == NULL || continueParsing == NULL || messageInput == NULL ||
	   messageInput->Buffer == NULL || messageInput->BufferByteCount == 0)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	udtMessage& message = context->InMessage;
	message.Init((u8*)messageInput->Buffer, ID_MAX_MSG_LENGTH);
	message.Buffer.cursize = (s32)messageInput->BufferByteCount;
	context->Context.Parser.PlugIns.Clear();
	context->Context.Parser.PlugIns.Add(&context->PlugIn);
	const bool cont = context->Context.Parser.ParseNextMessage(message, messageInput->MessageSequence, 0);
	*messageOutput = context->Message;
	*continueParsing = cont ? 1 : 0;

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtCuGetConfigString(udtCuContext* context, udtCuConfigString* configString, u32 configStringIndex)
{
	if(context == NULL || configString == NULL || configStringIndex >= (u32)MAX_CONFIGSTRINGS)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	const udtString cs = context->Context.Parser._inConfigStrings[configStringIndex];
	configString->ConfigString = cs.GetPtr();
	configString->ConfigStringLength = cs.GetLength();

	return (s32)udtErrorCode::None;
}

static s32 GetEntity(udtCuContext* context, idEntityStateBase** entityState, u32 entityIndex, bool baseLine)
{
	if(context == NULL || entityState == NULL || entityIndex >= (u32)ID_MAX_PARSE_ENTITIES)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	if(baseLine)
	{
		*entityState = context->Context.Parser.GetBaseline((s32)entityIndex);
	}
	else
	{
		*entityState = context->Context.Parser.GetEntity((s32)entityIndex);
	}

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtCuGetEntityBaseline(udtCuContext* context, idEntityStateBase** entityState, u32 entityIndex)
{
	return GetEntity(context, entityState, entityIndex, true);
}

UDT_API(s32) udtCuGetEntityState(udtCuContext* context, idEntityStateBase** entityState, u32 entityIndex)
{
	return GetEntity(context, entityState, entityIndex, false);
}

UDT_API(s32) udtCuDestroyContext(udtCuContext* context)
{
	if(context == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	// @NOTE: We don't use the standard operator new approach to avoid C++ exceptions.
	context->~udtCuContext();
	free(context);

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtCleanUpString(char* string, u32 protocol)
{
	if(string == NULL || !udtIsValidProtocol(protocol))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	CleanUpString(string, (udtProtocol::Id)protocol);

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtGetIdMagicNumber(s32* idNumber, u32 magicNumberTypeId, s32 udtNumber, u32 protocol, u32 mod)
{
	if(idNumber == NULL || 
	   magicNumberTypeId >= (u32)udtMagicNumberType::Count || 
	   mod >= (u32)udtMod::Count ||
	   !udtIsValidProtocol(protocol))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	if(!GetIdNumber(*idNumber, (udtMagicNumberType::Id)magicNumberTypeId, (u32)udtNumber, (udtProtocol::Id)protocol, (udtMod::Id)mod))
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtGetUDTMagicNumber(s32* udtNumber, u32 magicNumberTypeId, s32 idNumber, u32 protocol, u32 mod)
{
	if(udtNumber == NULL || 
	   magicNumberTypeId >= (u32)udtMagicNumberType::Count || 
	   mod >= (u32)udtMod::Count ||
	   !udtIsValidProtocol(protocol))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	u32 result;
	if(!GetUDTNumber(result, (udtMagicNumberType::Id)magicNumberTypeId, (s32)idNumber, (udtProtocol::Id)protocol, (udtMod::Id)mod))
	{
		return (s32)udtErrorCode::OperationFailed;
	}
	*udtNumber = result;

	return (s32)udtErrorCode::None;
}

static const char* FindConfigStringValueAddress(bool& bufferTooSmall, char* tempBuf, u32 tempBytes, const char* variableName, const char* configString)
{
	bufferTooSmall = false;

	const u32 nameLength = (u32)strlen(variableName);
	const u32 csLength = (u32)strlen(configString);
	if(csLength > nameLength && 
	   strstr(configString, variableName) == configString && 
	   configString[nameLength] == '\\')
	{
		// Variable found at the start.
		return configString + nameLength + 1;
	}

	const u32 patternLength = nameLength + 2;
	const u32 patternByteCount = patternLength + 1;
	if(tempBytes < patternByteCount)
	{
		// The buffer's not large enough.
		bufferTooSmall = true;
		return NULL;
	}

	sprintf(tempBuf, "\\%s\\", variableName);
	const char* const keySepAddress = strstr(configString, tempBuf);
	if(keySepAddress == NULL)
	{
		// Variable not found.
		return NULL;
	}

	return keySepAddress + patternLength;
}

UDT_API(s32) udtParseConfigStringValueAsInteger(s32* res, char* tempBuf, u32 tempBytes, const char* varName, const char* configString)
{
	if(res == NULL || tempBuf == NULL || tempBytes == 0 || varName == NULL || configString == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	bool bufferTooSmall = false;
	const char* const valueAddress = FindConfigStringValueAddress(bufferTooSmall, tempBuf, tempBytes, varName, configString);
	if(bufferTooSmall)
	{
		return (s32)udtErrorCode::InsufficientBufferSize;
	}
	else if(valueAddress == NULL)
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	int result = 0;
	if(sscanf(valueAddress, "%d", &result) != 1)
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	*res = (s32)result;

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtParseConfigStringValueAsString(char* resBuf, u32 resBytes, char* tempBuf, u32 tempBytes, const char* varName, const char* configString)
{
	if(resBuf == NULL || resBytes == 0 || tempBuf == NULL || tempBytes == 0 || varName == NULL || configString == NULL)
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	bool bufferTooSmall = false;
	const char* const valueAddress = FindConfigStringValueAddress(bufferTooSmall, tempBuf, tempBytes, varName, configString);
	if(bufferTooSmall)
	{
		return (s32)udtErrorCode::InsufficientBufferSize;
	}
	else if(valueAddress == NULL)
	{
		return (s32)udtErrorCode::OperationFailed;
	}

	const char* const sepAfterValue = strchr(valueAddress, '\\');
	if(sepAfterValue == NULL)
	{
		const u32 length = (u32)strlen(valueAddress);
		if(resBytes < length + 1)
		{
			return (s32)udtErrorCode::InsufficientBufferSize;
		}
		strcpy(resBuf, valueAddress);
	}
	else
	{
		const u32 length = (u32)(sepAfterValue - valueAddress);
		if(resBytes < length + 1)
		{
			return (s32)udtErrorCode::InsufficientBufferSize;
		}
		memcpy(resBuf, valueAddress, length);
		resBuf[length] = '\0';
	}

	return (s32)udtErrorCode::None;
}

UDT_API(s32) udtPlayerStateToEntityState(idEntityStateBase* es, const idPlayerStateBase* ps, u32 extrapolate, s32 serverTimeMs, u32 protocol)
{
	if(es == NULL || ps == NULL || !udtIsValidProtocol(protocol))
	{
		return (s32)udtErrorCode::InvalidArgument;
	}

	// @TODO: deal with the lastEventSequence business.
	s32 dummy = 0;
	PlayerStateToEntityState(*es, dummy, *ps, extrapolate != 0, serverTimeMs, (udtProtocol::Id)protocol);

	return (s32)udtErrorCode::None;
}
