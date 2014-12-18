#pragma once


#include "api.h"


extern bool CutByChat(udtParserContext* context, const udtParseArg* info, const udtCutByChatArg* chatInfo, const char* demoFilePath);
extern bool CutByFrag(udtParserContext* context, const udtParseArg* info, const udtCutByFragArg* fragInfo, const char* demoFilePath);
extern bool CutByAward(udtParserContext* context, const udtParseArg* info, const udtCutByAwardArg* awardInfo, const char* demoFilePath);
extern bool ParseDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath, bool clearPlugInData);
