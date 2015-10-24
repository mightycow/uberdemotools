#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"


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

	void ProcessPlayerConfigString(udtBaseParser& parser, const udtString& cs, s32 playerIndex);
	void ProcessChatCommand(udtBaseParser& parser);
	void ProcessTeamChatCommand(udtBaseParser& parser);
	void ProcessCPMATeamChatCommand(udtBaseParser& parser);
	void ExtractPlayerIndexRelatedData(udtParseDataChat& chatEvent, const udtString& argument1, udtBaseParser& parser);
	void ProcessQ3GlobalChat(udtParseDataChat& chatEvent, const udtString* argument1);
	bool IsPlayerCleanName(const udtString& cleanName);

public:
	udtVMArray<udtParseDataChat> ChatEvents;

private:
	udtString _cleanPlayerNames[64];
	udtVMLinearAllocator _chatStringAllocator;
	s32 _gameStateIndex;
};
