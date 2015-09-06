#include "analysis_general.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


/*
=================
Match starts/ends
=================

CPMA:
- CS_CPMA_GAME_INFO: tw  -1 means in warm-up           --> ts last restart time --- if ts is 0, the match countdown is starting
- CS_CPMA_GAME_INFO: tw   0 means in match             --> ts match start time
- CS_CPMA_GAME_INFO: tw > 0 means in warm-up countdown --> ts last restart time --- tw countdown end time
- When a match starts: tw is set to 0 and ts is the exact start time
- When a match ends  : nothing except for print/buzzer commands
- No command is sent to specify there is an overtime (except for print commands, it's implicitly defined)
- No command is sent to notify that a match ends, and tw/ts don't change at the end of a match recored with cg_autoAction

BaseQ3 and OSP:
- When a match starts: map_restart is called and CS_LEVEL_START_TIME holds the exact start time
- When a match ends  : CS_INTERMISSION is set to "1" or "qtrue"

Quake Live:
- cs 0 > g_roundWarmupDelay timeInMs
- cs 0 > g_gameState PRE_GAME COUNT_DOWN IN_PROGRESS
- When a match starts: map_restart is called, g_gameState is set to IN_PROGRESS in cs 0 and CS_LEVEL_START_TIME holds the exact start time
- When a match ends  : CS_INTERMISSION is set to "1" or "qtrue"

=========
Time-outs
=========

CPMA:
- CS_CPMA_GAME_INFO: te    0 means not in time-out --> td last time-out's duration
- CS_CPMA_GAME_INFO: te != 0 means in time-out     --> te time-out start time

======
Scores
======

CPMA:
- Non-team modes: sb is 1st place score, sr is 2nd place score
- Non-team modes: 1st and 2nd place client numbers are given by the dmscores command
- Duel: during the match, the score is sent via the round info CS and when it ends the game info CS
- Duel: when a player forfeits by going to the spectators team, his round score is set to -9999
*/


udtGeneralAnalyzer::udtGeneralAnalyzer()
{
	_parser = NULL;
	_tempAllocator = NULL;
	_processingGameState = false;
}

udtGeneralAnalyzer::~udtGeneralAnalyzer()
{
}

void udtGeneralAnalyzer::InitAllocators(udtVMLinearAllocator& tempAllocator, u32 demoCount)
{
	_tempAllocator = &tempAllocator;
	_stringAllocator.Init((uptr)demoCount * (uptr)(1 << 16), "GeneralAnalyzer::Strings");
}

void udtGeneralAnalyzer::ResetForNextDemo()
{
	_modVersion = NULL;
	_mapName = NULL;
	_gameStateIndex = -1;
	_matchStartTime = S32_MIN;
	_matchEndTime = S32_MIN;
	_prevMatchStartTime = S32_MIN;
	_overTimeCount = 0;
	_timeOutCount = 0;
	_totalTimeOutDuration = 0;
	_game = udtGame::Q3;
	_gameType = udtGameType::Invalid;
	_gameState = udtGameState::WarmUp;
	_lastGameState = udtGameState::WarmUp;
	_mod = udtMod::None;
	_overTimeType = udtOvertimeType::Timed;
	_gamePlay = udtGamePlay::VQ3;
	_protocol = udtProtocol::Invalid;
	_forfeited = false;
	_timeOut = false;
	_mercyLimited = false;
	_serverPause = false;
}

void udtGeneralAnalyzer::FinishDemoAnalysis()
{
	if(_gameState == udtGameState::InProgress && _matchStartTime != S32_MIN)
	{
		_lastGameState = udtGameState::InProgress;
		_gameState = udtGameState::WarmUp;
		_matchEndTime = _parser->_inServerTime;
	}
}

void udtGeneralAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& /*arg*/, udtBaseParser& /*parser*/)
{
}

void udtGeneralAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& parser)
{
	++_gameStateIndex;
	_parser = &parser;
	_protocol = parser._inProtocol;
	_gamePlay = _protocol <= udtProtocol::Dm68 ? udtGamePlay::VQ3 : udtGamePlay::CQL;

	_processingGameState = true;

	// First game state message?
	if(_gameStateIndex == 0)
	{
		_game = udtGame::Q3;
		if(_protocol >= udtProtocol::Dm73)
		{
			_game = udtGame::QL;
		}
		else
		{
			ProcessQ3ServerInfoConfigStringOnce(parser._inConfigStrings[CS_SERVERINFO].String);
			ProcessModNameAndVersionOnce();
		}
	}
	else
	{
		ResetForNextGameState();
	}

	ProcessMapNameOnce();
	ProcessGameTypeFromServerInfo(parser._inConfigStrings[CS_SERVERINFO].String);

	if(_game == udtGame::CPMA)
	{
		ProcessCPMAGameInfoConfigString(parser._inConfigStrings[CS_CPMA_GAME_INFO].String);
	}
	else if(_game == udtGame::QL)
	{
		ProcessQLServerInfoConfigString(parser._inConfigStrings[CS_SERVERINFO].String);
		const s32 startIdx = idConfigStringIndex::PauseStart(_protocol);
		const s32 endIdx = idConfigStringIndex::PauseEnd(_protocol);
		if(startIdx != -1 && endIdx != -1)
		{
			ProcessQLPauseStartConfigString(parser._inConfigStrings[startIdx].String);
			ProcessQLPauseEndConfigString(parser._inConfigStrings[endIdx].String);
		}
	}
	else if(_game == udtGame::Q3 || _game == udtGame::OSP)
	{
		UpdateMatchStartTime();
		const s32 warmUpEndTime = GetWarmUpEndTime();
		const bool noIntermission = !IsIntermission();
		const bool noWarmUpEndTime = warmUpEndTime == S32_MIN || warmUpEndTime < _matchStartTime;
		if(noIntermission && noWarmUpEndTime)
		{
			UpdateGameState(udtGameState::InProgress);
		}

		if(_game == udtGame::OSP)
		{
			ProcessOSPGamePlayConfigString(parser._inConfigStrings[CS_OSP_GAMEPLAY].String);
		}
	}

	_processingGameState = false;
}

void udtGeneralAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& parser)
{
	const idTokenizer& tokenizer = parser.GetTokenizer();
	if(tokenizer.GetArgCount() == 0)
	{
		return;
	}

	const udtString command = tokenizer.GetArg(0);

	if(_game == udtGame::CPMA && 
	   tokenizer.GetArgCount() >= 2 && 
	   udtString::Equals(command, "print"))
	{
		u32 index = 0;
		const udtString printMessage = tokenizer.GetArg(1);
		if(udtString::ContainsNoCase(index, printMessage, "match complete") || 
		   udtString::ContainsNoCase(index, printMessage, "match over") ||
		   udtString::ContainsNoCase(index, printMessage, "timelimit hit"))
		{
			UpdateGameState(udtGameState::WarmUp);
			if(HasMatchJustEnded())
			{
				_matchEndTime = parser._inServerTime;
			}
		}
	}

	if(tokenizer.GetArgCount() >= 2 &&
	   (udtString::Equals(command, "print") || 
	   udtString::Equals(command, "cp") ||
	   udtString::Equals(command, "pcp")))
	{
		u32 index = 0;
		const udtString printMessage = tokenizer.GetArg(1);
		if(udtString::ContainsNoCase(index, printMessage, "sudden death") &&
		   !udtString::ContainsNoCase(index, printMessage, "overtime mode:"))
		{
			// @NOTE: That message might contain the word "overtime" too,
			// which is why we check against this one first.
			UpdateGameState(udtGameState::InProgress);
			++_overTimeCount;
			_overTimeType = udtOvertimeType::SuddenDeath;
		}
		else if(udtString::ContainsNoCase(index, printMessage, "overtime") &&
				!udtString::ContainsNoCase(index, printMessage, "overtime mode:"))
		{
			UpdateGameState(udtGameState::InProgress);
			++_overTimeCount;
			if(_mod == udtMod::CPMA && _gameType == udtGameType::CTFS)
			{
				_overTimeType = udtOvertimeType::SuddenDeath;
			}
		}
		else if(udtString::ContainsNoCase(index, printMessage, "respawn delay") &&
				!udtString::ContainsNoCase(index, printMessage, "weapon respawn delay:"))
		{
			UpdateGameState(udtGameState::InProgress);
			_overTimeCount = 1;
			_overTimeType = udtOvertimeType::SuddenDeath;
		}

		if(udtString::ContainsNoCase(index, printMessage, "forfeit") && 
		   _gameState == udtGameState::InProgress)
		{
			_forfeited = true;
		}

		if(udtString::ContainsNoCase(index, printMessage, "mercylimit") &&
		   _gameState == udtGameState::InProgress)
		{
			_mercyLimited = true;
		}
	}
	
	if(_game != udtGame::CPMA &&
	   tokenizer.GetArgCount() == 1 &&
	   udtString::Equals(command, "map_restart"))
	{
		UpdateGameState(udtGameState::InProgress);
		if(HasMatchJustStarted())
		{
			UpdateMatchStartTime();
		}
	}
	
	s32 csIndex = 0;
	if(tokenizer.GetArgCount() != 3 || 
	   !udtString::Equals(command, "cs") || 
	   !StringParseInt(csIndex, tokenizer.GetArgString(1)))
	{
		return;
	}

	const char* const configString = tokenizer.GetArgString(2);

	if(csIndex == CS_SERVERINFO)
	{
		ProcessGameTypeFromServerInfo(configString);
	}

	if(_game == udtGame::QL && csIndex == CS_SERVERINFO)
	{
		ProcessQLServerInfoConfigString(configString);
	}

	if(_game == udtGame::CPMA && csIndex == CS_CPMA_GAME_INFO)
	{
		ProcessCPMAGameInfoConfigString(configString);
	}
	else if(_game == udtGame::CPMA && csIndex == CS_CPMA_ROUND_INFO)
	{
		ProcessCPMARoundInfoConfigString(configString);
	}
	else if((_game == udtGame::Q3 || _game == udtGame::OSP) && csIndex == idConfigStringIndex::LevelStartTime(_protocol))
	{
		UpdateMatchStartTime();
	}
	else if(_game == udtGame::QL && csIndex == idConfigStringIndex::LevelStartTime(_protocol))
	{
		if(_timeOutCount == 0 && !_timeOut)
		{
			UpdateMatchStartTime();
		}
		else if(!_serverPause)
		{
			const s32 shiftedStartTime = GetLevelStartTime();
			_totalTimeOutDuration = shiftedStartTime - _matchStartTime;
		}
	}
	else if(_game != udtGame::CPMA && csIndex == idConfigStringIndex::Intermission(_protocol))
	{	
		ProcessIntermissionConfigString(tokenizer.GetArg(2));
	}
	else if(_game == udtGame::OSP && csIndex == CS_OSP_GAMEPLAY)
	{
		ProcessOSPGamePlayConfigString(configString);
	}
	else if(csIndex == CS_SCORES2)
	{
		ProcessScores2(configString);
	}
	else if(csIndex == idConfigStringIndex::SecondPlacePlayerName(_protocol))
	{
		ProcessScores2Player(configString);
	}
	else if(_game == udtGame::QL && csIndex == idConfigStringIndex::PauseStart(_protocol))
	{
		ProcessQLPauseStartConfigString(configString);
	}
	else if(_game == udtGame::QL && csIndex == idConfigStringIndex::PauseEnd(_protocol))
	{
		ProcessQLPauseEndConfigString(configString);
	}
}

void udtGeneralAnalyzer::ResetForNextMatch()
{
	_overTimeCount = 0;
	_timeOutCount = 0;
	_totalTimeOutDuration = 0;
	_overTimeType = udtOvertimeType::Timed;
	_forfeited = false;
	_timeOut = false;
	_mercyLimited = false;
	_serverPause = false;
}

bool udtGeneralAnalyzer::HasMatchJustStarted() const
{
	return _lastGameState != udtGameState::InProgress && _gameState == udtGameState::InProgress;
}

bool udtGeneralAnalyzer::HasMatchJustEnded() const
{
	// The stricter rule is to avoid CPMA round endings to look like match endings.
	return _lastGameState == udtGameState::InProgress && _gameState == udtGameState::WarmUp;
}

bool udtGeneralAnalyzer::IsMatchInProgress() const
{
	return _gameState == udtGameState::InProgress;
}

s32 udtGeneralAnalyzer::MatchStartTime() const
{
	return (_matchStartTime < _matchEndTime) ? _matchStartTime : _prevMatchStartTime;
}

s32 udtGeneralAnalyzer::MatchEndTime() const
{
	return _matchEndTime;
}

s32 udtGeneralAnalyzer::GameStateIndex() const
{
	return _gameStateIndex;
}

u32 udtGeneralAnalyzer::OvertimeCount() const
{
	return _overTimeCount;
}

u32 udtGeneralAnalyzer::TimeOutCount() const
{
	return _timeOutCount;
}

s32 udtGeneralAnalyzer::TotalTimeOutDuration() const
{
	return _totalTimeOutDuration;
}

udtGameType::Id udtGeneralAnalyzer::GameType() const
{
	return _gameType;
}

udtMod::Id udtGeneralAnalyzer::Mod() const
{
	return _mod;
}

udtOvertimeType::Id udtGeneralAnalyzer::OvertimeType() const
{
	return _overTimeType;
}

udtGamePlay::Id udtGeneralAnalyzer::GamePlay() const
{
	return _gamePlay;
}

const char* udtGeneralAnalyzer::ModVersion() const
{
	return _modVersion;
}

const char* udtGeneralAnalyzer::MapName() const
{
	return _mapName;
}

bool udtGeneralAnalyzer::Forfeited() const
{
	return _forfeited;
}

bool udtGeneralAnalyzer::MercyLimited() const
{
	return _mercyLimited;
}

void udtGeneralAnalyzer::SetInWarmUp()
{
	_lastGameState = udtGameState::WarmUp;
	_gameState = udtGameState::WarmUp;
	_matchStartTime = S32_MIN;
	_matchEndTime = S32_MIN;
}

void udtGeneralAnalyzer::SetInProgress()
{
	_lastGameState = udtGameState::InProgress;
	_gameState = udtGameState::InProgress;
}

void udtGeneralAnalyzer::UpdateGameState(udtGameState::Id gameState)
{
	if(_processingGameState)
	{
		_lastGameState = gameState;
		_gameState = gameState;
	}
	else
	{
		_lastGameState = _gameState;
		_gameState = gameState;
	}
}

void udtGeneralAnalyzer::ProcessQ3ServerInfoConfigStringOnce(const char* configString)
{
	if(configString == NULL)
	{
		return;
	}

	udtVMScopedStackAllocator scopedTempAllocator(*_tempAllocator);

	u32 charIndex = 0;
	udtString varValue;
	if(ParseConfigStringValueString(varValue, *_tempAllocator, "gamename", configString) &&
	   udtString::Equals(varValue, "cpma"))
	{
		_game = udtGame::CPMA;
	}
	else if(ParseConfigStringValueString(varValue, *_tempAllocator, "gamename", configString) &&
			udtString::ContainsNoCase(charIndex, varValue, "osp"))
	{
		_game = udtGame::OSP;
	}
	else if(ParseConfigStringValueString(varValue, *_tempAllocator, "gameversion", configString) &&
			udtString::ContainsNoCase(charIndex, varValue, "osp"))
	{
		_game = udtGame::OSP;
	}
}

void udtGeneralAnalyzer::ProcessCPMAGameInfoConfigString(const char* configString)
{
	if(configString == NULL)
	{
		return;
	}

	udtVMScopedStackAllocator tempAllocScope(*_tempAllocator);

	s32 gamePlay = 0;
	if(ParseConfigStringValueInt(gamePlay, *_tempAllocator, "pm", configString))
	{
		switch(gamePlay)
		{
			case 0: _gamePlay = udtGamePlay::VQ3; break;
			case 1: _gamePlay = udtGamePlay::PMC; break;
			case 2: _gamePlay = udtGamePlay::CPM; break;
			case 3: _gamePlay = udtGamePlay::CQ3; break;
			case 4: _gamePlay = udtGamePlay::PMD; break;
			default: break;
		}
	}

	s32 te = -1;
	s32 td = -1;
	if(ParseConfigStringValueInt(te, *_tempAllocator, "te", configString) &&
	   ParseConfigStringValueInt(td, *_tempAllocator, "td", configString))
	{
		const bool oldTimeOut = _timeOut;
		_timeOut = te != 0;
		if(oldTimeOut && !_timeOut)
		{
			++_timeOutCount;
			_totalTimeOutDuration += td;
		}
	}

	s32 tw = -1;
	s32 ts = -1;
	if(ParseConfigStringValueInt(tw, *_tempAllocator, "tw", configString) &&
	   ParseConfigStringValueInt(ts, *_tempAllocator, "ts", configString))
	{
		// CPMA problem:
		// Can go from InProgress to WarmUp for CTFS/CA rounds.
		// So we only keep the very first round start until we get a real match end.
		if(tw == 0)
		{
			UpdateGameState(udtGameState::InProgress);
			if(_matchStartTime == S32_MIN)
			{
				_prevMatchStartTime = _matchStartTime;
				_matchStartTime = ts;
			}
		}
		else if(tw > 0)
		{
			UpdateGameState(udtGameState::CountDown);
		}
		else
		{
			UpdateGameState(udtGameState::WarmUp);
			if(HasMatchJustEnded())
			{
				_matchEndTime = _parser->_inServerTime;
			}
		}
	}

	s32 cr = -1;
	s32 cb = -1;
	if(_gameType >= udtGameType::FirstTeamMode &&
	   _gameState == udtGameState::InProgress &&
	   ParseConfigStringValueInt(cr, *_tempAllocator, "cr", configString) &&
	   ParseConfigStringValueInt(cb, *_tempAllocator, "cb", configString) &&
	   (cr == 0 || cb == 0))
	{
		// If all the players of a team leave during a match, the team forfeits.
		_forfeited = true;
	}
}

void udtGeneralAnalyzer::ProcessCPMARoundInfoConfigString(const char* configString)
{
	if(configString == NULL)
	{
		return;
	}

	udtVMScopedStackAllocator scopedTempAllocator(*_tempAllocator);

	s32 score = 0;
	if((ParseConfigStringValueInt(score, *_tempAllocator, "sr", configString) && score == -9999) ||
	   (ParseConfigStringValueInt(score, *_tempAllocator, "sb", configString) && score == -9999))
	{
		_forfeited = true;
	}
}

void udtGeneralAnalyzer::ProcessQLServerInfoConfigString(const char* configString)
{
	if(configString == NULL)
	{
		return;
	}

	udtVMScopedStackAllocator scopedTempAllocator(*_tempAllocator);

	s32 gamePlay = 0;
	if(ParseConfigStringValueInt(gamePlay, *_tempAllocator, "ruleset", configString))
	{
		switch(gamePlay)
		{
			case 1: _gamePlay = udtGamePlay::CQL; break; // classic
			case 2: _gamePlay = udtGamePlay::PQL; break; // turbo
			case 3: _gamePlay = udtGamePlay::DQL; break; // default
			default: break;
		}
	}

	udtString gameStateString;
	if(!ParseConfigStringValueString(gameStateString, *_tempAllocator, "g_gameState", configString))
	{
		return;
	}

	if(udtString::Equals(gameStateString, "PRE_GAME"))
	{
		UpdateGameState(udtGameState::WarmUp);
	}
	else if(udtString::Equals(gameStateString, "COUNT_DOWN"))
	{
		UpdateGameState(udtGameState::CountDown);
	}
	else if(udtString::Equals(gameStateString, "IN_PROGRESS"))
	{
		UpdateGameState(udtGameState::InProgress);
		if(HasMatchJustStarted() || _processingGameState)
		{
			UpdateMatchStartTime();
		}
	}
}

void udtGeneralAnalyzer::ProcessIntermissionConfigString(const udtString& configString)
{
	// It can be written as "0", "1", "qfalse" and "qtrue".
	if(udtString::EqualsNoCase(configString, "1") || 
	   udtString::EqualsNoCase(configString, "qtrue"))
	{
		UpdateGameState(udtGameState::WarmUp);
		if(HasMatchJustEnded())
		{
			_matchEndTime = _parser->_inServerTime;
		}
	}
}

void udtGeneralAnalyzer::ProcessGameTypeFromServerInfo(const char* configString)
{
	if(configString == NULL)
	{
		return;
	}

	udtVMScopedStackAllocator scopedTempAllocator(*_tempAllocator);

	s32 gameType = 0;
	if(!ParseConfigStringValueInt(gameType, *_tempAllocator, "g_gametype", configString))
	{
		return;
	}

	const s32 udtGT = GetUDTGameTypeFromIdGameType(gameType, _protocol, _game);
	if(udtGT >= 0 && udtGT < (s32)udtGameType::Count)
	{
		_gameType = (udtGameType::Id)udtGT;
	}
}

void udtGeneralAnalyzer::ProcessOSPGamePlayConfigString(const char* configString)
{
	if(configString == NULL)
	{
		return;
	}

	udtVMScopedStackAllocator scopedTempAllocator(*_tempAllocator);

	// When voting "promode 1" in OSP, you get BFG splash damage and 100 damage rails...
	// It's the old ProMode for sure.
	s32 gamePlay = 0;
	if(StringParseInt(gamePlay, configString))
	{
		switch(gamePlay)
		{
			case 0: _gamePlay = udtGamePlay::VQ3; break;
			case 1: _gamePlay = udtGamePlay::PMC; break;
			case 2: _gamePlay = udtGamePlay::CQ3; break;
			default: break;
		}
	}
}

void udtGeneralAnalyzer::ProcessScores2(const char* configString)
{
	if(configString == NULL)
	{
		return;
	}

	// This is to work around old demos sending a score of -9999 on match start.
	// Maybe this is an OSP specialty?
	const s32 matchTime = _parser->_inServerTime - _matchStartTime;

	s32 score = 0;
	if(_gameState == udtGameState::InProgress &&
	   StringParseInt(score, configString) &&
	   score == -9999 && 
	   matchTime >= 1000)
	{
		_forfeited = true;
	}
}

void udtGeneralAnalyzer::ProcessScores2Player(const char* configString)
{
	if(configString == NULL)
	{
		return;
	}

	// The 2nd place player's name becomes empty during a game?
	if(_gameState == udtGameState::InProgress &&
	   configString[0] == '\0')
	{
		// The player left.
		_forfeited = true;
	}
}

void udtGeneralAnalyzer::ProcessQLPauseStartConfigString(const char* configString)
{
	if(configString == NULL)
	{
		return;
	}

	s32 startTime = 0;

	// String cleared means no pause/time-out.
	if(configString[0] == '\0')
	{
		if(_timeOut)
		{
			++_timeOutCount;
		}
		_timeOut = false;
		_serverPause = false;
	}
	else if(StringParseInt(startTime, configString) && 
			startTime > 0)
	{
		_timeOut = true;
	}
}

void udtGeneralAnalyzer::ProcessQLPauseEndConfigString(const char* configString)
{
	if(configString == NULL)
	{
		return;
	}

	s32 endTime = 0;
	if(_timeOut && 
	   StringParseInt(endTime, configString) &&
	   endTime == 0)
	{
		_serverPause = true;
	}
}

void udtGeneralAnalyzer::ProcessModNameAndVersionOnce()
{
	udtVMScopedStackAllocator scopedTempAllocator(*_tempAllocator);
	const char* const serverInfo = _parser->_inConfigStrings[CS_SERVERINFO].String;

	u32 charIndex = 0;
	udtString varValue;

	if(_game == udtGame::CPMA)
	{
		_mod = udtMod::CPMA;

		if(serverInfo != NULL &&
		   ParseConfigStringValueString(varValue, *_tempAllocator, "gameversion", serverInfo))
		{
			_modVersion = udtString::NewCloneFromRef(_stringAllocator, varValue).String;
		}
	}
	else if(_game == udtGame::OSP)
	{
		_mod = udtMod::OSP;

		if(serverInfo != NULL &&
		   ParseConfigStringValueString(varValue, *_tempAllocator, "gameversion", serverInfo))
		{
			u32 openParen = 0;
			u32 closeParen = 0;
			u32 vIndex = 0;
			if(udtString::FindLastCharacterMatch(openParen, varValue, '(') &&
			   udtString::FindLastCharacterMatch(closeParen, varValue, ')') &&
			   openParen < closeParen)
			{
				// OSP TourneyBlaBla v($(version))
				_modVersion = udtString::NewSubstringClone(_stringAllocator, varValue, openParen + 1, closeParen - openParen - 1).String;
			}
			else if(udtString::FindFirstCharacterMatch(vIndex, varValue, 'v'))
			{
				// OSP v$(version)
				_modVersion = udtString::NewSubstringClone(_stringAllocator, varValue, vIndex + 1).String;
			}
			else
			{
				// Unknown OSP version format.
				_modVersion = udtString::NewCloneFromRef(_stringAllocator, varValue).String;
			}
		}
	}
	else if(_mod == udtMod::None)
	{
		if(serverInfo != NULL &&
		   ParseConfigStringValueString(varValue, *_tempAllocator, "gamename", serverInfo) &&
		   udtString::ContainsNoCase(charIndex, varValue, "defrag"))
		{
			_mod = udtMod::Defrag;

			if(ParseConfigStringValueString(varValue, *_tempAllocator, "defrag_vers", serverInfo))
			{
				s32 version = 0;
				if(varValue.Length == 5 && 
				   StringParseInt(version, varValue.String) &&
				   version >= 10000 &&
				   version < 100000)
				{
					// Example: "19123" becomes "1.91.23"
					char stringVersion[16];
					sprintf(stringVersion, "%c.%c%c.%c%c", varValue.String[0], varValue.String[1], varValue.String[2], varValue.String[3], varValue.String[4]);
					_modVersion = udtString::NewClone(_stringAllocator, stringVersion).String;
				}
				else
				{
					// Unknown DeFRaG version format.
					_modVersion = udtString::NewCloneFromRef(_stringAllocator, varValue).String;
				}
			}
		}
	}
}

void udtGeneralAnalyzer::ProcessMapNameOnce()
{
	udtVMScopedStackAllocator scopedTempAllocator(*_tempAllocator);

	const udtString& cs = _parser->GetConfigString(CS_SERVERINFO);
	udtString mapName;
	if(!udtString::IsNullOrEmpty(cs) &&
	   ParseConfigStringValueString(mapName, *_tempAllocator, "mapname", cs.String))
	{
		_mapName = udtString::NewCloneFromRef(_stringAllocator, mapName).String;
	}
}

s32 udtGeneralAnalyzer::GetLevelStartTime()
{
	const s32 csIndex = idConfigStringIndex::LevelStartTime(_protocol);
	const udtString& cs = _parser->GetConfigString(csIndex);
	if(udtString::IsNullOrEmpty(cs))
	{
		return S32_MIN;
	}

	s32 matchStartTimeMs = S32_MIN;
	if(!StringParseInt(matchStartTimeMs, cs.String))
	{
		return S32_MIN;
	}

	return matchStartTimeMs;
}

s32 udtGeneralAnalyzer::GetWarmUpEndTime()
{
	// @TODO: Is the "\time\$(time)" syntax for server pauses?

	// The next match start time is given.
	// When a game starts, the config string gets cleared ("" string).
	// It can either be encoded as "$(time)" or "\time\$(time)". :-(
	// When "\time\$(time)", 0 and -1 have a different meaning it seems.
	const s32 csIndex = idConfigStringIndex::WarmUpEndTime(_protocol);
	const udtString& cs = _parser->GetConfigString(csIndex);
	if(udtString::IsNullOrEmpty(cs))
	{
		return S32_MIN;
	}

	s32 warmUpEndTimeMs = S32_MIN;
	if(StringParseInt(warmUpEndTimeMs, cs.String))
	{
		return warmUpEndTimeMs;
	}

	udtVMScopedStackAllocator scopedTempAllocator(*_tempAllocator);
	if(ParseConfigStringValueInt(warmUpEndTimeMs, *_tempAllocator, "time", cs.String))
	{
		return warmUpEndTimeMs;
	}

	return S32_MIN;
}

bool udtGeneralAnalyzer::IsIntermission()
{
	const s32 csIndex = idConfigStringIndex::Intermission(_protocol);
	const udtString& cs = _parser->GetConfigString(csIndex);
	if(udtString::IsNullOrEmpty(cs))
	{
		return false;
	}

	return udtString::EqualsNoCase(cs, "1") || udtString::EqualsNoCase(cs, "qtrue");
}

void udtGeneralAnalyzer::UpdateMatchStartTime()
{
	const s32 newStartTime = GetLevelStartTime();
	if(_matchEndTime == S32_MIN || newStartTime < _matchEndTime)
	{
		_prevMatchStartTime = _matchStartTime;
		_matchStartTime = newStartTime;
	}
}

void udtGeneralAnalyzer::ResetForNextGameState()
{
	ResetForNextMatch();
	_matchStartTime = S32_MIN;
	_matchEndTime = S32_MIN;
	_prevMatchStartTime = S32_MIN;
	_gameState = udtGameState::WarmUp;
	_lastGameState = udtGameState::WarmUp;
}
