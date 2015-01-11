#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"
#include "api.h"


struct udtParserPlugInChat : udtBaseParserPlugIn
{
public:
	udtParserPlugInChat();
	~udtParserPlugInChat();

	void InitAllocators(u32 demoCount);
	u32  GetElementSize() const { return (u32)sizeof(udtParseDataChat); }

	void ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser);
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser);

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInChat);

public:
	udtVMArray<udtParseDataChat> ChatEvents;

private:
	udtVMLinearAllocator _chatStringAllocator;
	s32 _gameStateIndex;
};
