#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"
#include "string.hpp"
#include "analysis_general.hpp"
#include "api.h"


struct udtParserPlugInGameState : udtBaseParserPlugIn
{
public:
	udtParserPlugInGameState();
	~udtParserPlugInGameState();

	void InitAllocators(u32 demoCount) override;
	u32  GetElementSize() const override { return (u32)sizeof(udtParseDataGameState); }

	void StartDemoAnalysis() override;
	void FinishDemoAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& info, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInGameState);

private:
	void ClearMatch();
	void ClearPlayerInfos();
	void ClearGameState();
	void AddCurrentMatchIfValid();
	void AddCurrentPlayersIfValid();
	void AddCurrentGameState();
	void ProcessDemoTakerName(s32 playerIndex, const udtString* configStrings, udtProtocol::Id protocol);
	void ProcessSystemAndServerInfo(const udtString& configStrings);
	void ProcessPlayerInfo(s32 playerIndex, const udtString& configString);

private:
	udtGeneralAnalyzer _analyzer;
	udtGameStatePlayerInfo _playerInfos[64];
	udtVMArray<udtParseDataGameState> _gameStates; // The final array.
	udtVMArrayWithAlloc<udtMatchInfo> _matches;
	udtVMArrayWithAlloc<udtGameStateKeyValuePair> _keyValuePairs; // Key/value pairs from config strings 0 and 1.
	udtVMArrayWithAlloc<udtGameStatePlayerInfo> _players;
	udtVMLinearAllocator _stringAllocator; // For the key/value pairs and the demo taker's name.
	udtParseDataGameState _currentGameState;
	udtMatchInfo _currentMatch;
	udtProtocol::Id _protocol;
};
