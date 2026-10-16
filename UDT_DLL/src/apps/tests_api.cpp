#include "tests.hpp"
#include "uberdemotools.h"
#include <stdio.h>

UDT_TEST_FILE("API");

UDT_TEST("Corrupt")
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
	parse.PlugInCount = UDT_COUNT_OF(plugIns);
	parse.PlugIns = plugIns;
	parse.OutputFolderPath = ctx.OutTempDir.Path;

	udtMultiParseArg multiParse = {};
	multiParse.FileCount = demoCount;
	multiParse.FilePaths = fullDemoPaths;
	multiParse.OutputErrorCodes = errorCodes;
	multiParse.MaxThreadCount = 1;

	udtParserContextGroup* contextGroup = NULL;
	UDT_ENSURE(udtInitLibrary() == (s32)udtErrorCode::None);
	UDT_ENSURE(udtParseDemoFiles(&contextGroup, &parse, &multiParse) == (s32)udtErrorCode::None);
	UDT_ENSURE(contextGroup != NULL);

	u32 contextCount = 0;
	UDT_ENSURE(udtGetContextCountFromGroup(contextGroup, &contextCount) == (s32)udtErrorCode::None);
	UDT_ENSURE(contextCount == 1); // we asked for single-threaded mode
	for(u32 contextIdx = 0; contextIdx < contextCount; ++contextIdx)
	{
		udtParserContext* context = NULL;
		UDT_ENSURE(udtGetContextFromGroup(contextGroup, contextIdx, &context) == (s32)udtErrorCode::None);
		UDT_ENSURE(context != NULL);

		u32 contextDemoCount = 0;
		UDT_ENSURE(udtGetDemoCountFromContext(context, &contextDemoCount) == (s32)udtErrorCode::None);
		UDT_ENSURE(contextDemoCount == demoCount);

		udtParseDataGameStateBuffers buffers = {};
		UDT_ENSURE(udtGetContextPlugInBuffers(context, (u32)udtParserPlugIn::GameState, &buffers) == (s32)udtErrorCode::None);
		UDT_ENSURE(buffers.GameStateCount == demoCount);

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

#if 0

UDT_TEST("udtCutDemoFilesByPattern/OverlappingCuts")
{
	UDT_ENSURE(udtInitLibrary() == (s32)udtErrorCode::None);

	udtParseArg parse = {};
	parse.OutputFolderPath = ctx.OutTempDir.Path;

	constexpr int fileCount = 2;
	s32 errorCodes[fileCount] = {};
	const char* filePaths[fileCount] =
	{
		// @TODO: fix paths
		"C:\\Code\\UberDemoTools\\UDT_DLL\\.bin\\vs2022\\x64\\debug\\dm_68_cpma\\duel.dm_68",
		"C:\\Code\\UberDemoTools\\UDT_DLL\\.bin\\vs2022\\x64\\debug\\dm_68_cpma\\2v2_xscores_2.dm_68"
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
	pattern.StartOffsetSec = 10;
	pattern.EndOffsetSec = 10;
	pattern.PatternCount = 2;
	pattern.Patterns = patterns;
	//pattern.Flags = udtPatternSearchArgMask::MergeCutSections;

	UDT_ENSURE(udtCutDemoFilesByPattern(&parse, &multiParse, &pattern) == (s32)udtErrorCode::None);
	UDT_ENSURE(udtShutDownLibrary() == (s32)udtErrorCode::None);

	ctx.OutTempDir.ListFiles();
	UDT_ENSURE(ctx.OutTempDir.GetNewFiles().GetSize() == 8);
	
	// @TODO: validate the files and/or cut locations

	return true;
}

#endif

#if 0

// @TODO: (re)move
void TestMultiTimedCut()
{
	const char* filePath = "C:\\Code\\UberDemoTools\\demo_files\\dm_68_cpma\\3_matches_2_gamestates.dm_68";
	const char* outputPath = "C:\\Code\\UberDemoTools\\UDT_DLL\\.bin\\vs2022\\x64\\debug\\cut";

	udtParseArg info = {};
	info.MessageCb = &CallbackConsoleMessage;
	info.ProgressCb = &CallbackConsoleProgress;
	info.OutputFolderPath = outputPath;
	info.MinProgressTimeMs = 50;

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
	cuts[i].EndTimeMs = ServerTime(1, 0); // invalid on purpose
	i++;
	cuts[i].GameStateIndex = 0;
	cuts[i].StartTimeMs = ServerTime(2, 50);
	cuts[i].EndTimeMs = ServerTime(2, 0); // invalid on purpose
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
	assert((size_t)i == UDT_ARRAY_LENGTH(cuts));

	udtCutByTimeArg cutInfo = {};
	cutInfo.CutCount = i;
	cutInfo.Cuts = cuts;

	udtParserContext* const context = udtCreateContext();
	udtCutDemoFileByTime(context, &info, &cutInfo, filePath);
	udtDestroyContext(context);
}

// @TODO: (re)move
void TestPatternCutMinqlx()
{
	udtParseArg parse = {};
	parse.OutputFolderPath = "C:\\Code\\UberDemoTools\\UDT_DLL\\.bin\\vs2022\\x64\\debug\\cut";
	parse.MessageCb = &CallbackConsoleMessage;
	parse.ProgressCb = &CallbackConsoleProgress;
	parse.MinProgressTimeMs = 50;

	s32 errorCode = 0;
	const char* filePath = "C:\\Code\\UberDemoTools\\UDT_DLL\\.bin\\vs2022\\x64\\debug\\tenbit\\20260914-183300_slot03_tenbit.dm_91";
	udtMultiParseArg multiParse = {};
	multiParse.FileCount = 1;
	multiParse.FilePaths = &filePath;
	multiParse.OutputErrorCodes = &errorCode;

	udtMatchPatternArg matchPattern = {};
	matchPattern.MatchStartOffsetMs = 0;
	matchPattern.MatchEndOffsetMs = 0;

	udtFlickRailPatternArg flickPattern = {};
	flickPattern.MinAngleDelta = 0.001f;
	flickPattern.MinSpeed = 0.001f;
	flickPattern.MinAngleDeltaSnapshotCount = 2;
	flickPattern.MinSpeedSnapshotCount = 2;

	udtPatternInfo patterns[2] = {};
	patterns[0].Type = udtPatternType::Matches;
	patterns[0].TypeSpecificInfo = &matchPattern;
	patterns[1].Type = udtPatternType::FlickRailFrags;
	patterns[1].TypeSpecificInfo = &flickPattern;

	udtPatternSearchArg pattern = {};
	pattern.PlayerIndex = udtPlayerIndex::FirstPersonPlayer;
	pattern.StartOffsetSec = 10;
	pattern.EndOffsetSec = 10;
	pattern.PatternCount = 2;
	pattern.Patterns = patterns;
	//pattern.Flags = udtPatternSearchArgMask::MergeCutSections;

	udtCutDemoFilesByPattern(&parse, &multiParse, &pattern);
}

#endif
