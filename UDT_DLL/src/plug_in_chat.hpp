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
	void CopyBuffersStruct(void* buffersStruct) const override;
	void UpdateBufferStruct() override;
	u32  GetItemCount() const override;

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
	void InitChatEvent(udtParseDataChat& chatEvent, s32 serverTimeMs);

public:
	udtVMArray<udtParseDataChat> ChatEvents { "ParserPlugInChat::ChatMessagesArray" };

private:
	udtString _cleanPlayerNames[64];
	udtVMLinearAllocator _stringAllocator { "ParserPlugInChat::Strings" };
	udtParseDataChatBuffers _buffers;
	s32 _gameStateIndex;
};
