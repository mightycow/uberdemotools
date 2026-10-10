#include "tests.hpp"
#include "uberdemotools.h"
#include <stdio.h>

UDT_TEST_FILE("API");

#if 1
UDT_TEST("HW_exception")
{
	volatile int div = 0;
	volatile int x = 42 / div;
	printf("%d", x);

	return true;
}
#endif

#if 0

// @TODO: (re)move
void TestMultiPatternCut()
{
	udtParseArg parse = {};
	parse.OutputFolderPath = "C:\\Code\\UberDemoTools\\UDT_DLL\\.bin\\vs2022\\x64\\debug\\cut";
	parse.MessageCb = &CallbackConsoleMessage;
	parse.ProgressCb = &CallbackConsoleProgress;
	parse.MinProgressTimeMs = 50;

	constexpr int fileCount = 2;
	s32 errorCodes[fileCount] = {};
	const char* filePaths[fileCount] =
	{
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

	udtCutDemoFilesByPattern(&parse, &multiParse, &pattern);
}

// @TODO: (re)move
s32 ServerTime(s32 minute, s32 seconds)
{
	return 1000 * (minute * 60 + seconds);
}

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
