#pragma once


#include "parser.hpp"


struct udtGeneralAnalyzer
{
public:
	udtGeneralAnalyzer();
	~udtGeneralAnalyzer();

	void SetTempAllocator(udtVMLinearAllocator& tempAllocator);
	void ResetForNextDemo();
	void FinishDemoAnalysis();
 
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser);

	bool HasMatchJustStarted() const;
	bool HasMatchJustEnded() const;
	bool IsMatchInProgress() const;
	s32  MatchStartTime() const;
	s32  MatchEndTime() const;
	s32  GameStateIndex() const;
	void SetInWarmUp();
	void SetInProgress();

	udtGameType::Id GetGameType() const;

private:
	UDT_NO_COPY_SEMANTICS(udtGeneralAnalyzer);

	struct udtGameState
	{
		enum Id
		{
			WarmUp, // For our purposes, we consider intermission to be warm-up.
			CountDown,
			InProgress,
			Count
		};
	};

	void ClearMatch();
	void UpdateGameState(udtGameState::Id gameState);
	void ProcessQ3ServerInfoConfigString(const char* configString);
	void ProcessCPMAGameInfoConfigString(const char* configString);
	void ProcessQLServerInfoConfigString(const char* configString);
	void ProcessIntermissionConfigString(const udtString& configString);
	void ProcessGameTypeConfigString(const char* configString);
	s32  GetLevelStartTime();

	udtBaseParser* _parser;
	udtVMLinearAllocator* _tempAllocator;
	s32 _gameStateIndex;
	s32 _matchStartTime;
	s32 _matchEndTime;
	udtGame::Id _game;
	udtGameType::Id _gameType;
	udtGameState::Id _gameState;
	udtGameState::Id _lastGameState;
	udtProtocol::Id _protocol;
	bool _processingGameState;
};
