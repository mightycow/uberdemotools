#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"
#include "string.hpp"
#include "analysis_general.hpp"


struct udtParserPlugInGameState : udtBaseParserPlugIn
{
public:
	udtParserPlugInGameState();
	~udtParserPlugInGameState();

	void InitAllocators(u32 demoCount) override;
	void CopyBuffersStruct(void* buffersStruct) const override;
	void UpdateBufferStruct() override;
	u32  GetItemCount() const override;

	void StartDemoAnalysis() override;
	void FinishDemoAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& info, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInGameState);

private:
	void ClearPlayerInfos();
	void ClearGameState();
	void AddCurrentMatchIfValid(bool addIfInProgress = false);
	void AddCurrentPlayersIfValid();
	void AddCurrentGameState();
	void ProcessDemoTakerName(s32 playerIndex, const udtString* configStrings, udtProtocol::Id protocol);
	void ProcessSystemAndServerInfo(const udtString& configStrings);
	void ProcessPlayerInfo(s32 playerIndex, const udtString& configString, s32 serverTimeMs);

private:
	udtGeneralAnalyzer _analyzer;
	udtGameStatePlayerInfo _playerInfos[64];
	bool _playerConnected[64];
	udtVMArray<udtParseDataGameState> _gameStates { "ParserPlugInGameState::GameStatesArray" };
	udtVMArray<udtMatchInfo> _matches { "ParserPlugInGameState::MatchesArray" };
	udtVMArray<udtGameStateKeyValuePair> _keyValuePairs { "ParserPlugInGameState::KeyValuePairsArray" }; // Key/value pairs from config strings 0 and 1.
	udtVMArray<udtGameStatePlayerInfo> _players { "ParserPlugInGameState::PlayersArray" };
	udtVMLinearAllocator _stringAllocator { "ParserPlugInGameState::Strings" }; // For the key/value pairs and the demo taker's name.
	udtParseDataGameState _currentGameState;
	udtParseDataGameStateBuffers _buffers;
	udtProtocol::Id _protocol;
};
