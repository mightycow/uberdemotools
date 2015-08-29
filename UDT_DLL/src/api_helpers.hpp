#pragma once


#include "api.h"


struct udtParsingJobType
{
	enum Id
	{
		General,      // Parse the demo to extract information the user can later access.
		CutByPattern, // Parse the demo to create a cut list, then apply the cuts and discard all the data.
		Conversion,   // Convert the demo to the selected protocol or leave it untouched if it already is good.
		TimeShift,    // Shift non-first-person living player entities back in time to act as an anti-lag.
		ExportToJSON, // Write a .JSON file with the data from the selected plug-ins.
		Count
	};
};

extern bool InitContextWithPlugIns(udtParserContext& context, const udtParseArg& info, u32 demoCount, udtParsingJobType::Id jobType, const void* jobSpecificInfo = NULL);
extern bool ProcessSingleDemoFile(udtParsingJobType::Id jobType, udtParserContext* context, u32 demoIndex, const udtParseArg* info, const char* demoFilePath, const void* jobSpecificInfo);
extern bool MergeDemosNoInputCheck(const udtParseArg* info, const char** filePaths, u32 fileCount, udtProtocol::Id protocol);
extern s32  udtParseMultipleDemosSingleThread(udtParsingJobType::Id jobType, udtParserContext* context, const udtParseArg* info, const udtMultiParseArg* extraInfo, const void* jobSpecificInfo);
