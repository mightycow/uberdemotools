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

	void ClearStringAllocator();
	void SetIntermissionEndTime();
	void ResetForNextMatch();
	bool HasMatchJustStarted() const;
	bool HasMatchJustEnded() const;
	bool IsMatchInProgress() const;
	bool IsInIntermission() const;
	bool CanMatchBeAdded() const;
	s32  MatchStartTime() const;
	s32  MatchEndTime() const;
	s32  GameStateIndex() const;
	u32  OvertimeCount() const;
	u32  TimeOutCount() const;
	s32  TotalTimeOutDuration() const;
	bool Forfeited() const;
	bool MercyLimited() const;
	s32  GetTimeOutStartTime(u32 index) const;
	s32  GetTimeOutEndTime(u32 index) const;
	u32  GetMatchStartDateEpoch() const;
	u32  GetTimeLimit() const { return _timeLimit; }
	u32  GetScoreLimit() const { return _scoreLimit; }
	u32  GetFragLimit() const { return _fragLimit; }
	u32  GetCaptureLimit() const { return _captureLimit; }
	u32  GetRoundLimit() const { return _roundLimit; }
	s32  CountDownStartTime() const { return _warmUpEndTime; }
	s32  IntermissionEndTime() const { return _intermissionEndTime; }

	udtGameType::Id     GameType() const;
	udtMod::Id          Mod() const;
	udtOvertimeType::Id OvertimeType() const;
	udtGamePlay::Id     GamePlay() const;
	udtString           ModVersion() const;
	udtString           MapName() const;

private:
	UDT_NO_COPY_SEMANTICS(udtGeneralAnalyzer);

	struct udtGameState
	{
		enum Id
		{
			WarmUp,
			CountDown,
			InProgress,
			Intermission,
			Count
		};
	};

	struct Constants
	{
		enum Id
		{
			MaxTimeOutCount = 64
		};
	};

	struct udtTimeOut
	{
		s32 StartTime;
		s32 EndTime;
	};

	// Functions with the "Once" suffix only need to be called from ProcessGamestateMessage.
	void UpdateGameState(udtGameState::Id gameState);
	void ProcessModNameAndVersionOnce();
	void ProcessMapNameOnce();
	void ProcessQ3ServerInfoConfigStringOnce(const char* configString);
	void ProcessCPMAGameInfoConfigString(const char* configString);
	void ProcessCPMARoundInfoConfigString(const char* configString);
	void ProcessQLServerInfoConfigString(const char* configString);
	void ProcessIntermissionConfigString(const udtString& configString);
	void ProcessGameTypeFromServerInfo(const char* configString);
	void ProcessOSPGamePlayConfigString(const char* configString);
	void ProcessQ3AndQLServerInfoConfigString(const char* configString);
	void ProcessScores2(const char* configString);
	void ProcessScores2Player(const char* configString);
	void ProcessQLPauseStartConfigString(const char* configString);
	void ProcessQLPauseEndConfigString(const char* configString);
	s32  GetLevelStartTime();
	s32  GetWarmUpEndTime();
	bool IsIntermission();
	void UpdateMatchStartTime();
	void ResetForNextGameState();
	void SetTimeOutStartTime(s32 startTime);
	void SetTimeOutEndTime(s32 startTime);
	s32  GetTimeOutStartTime() const;

	udtTimeOut _timeOutStartAndEndTimes[Constants::MaxTimeOutCount];
	udtVMLinearAllocator _stringAllocator { "GeneralAnalyzer::Strings" };
	udtBaseParser* _parser;
	udtVMLinearAllocator* _tempAllocator;
	udtString _modVersion; // Allocated by _stringAllocator.
	udtString _mapName;    // Allocated by _stringAllocator.
	s32 _gameStateIndex;
	s32 _matchStartTime;
	s32 _matchEndTime;
	s32 _prevMatchStartTime;
	s32 _warmUpEndTime; // Count down start time or match start time.
	s32 _intermissionEndTime; // Intermission end time or match end time.
	u32 _overTimeCount;
	u32 _timeOutCount;
	s32 _totalTimeOutDuration;
	u32 _matchStartDateEpoch;
	u32 _timeLimit;
	u32 _scoreLimit;
	u32 _fragLimit;
	u32 _captureLimit;
	u32 _roundLimit;
	s32 _te;
	udtGame::Id _game;
	udtGameType::Id _gameType;
	udtGameState::Id _gameState;
	udtGameState::Id _lastGameState;
	udtMod::Id _mod;
	udtOvertimeType::Id _overTimeType;
	udtGamePlay::Id _gamePlay;
	udtProtocol::Id _protocol;
	bool _processingGameState;
	bool _forfeited;
	bool _timeOut;
	bool _mercyLimited;
	bool _serverPause;
};
