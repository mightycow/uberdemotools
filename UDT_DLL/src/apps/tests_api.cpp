#include "tests.hpp"
#include "uberdemotools.h"
#include "utils.hpp"
#include "path.hpp"
#include <stdio.h>

#if 1

UDT_TEST_FILE("API");

static void CallbackConsoleMessage(s32 logLevel, const char* message)
{
	const char* logLevels[4] =
	{
		"",
		"Warning: ",
		"Error: ",
		"Fatal: "
	};

	if(logLevel < 0 || logLevel >= 3)
	{
		logLevel = 3;
	}

	const bool useStdOut = logLevel == 0 || logLevel == 1;
	fprintf(useStdOut ? stdout : stderr, "%s%s\n", logLevels[logLevel], message);
}

static bool KeepOnlyDemoFiles(const char* name, u64 /*size*/, void* /*userData*/)
{
	return udtPath::HasValidDemoFileExtension(name);
}

struct Cut
{
	Cut(const char* start, const char* end)
	{
		StringParseSeconds(StartTimeSec, start);
		StringParseSeconds(EndTimeSec, end);
		Found = 0;
	}

	s32 StartTimeSec;
	s32 EndTimeSec;
	s32 Found;
};

static bool ValidateDemoCutRange(const char* demoPath, Cut* cuts, u32 cutCount)
{
	const u32 plugIns[] = { udtParserPlugIn::GameState };

	udtParseArg parse = {};
	parse.MessageCb = &CallbackConsoleMessage;
	parse.PlugInCount = UDT_COUNT_OF(plugIns);
	parse.PlugIns = plugIns;
	parse.OutputFolderPath = ctx.OutTempDir.Path;

	s32 errorCode = 0;
	udtMultiParseArg multiParse = {};
	multiParse.FileCount = 1;
	multiParse.FilePaths = &demoPath;
	multiParse.OutputErrorCodes = &errorCode;
	multiParse.MaxThreadCount = 1;

	UDT_ENSURE(udtInitLibrary() == (s32)udtErrorCode::None);

	udtParserContextGroup* contextGroup = NULL;
	UDT_ENSURE(udtParseDemoFiles(&contextGroup, &parse, &multiParse) == (s32)udtErrorCode::None);
	UDT_ENSURE(contextGroup != NULL);

	u32 contextCount = 0;
	UDT_ENSURE(udtGetContextCountFromGroup(contextGroup, &contextCount) == (s32)udtErrorCode::None);
	UDT_ENSURE(contextCount == 1);

	udtParserContext* context = NULL;
	UDT_ENSURE(udtGetContextFromGroup(contextGroup, 0, &context) == (s32)udtErrorCode::None);
	UDT_ENSURE(context != NULL);

	u32 contextDemoCount = 0;
	UDT_ENSURE(udtGetDemoCountFromContext(context, &contextDemoCount) == (s32)udtErrorCode::None);
	UDT_ENSURE(contextDemoCount == 1);

	udtParseDataGameStateBuffers buffers = {};
	UDT_ENSURE(udtGetContextPlugInBuffers(context, (u32)udtParserPlugIn::GameState, &buffers) == (s32)udtErrorCode::None);
	UDT_ENSURE(buffers.GameStateCount >= 1);

	const udtParseDataBufferRange range = buffers.GameStateRanges[0];
	UDT_ENSURE(range.Count >= 1);

	const udtParseDataGameState gs = buffers.GameStates[range.FirstIndex];
	const s32 firstSec = gs.FirstSnapshotTimeMs / 1000;
	const s32 lastSec = gs.LastSnapshotTimeMs / 1000;
	for(u32 i = 0; i < cutCount; ++i)
	{
		Cut& cut = cuts[i];
		if(cut.StartTimeSec == firstSec &&
			(cut.EndTimeSec == lastSec || cut.EndTimeSec - 1 == lastSec))
		{
			cut.Found++;
		}
	}
	
	UDT_ENSURE(udtDestroyContextGroup(contextGroup) == (s32)udtErrorCode::None);
	UDT_ENSURE(udtShutDownLibrary() == (s32)udtErrorCode::None);

	return true;
}

static bool ValidateDemoCutRanges(const udtVMArray<udtFileInfo>& files, Cut* cuts, u32 cutCount)
{
	for(u32 i = 0; i < cutCount; ++i)
	{
		cuts[i].Found = 0;
	}

	for(u32 i = 0, count = files.GetSize(); i < count; ++i)
	{
		const udtString path = files[i].Path;
		ValidateDemoCutRange(path.GetPtrOrEmpty(), cuts, cutCount);
	}

	bool success = true;
	for(u32 i = 0; i < cutCount; ++i)
	{
		UDT_ENSURE(cuts[i].Found == 1);
		if(cuts[i].Found != 1)
		{
			success = false;
		}
	}

	return success;
}

static bool TestCorruptDemos(u32 maxThreadCount)
{
	udtVMLinearAllocator pathAlloc("PathAllocator");
	pathAlloc.Init(UDT_KB(64));

	struct DemoInfo
	{
		const char* relativePath;
		s32 firstTimeMs;
		s32 lastTimeMs;
	};

	const DemoInfo demos[] =
	{
		{ "invalid_area_mask_length.dm_68", 33037, 33169 }, // 0:33 -> 0:33
		{ "invalid_command_byte.dm_68", UDT_S32_MAX, UDT_S32_MIN }, // corrupted gamestate... should be dropped!
		{ "invalid_command_byte_2.dm_68", 29968, 30034 }, // 0:29 -> 0:30
		{ "invalid_entity_state_field_count.dm_68", 32575, 44488 }, // 0:32 -> 0:44
		{ "invalid_entity_state_field_count_2.dm_68", 54652, 67555 }, // 0:54 -> 1:07
		{ "truncated.dm_68", 41716, 49504 }, // 0:41 -> 0:49
		{ "truncated.dm_73", 897175, 1222500 } // 14:57 -> 20:22
	};
	const constexpr u32 demoCount = (u32)UDT_COUNT_OF(demos);
	udtString fullDemoStrings[demoCount] = {};
	const char* fullDemoPaths[demoCount] = {};
	s32 errorCodes[demoCount] = {};

	for(u32 i = 0; i < demoCount; ++i)
	{
		char path[512];
		sprintf(path, "%s/%s", ctx.DemoDir, demos[i].relativePath);
		fullDemoStrings[i] = udtString::NewClone(pathAlloc, path);
	}

	// only grab pointers when all allocations are done since relocations can happen
	for(u32 i = 0; i < demoCount; ++i)
	{
		fullDemoPaths[i] = fullDemoStrings[i].GetPtrOrEmpty();
	}

	const u32 plugIns[] = { udtParserPlugIn::GameState };

	udtParseArg parse = {};
	parse.MessageCb = &CallbackConsoleMessage;
	parse.PlugInCount = UDT_COUNT_OF(plugIns);
	parse.PlugIns = plugIns;
	parse.OutputFolderPath = ctx.OutTempDir.Path;

	udtMultiParseArg multiParse = {};
	multiParse.FileCount = demoCount;
	multiParse.FilePaths = fullDemoPaths;
	multiParse.OutputErrorCodes = errorCodes;
	multiParse.MaxThreadCount = maxThreadCount;

	udtParserContextGroup* contextGroup = NULL;
	UDT_ENSURE(udtInitLibrary() == (s32)udtErrorCode::None);
	UDT_ENSURE(udtParseDemoFiles(&contextGroup, &parse, &multiParse) == (s32)udtErrorCode::None);
	UDT_ENSURE(contextGroup != NULL);
	u32 contextCount = 0;
	UDT_ENSURE(udtGetContextCountFromGroup(contextGroup, &contextCount) == (s32)udtErrorCode::None);
	if(maxThreadCount == 1)
	{
		// we asked for single-threaded mode
		UDT_ENSURE(contextCount == 1);
	}

	u32 udtDemoCount = 0;
	for(u32 contextIdx = 0; contextIdx < contextCount; ++contextIdx)
	{
		udtParserContext* context = NULL;
		UDT_ENSURE(udtGetContextFromGroup(contextGroup, contextIdx, &context) == (s32)udtErrorCode::None);
		UDT_ENSURE(context != NULL);

		u32 contextDemoCount = 0;
		UDT_ENSURE(udtGetDemoCountFromContext(context, &contextDemoCount) == (s32)udtErrorCode::None);

		udtDemoCount += contextDemoCount;
	}
	UDT_ENSURE(udtDemoCount == demoCount);
	
	for(u32 contextIdx = 0; contextIdx < contextCount; ++contextIdx)
	{
		udtParserContext* context = NULL;
		UDT_ENSURE(udtGetContextFromGroup(contextGroup, contextIdx, &context) == (s32)udtErrorCode::None);
		UDT_ENSURE(context != NULL);

		u32 contextDemoCount = 0;
		UDT_ENSURE(udtGetDemoCountFromContext(context, &contextDemoCount) == (s32)udtErrorCode::None);
		UDT_ENSURE(contextDemoCount > 0 && contextDemoCount <= demoCount);

		udtParseDataGameStateBuffers buffers = {};
		UDT_ENSURE(udtGetContextPlugInBuffers(context, (u32)udtParserPlugIn::GameState, &buffers) == (s32)udtErrorCode::None);
		UDT_ENSURE(buffers.GameStateCount > 0 && buffers.GameStateCount <= demoCount);

		for(u32 demoIdx = 0; demoIdx < contextDemoCount; ++demoIdx)
		{
			u32 demoInputIdx = 0;
			UDT_ENSURE(udtGetDemoInputIndex(context, demoIdx, &demoInputIdx) == (s32)udtErrorCode::None);
			UDT_ENSURE(demoInputIdx < demoCount);

			// @TODO: "invalid_command_byte.dm_68" should return Count 0
			const udtParseDataBufferRange range = buffers.GameStateRanges[demoIdx];
			UDT_ENSURE(range.Count == 1);

			const udtParseDataGameState gs = buffers.GameStates[range.FirstIndex];
			const DemoInfo& d = demos[demoInputIdx];
			UDT_ENSURE(d.firstTimeMs == gs.FirstSnapshotTimeMs);
			UDT_ENSURE(d.lastTimeMs == gs.LastSnapshotTimeMs);
		}
	}

	UDT_ENSURE(udtDestroyContextGroup(contextGroup) == (s32)udtErrorCode::None);
	UDT_ENSURE(udtShutDownLibrary() == (s32)udtErrorCode::None);

	return true;
}

UDT_TEST("udtParseDemoFiles/Corrupt/1T")
{
	return TestCorruptDemos(1);
}

UDT_TEST("udtParseDemoFiles/Corrupt/4T")
{
	return TestCorruptDemos(4);
}

UDT_TEST("udtCutDemoFilesByPattern/OverlappingCuts/minqlx")
{
	udtParseArg parse = {};
	parse.MessageCb = &CallbackConsoleMessage;
	parse.OutputFolderPath = ctx.OutTempDir.Path;

	char filePath[512];
	sprintf(filePath, "%s/wrong_cut_time_42_653.dm_91", ctx.DemoDir);
	const char* filePathPtr = filePath;
	s32 errorCode = 0;
	udtMultiParseArg multiParse = {};
	multiParse.FileCount = 1;
	multiParse.FilePaths = &filePathPtr;
	multiParse.OutputErrorCodes = &errorCode;

	udtMatchPatternArg matchPattern = {};
	matchPattern.MatchStartOffsetMs = 0;
	matchPattern.MatchEndOffsetMs = 0;

	// let's match every rail shot
	udtFlickRailPatternArg flickPattern = {};
	flickPattern.MinAngleDelta = 0.001f;
	flickPattern.MinSpeed = 0.001f;
	flickPattern.MinAngleDeltaSnapshotCount = 2;
	flickPattern.MinSpeedSnapshotCount = 2;

	udtPatternInfo patterns[2] = {};
	patterns[0].Type = udtPatternType::Matches; // there is 1 match
	patterns[0].TypeSpecificInfo = &matchPattern;
	patterns[1].Type = udtPatternType::FlickRailFrags; // tenbit get 5 rails frags
	patterns[1].TypeSpecificInfo = &flickPattern;

	udtPatternSearchArg pattern = {};
	pattern.PlayerIndex = udtPlayerIndex::FirstPersonPlayer;
	pattern.StartOffsetSec = 10;
	pattern.EndOffsetSec = 10;
	pattern.PatternCount = 2;
	pattern.Patterns = patterns;
	//pattern.Flags = udtPatternSearchArgMask::MergeCutSections; // no merging, please

	UDT_ENSURE(udtInitLibrary() == (s32)udtErrorCode::None);
	UDT_ENSURE(udtCutDemoFilesByPattern(&parse, &multiParse, &pattern) == (s32)udtErrorCode::None);
	UDT_ENSURE(udtShutDownLibrary() == (s32)udtErrorCode::None);

	Cut cuts[] =
	{
		Cut("0:34", "10:55"),
		Cut("3:54", "4:14"),
		Cut("6:20", "6:40"),
		Cut("6:40", "7:00"),
		Cut("7:33", "7:53"),
		Cut("9:17", "9:37")
	};
	ctx.OutTempDir.ListFiles();
	const auto& newFiles = ctx.OutTempDir.GetNewFiles();
	UDT_ENSURE(newFiles.GetSize() == 6);
	UDT_ENSURE(ValidateDemoCutRanges(newFiles, cuts, UDT_COUNT_OF(cuts)));

	return true;
}

UDT_TEST("udtCutDemoFilesByPattern/OverlappingCuts/CPMA")
{
	char filePath0[512];
	char filePath1[512];
	sprintf(filePath0, "%s/dm_68_cpma/duel.dm_68", ctx.DemoDir);
	sprintf(filePath1, "%s/dm_68_cpma/2v2_xscores_2.dm_68", ctx.DemoDir);
	udtParseArg parse = {};
	parse.MessageCb = &CallbackConsoleMessage;
	parse.OutputFolderPath = ctx.OutTempDir.Path;

	constexpr int fileCount = 2;
	s32 errorCodes[fileCount] = {};
	const char* filePaths[fileCount] =
	{
		filePath0,
		filePath1
	};
	udtMultiParseArg multiParse = {};
	multiParse.FileCount = fileCount;
	multiParse.FilePaths = filePaths;
	multiParse.OutputErrorCodes = errorCodes;

	udtMatchPatternArg matchPattern = {};
	matchPattern.MatchStartOffsetMs = 0;
	matchPattern.MatchEndOffsetMs = 0;

	udtFragRunPatternArg fragRunPattern = {};
	fragRunPattern.AllowedMeansOfDeaths = u64(~0);
	fragRunPattern.MinFragCount = 2;
	fragRunPattern.TimeBetweenFragsSec = 15;

	udtPatternInfo patterns[2] = {};
	patterns[0].Type = udtPatternType::Matches;
	patterns[0].TypeSpecificInfo = &matchPattern;
	patterns[1].Type = udtPatternType::FragSequences;
	patterns[1].TypeSpecificInfo = &fragRunPattern;

	udtPatternSearchArg pattern = {};
	pattern.PlayerIndex = udtPlayerIndex::FirstPersonPlayer;
	pattern.StartOffsetSec = 12;
	pattern.EndOffsetSec = 8;
	pattern.PatternCount = 2;
	pattern.Patterns = patterns;
	//pattern.Flags = udtPatternSearchArgMask::MergeCutSections;

	UDT_ENSURE(udtInitLibrary() == (s32)udtErrorCode::None);
	UDT_ENSURE(udtCutDemoFilesByPattern(&parse, &multiParse, &pattern) == (s32)udtErrorCode::None);
	UDT_ENSURE(udtShutDownLibrary() == (s32)udtErrorCode::None);

	Cut cuts[] =
	{
		Cut("15:11", "25:25"),
		Cut("15:53", "16:41"),
		Cut("21:27", "21:59"),
		Cut("24:09", "24:43"),
		Cut("23:47", "39:01"),
		Cut("27:22", "27:48"),
		Cut("29:16", "29:59"),
		Cut("37:42", "38:07")
	};
	ctx.OutTempDir.ListFiles();
	const auto& newFiles = ctx.OutTempDir.GetNewFiles();
	UDT_ENSURE(newFiles.GetSize() == UDT_COUNT_OF(cuts));
	UDT_ENSURE(ValidateDemoCutRanges(newFiles, cuts, UDT_COUNT_OF(cuts)));

	return true;
}

UDT_TEST("udtCutDemoFileByTime/OverlappingCuts")
{
	char filePath[512];
	sprintf(filePath, "%s/dm_68_cpma/3_matches_2_gamestates.dm_68", ctx.DemoDir);

	udtParseArg info = {};
	info.MessageCb = &CallbackConsoleMessage;
	info.OutputFolderPath = ctx.OutTempDir.Path;

	// not in order on purpose, should be sorted
	int i = 0;
	udtCut cuts[7] = {};
	cuts[i].GameStateIndex = 1;
	cuts[i].StartTimeMs = ServerTime(0, 45);
	cuts[i].EndTimeMs = ServerTime(10, 45);
	i++;
	cuts[i].GameStateIndex = 0;
	cuts[i].StartTimeMs = ServerTime(0, 19);
	cuts[i].EndTimeMs = ServerTime(3, 32);
	i++;
	cuts[i].GameStateIndex = 0;
	cuts[i].StartTimeMs = ServerTime(1, 50);
	cuts[i].EndTimeMs = ServerTime(1, 0); // invalid on purpose, should be filtered out
	i++;
	cuts[i].GameStateIndex = 0;
	cuts[i].StartTimeMs = ServerTime(2, 50);
	cuts[i].EndTimeMs = ServerTime(2, 0); // invalid on purpose, should be filtered out
	i++;
	cuts[i].GameStateIndex = 1;
	cuts[i].StartTimeMs = ServerTime(1, 51);
	cuts[i].EndTimeMs = ServerTime(2, 11);
	i++;
	cuts[i].GameStateIndex = 1;
	cuts[i].StartTimeMs = ServerTime(9, 6);
	cuts[i].EndTimeMs = ServerTime(9, 26);
	i++;
	cuts[i].GameStateIndex = 0;
	cuts[i].StartTimeMs = ServerTime(4, 38);
	cuts[i].EndTimeMs = ServerTime(4, 58);
	i++;
	UDT_ENSURE(i == (int)UDT_COUNT_OF(cuts));

	udtCutByTimeArg cutInfo = {};
	cutInfo.CutCount = i;
	cutInfo.Cuts = cuts;

	UDT_ENSURE(udtInitLibrary() == (s32)udtErrorCode::None);
	udtParserContext* const context = udtCreateContext();
	UDT_ENSURE(context != NULL);
	UDT_ENSURE(udtCutDemoFileByTime(context, &info, &cutInfo, filePath) == (s32)udtErrorCode::None);
	UDT_ENSURE(udtDestroyContext(context) == (s32)udtErrorCode::None);
	UDT_ENSURE(udtShutDownLibrary() == (s32)udtErrorCode::None);

	Cut expectedCuts[] =
	{
		// GS 0
		Cut("0:19", "3:32"),
		Cut("4:38", "4:58"),
		// GS 1
		Cut("0:45", "10:45"),
		Cut("1:51", "2:11"),
		Cut("9:06", "9:26")
	};
	ctx.OutTempDir.ListFiles();
	const auto& newFiles = ctx.OutTempDir.GetNewFiles();
	UDT_ENSURE(newFiles.GetSize() == UDT_COUNT_OF(expectedCuts));
	UDT_ENSURE(ValidateDemoCutRanges(newFiles, expectedCuts, UDT_COUNT_OF(expectedCuts)));

	return true;
}

// this doesn't validate a specific thing other than we don't crash, trip up sanitizers, ...
static bool ParseAllDemos(u32 maxThreadCount)
{
	udtFileListQuery query;
	query.FileFilter = &KeepOnlyDemoFiles;
	query.FolderPath = udtString::NewConstRef(ctx.DemoDir);
	query.Recursive = true;
	query.UserData = NULL;
	GetDirectoryFileList(query);

	const u32 demoCount = query.Files.GetSize();
	query.TempAllocator.Clear();
	const uptr filePathsOffset = query.PersistAllocator.Allocate((uptr)demoCount * sizeof(const char*));
	const uptr errorCodesOffset = query.PersistAllocator.Allocate((uptr)demoCount * sizeof(s32));

	// only grab pointers when all allocations are done since relocations can happen
	const char** const filePaths = (const char**)query.PersistAllocator.GetAddressAt(filePathsOffset);;
	s32* errorCodes = (s32*)query.PersistAllocator.GetAddressAt(errorCodesOffset);
	for(u32 i = 0; i < demoCount; ++i)
	{
		filePaths[i] = query.Files[i].Path.GetPtrOrEmpty();
	}

	const u32 plugIns[] =
	{
		udtParserPlugIn::Chat,
		udtParserPlugIn::GameState,
		udtParserPlugIn::Obituaries,
		udtParserPlugIn::Stats,
		udtParserPlugIn::RawCommands,
		udtParserPlugIn::RawConfigStrings,
		udtParserPlugIn::Captures,
		udtParserPlugIn::Scores
	};

	udtParseArg parse = {};
	parse.MessageCb = &CallbackConsoleMessage;
	parse.PlugInCount = UDT_COUNT_OF(plugIns);
	parse.PlugIns = plugIns;
	parse.OutputFolderPath = ctx.OutTempDir.Path;

	udtMultiParseArg multiParse = {};
	multiParse.FileCount = demoCount;
	multiParse.FilePaths = filePaths;
	multiParse.OutputErrorCodes = errorCodes;
	multiParse.MaxThreadCount = maxThreadCount;

	udtParserContextGroup* contextGroup = NULL;
	UDT_ENSURE(udtInitLibrary() == (s32)udtErrorCode::None);
	UDT_ENSURE(udtParseDemoFiles(&contextGroup, &parse, &multiParse) == (s32)udtErrorCode::None);
	UDT_ENSURE(contextGroup != NULL);
	u32 contextCount = 0;
	UDT_ENSURE(udtGetContextCountFromGroup(contextGroup, &contextCount) == (s32)udtErrorCode::None);
	if(maxThreadCount == 1)
	{
		// we asked for single-threaded mode
		UDT_ENSURE(contextCount == 1);
	}

	u32 udtDemoCount = 0;
	for(u32 contextIdx = 0; contextIdx < contextCount; ++contextIdx)
	{
		udtParserContext* context = NULL;
		UDT_ENSURE(udtGetContextFromGroup(contextGroup, contextIdx, &context) == (s32)udtErrorCode::None);
		UDT_ENSURE(context != NULL);

		u32 contextDemoCount = 0;
		UDT_ENSURE(udtGetDemoCountFromContext(context, &contextDemoCount) == (s32)udtErrorCode::None);

		udtDemoCount += contextDemoCount;
	}
	UDT_ENSURE(udtDemoCount == demoCount);

	for(u32 contextIdx = 0; contextIdx < contextCount; ++contextIdx)
	{
		udtParserContext* context = NULL;
		UDT_ENSURE(udtGetContextFromGroup(contextGroup, contextIdx, &context) == (s32)udtErrorCode::None);
		UDT_ENSURE(context != NULL);

		u32 contextDemoCount = 0;
		UDT_ENSURE(udtGetDemoCountFromContext(context, &contextDemoCount) == (s32)udtErrorCode::None);
		UDT_ENSURE(contextDemoCount > 0 && contextDemoCount <= demoCount);

		udtParseDataGameStateBuffers buffers = {};
		UDT_ENSURE(udtGetContextPlugInBuffers(context, (u32)udtParserPlugIn::GameState, &buffers) == (s32)udtErrorCode::None);

		for(u32 demoIdx = 0; demoIdx < contextDemoCount; ++demoIdx)
		{
			u32 demoInputIdx = 0;
			UDT_ENSURE(udtGetDemoInputIndex(context, demoIdx, &demoInputIdx) == (s32)udtErrorCode::None);
			UDT_ENSURE(demoInputIdx < demoCount);
		}
	}

	UDT_ENSURE(udtDestroyContextGroup(contextGroup) == (s32)udtErrorCode::None);
	UDT_ENSURE(udtShutDownLibrary() == (s32)udtErrorCode::None);

	return true;
}

UDT_TEST("udtParseDemoFiles/All/1T")
{
	return ParseAllDemos(1);
}

UDT_TEST("udtParseDemoFiles/All/4T")
{
	return ParseAllDemos(4);
}

#endif
