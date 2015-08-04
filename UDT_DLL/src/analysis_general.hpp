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

	// The start and end times will only be valid when MatchJustEnded() returns true.
	bool HasMatchJustStarted() const;
	bool HasMatchJustEnded() const;
	bool IsMatchInProgress() const;
	s32  MatchStartTime() const;
	s32  MatchEndTime() const;
	s32  GameStateIndex() const;
	void SetInWarmUp();
	void SetInProgress();

private:
	UDT_NO_COPY_SEMANTICS(udtGeneralAnalyzer);

	// "gamename" in cs 0.
	struct udtGameType
	{
		enum Id
		{
			Q3,
			QL,
			CPMA,
			OSP
		};
	};

	// "g_gameState" in cs 0.
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
	s32  GetLevelStartTime();

	udtBaseParser* _parser;
	udtVMLinearAllocator* _tempAllocator;
	s32 _gameStateIndex;
	s32 _matchStartTime;
	s32 _matchEndTime;
	udtGameType::Id _gameType;
	udtGameState::Id _gameState;
	udtGameState::Id _lastGameState;
	udtProtocol::Id _protocol;
	bool _processingGameState;
};
