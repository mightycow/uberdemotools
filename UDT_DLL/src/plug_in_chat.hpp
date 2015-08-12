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

	void InitAllocators(u32 demoCount) override;
	u32  GetElementSize() const override { return (u32)sizeof(udtParseDataChat); }

	void StartDemoAnalysis() override;
	void ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser) override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInChat);

	void ProcessChatCommand(udtBaseParser& parser, bool teamChat);
	void ProcessCPMATeamChatCommand(udtBaseParser& parser);

public:
	udtVMArray<udtParseDataChat> ChatEvents;

private:
	udtVMLinearAllocator _chatStringAllocator;
	s32 _gameStateIndex;
};
