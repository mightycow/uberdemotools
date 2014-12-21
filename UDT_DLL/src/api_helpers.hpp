#pragma once


#include "api.h"


struct udtParsingJobType
{
	enum Id
	{
		General,
		CutByChat,
		CutByFrag,
		CutByAward,
		Count
	};
};

extern bool CutByChat(udtParserContext* context, const udtParseArg* info, const udtCutByChatArg* chatInfo, const char* demoFilePath);
extern bool CutByFrag(udtParserContext* context, const udtParseArg* info, const udtCutByFragArg* fragInfo, const char* demoFilePath);
extern bool CutByAward(udtParserContext* context, const udtParseArg* info, const udtCutByAwardArg* awardInfo, const char* demoFilePath);
extern bool CutByWhatever(udtParsingJobType::Id jobType, udtParserContext* context, const udtParseArg* info, const void* jobSpecificInfo, const char* demoFilePath);
extern bool ParseDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath, bool clearPlugInData);
extern s32 udtParseMultipleDemosSingleThread(udtParsingJobType::Id jobType, udtParserContext* context, const udtParseArg* info, const udtMultiParseArg* extraInfo, const void* jobSpecificInfo);
