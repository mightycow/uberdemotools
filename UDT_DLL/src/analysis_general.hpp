#pragma once


#include "parser.hpp"


struct udtGeneralAnalyzer
{
public:
	udtGeneralAnalyzer();
	~udtGeneralAnalyzer();

	void InitAllocators(udtVMLinearAllocator& tempAllocator, u32 demoCount);
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
	u32  OvertimeCount() const;
	void SetInWarmUp();
	void SetInProgress();

	udtGameType::Id     GameType() const;
	udtMod::Id          Mod() const;
	udtOvertimeType::Id OvertimeType() const;
	udtGamePlay::Id     GamePlay() const;
	const char*         ModVersion() const;
	const char*         MapName() const;

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

	void UpdateGameState(udtGameState::Id gameState);
	void ProcessQ3ServerInfoConfigString(const char* configString);
	void ProcessCPMAGameInfoConfigString(const char* configString);
	void ProcessQLServerInfoConfigString(const char* configString);
	void ProcessIntermissionConfigString(const udtString& configString);
	void ProcessGameTypeConfigString(const char* configString);
	void ProcessModNameAndVersion();
	void ProcessMapName();
	s32  GetLevelStartTime();
	s32  GetWarmUpEndTime();
	bool IsIntermission();

	udtVMLinearAllocator _stringAllocator;
	udtBaseParser* _parser;
	udtVMLinearAllocator* _tempAllocator;
	const char* _modVersion;
	const char* _mapName;
	s32 _gameStateIndex;
	s32 _matchStartTime;
	s32 _matchEndTime;
	u32 _overTimeCount;
	udtGame::Id _game;
	udtGameType::Id _gameType;
	udtGameState::Id _gameState;
	udtGameState::Id _lastGameState;
	udtMod::Id _mod;
	udtOvertimeType::Id _overTimeType;
	udtGamePlay::Id _gamePlay;
	udtProtocol::Id _protocol;
	bool _processingGameState;
};
