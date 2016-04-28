#pragma once


#include "uberdemotools.h"
#include "macros.hpp"


struct udtParsingJobType
{
	enum Id
	{
		General,      // Parse the demo to extract information the user can later access.
		CutByPattern, // Parse the demo to create a cut list, then apply the cuts and discard all the data.
		Conversion,   // Convert the demo to the selected protocol or leave it untouched if it already is good.
		TimeShift,    // Shift non-first-person living player entities back in time to act as an anti-lag.
		ExportToJSON, // Write a .JSON file with the data from the selected plug-ins.
		FindPatterns, // Generate and keep the list of cuts.
		Count
	};
};

struct udtTimer;

struct SingleThreadProgressContext
{
	u64 TotalByteCount;
	u64 ProcessedByteCount;
	u64 CurrentJobByteCount;
	udtProgressCallback UserCallback;
	void* UserData;
	udtTimer* Timer;
	u32 MinProgressTimeMs;
};

extern void SingleThreadProgressCallback(f32 jobProgress, void* userData);
extern bool InitContextWithPlugIns(udtParserContext& context, const udtParseArg& info, u32 demoCount, udtParsingJobType::Id jobType, const void* jobSpecificInfo = NULL);
extern bool ProcessSingleDemoFile(udtParsingJobType::Id jobType, udtParserContext* context, u32 contextDemoIndex, u32 inputDemoIndex, const udtParseArg* info, const char* demoFilePath, const void* jobSpecificInfo);
extern bool MergeDemosNoInputCheck(const udtParseArg* info, const char** filePaths, u32 fileCount, udtProtocol::Id protocol);
extern s32  udtParseMultipleDemosSingleThread(udtParsingJobType::Id jobType, udtParserContext* context, const udtParseArg* info, const udtMultiParseArg* extraInfo, const void* jobSpecificInfo);
