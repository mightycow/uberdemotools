#pragma once


#include "api.h"


struct udtParsingJobType
{
	enum Id
	{
		General,      // Parse the demo to extract information the user can later access.
		CutByPattern, // Parse the demo to create a cut list, then apply the cuts and discard all the data.
		Conversion,   // Convert the demo to the latest protocol or leave it untouched if it already is good.
		Count
	};
};

extern bool InitContextWithPlugIns(udtParserContext& context, const udtParseArg& info, u32 demoCount, udtParsingJobType::Id jobType, const void* jobSpecificInfo = NULL);
extern bool CutByPattern(udtParserContext* context, const udtParseArg* info, const char* demoFilePath);
extern bool ParseDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath, bool clearPlugInData);
extern bool ConvertDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath, const udtProtocolConversionArg* conversionInfo);
extern s32  udtParseMultipleDemosSingleThread(udtParsingJobType::Id jobType, udtParserContext* context, const udtParseArg* info, const udtMultiParseArg* extraInfo, const void* jobSpecificInfo);
