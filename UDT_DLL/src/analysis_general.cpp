#include "analysis_general.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


/*
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
	_stringAllocator.Init((uptr)demoCount * (uptr)(1 << 16));
}

void udtGeneralAnalyzer::ResetForNextDemo()
{
	_modVersion = NULL;
	_mapName = NULL;
	_gameStateIndex = -1;
	_matchStartTime = S32_MIN;
	_matchEndTime = S32_MIN;
	_overTimeCount = 0;
	_game = udtGame::Q3;
	_gameType = udtGameType::Invalid;
	_gameState = udtGameState::WarmUp;
	_lastGameState = udtGameState::WarmUp;
	_mod = udtMod::None;
	_overTimeType = udtOvertimeType::Timed;
	_gamePlay = udtGamePlay::VQ3;
	_protocol = udtProtocol::Invalid;
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
			udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(CS_SERVERINFO);
			if(cs != NULL)
			{
				ProcessQ3ServerInfoConfigString(cs->String);
			}
			ProcessModNameAndVersion();
		}
	}

	ProcessMapName();

	udtBaseParser::udtConfigString* const serverInfoCs = parser.FindConfigStringByIndex(CS_SERVERINFO);
	if(serverInfoCs != NULL)
	{
		ProcessGameTypeConfigString(serverInfoCs->String);
	}

	if(_game == udtGame::CPMA)
	{
		udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(CS_CPMA_GAME_INFO);
		if(cs != NULL)
		{
			ProcessCPMAGameInfoConfigString(cs->String);
		}
	}
	else if(_game == udtGame::QL)
	{
		udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(CS_SERVERINFO);
		if(cs != NULL)
		{
			ProcessQLServerInfoConfigString(cs->String);
		}
	}
	else if(_game == udtGame::Q3 || _game == udtGame::OSP)
	{
		_matchStartTime = GetLevelStartTime();
		const s32 warmUpEndTime = GetWarmUpEndTime();
		const bool noIntermission = !IsIntermission();
		const bool noWarmUpEndTime = warmUpEndTime == S32_MIN || warmUpEndTime < _matchStartTime;
		if(noIntermission && noWarmUpEndTime)
		{
			UpdateGameState(udtGameState::InProgress);
		}
	}

	_processingGameState = false;
}

void udtGeneralAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	CommandLineTokenizer& tokenizer = parser._context->Tokenizer;
	tokenizer.Tokenize(arg.String);

	if(_game == udtGame::CPMA && 
	   tokenizer.GetArgCount() == 2 && 
	   udtString::Equals(tokenizer.GetArg(0), "print"))
	{
		u32 index = 0;
		const udtString printMessage = tokenizer.GetArg(1);
		if(udtString::ContainsNoCase(index, printMessage, "match over") ||
		   udtString::ContainsNoCase(index, printMessage, "timelimit hit"))
		{
			UpdateGameState(udtGameState::WarmUp);
			if(HasMatchJustEnded())
			{
				_matchEndTime = parser._inServerTime;
			}
		}
	}

	if(_game != udtGame::CPMA &&
	   tokenizer.GetArgCount() == 1 &&
	   udtString::Equals(tokenizer.GetArg(0), "map_restart"))
	{
		UpdateGameState(udtGameState::InProgress);
		if(HasMatchJustStarted())
		{
			_matchStartTime = GetLevelStartTime();
		}
	}

	s32 csIndex = 0;
	if(tokenizer.GetArgCount() != 3 || 
	   !udtString::Equals(tokenizer.GetArg(0), "cs") || 
	   !StringParseInt(csIndex, tokenizer.GetArgString(1)))
	{
		return;
	}

	const char* const configString = tokenizer.GetArgString(2);

	if(_game == udtGame::CPMA && csIndex == CS_CPMA_GAME_INFO)
	{
		ProcessCPMAGameInfoConfigString(configString);
	}
	else if(_game != udtGame::CPMA && csIndex == idConfigStringIndex::LevelStartTime(_protocol))
	{
		_matchStartTime = GetLevelStartTime();
	}
	else if(_game == udtGame::QL && csIndex == CS_SERVERINFO)
	{
		ProcessQLServerInfoConfigString(configString);
	}
	else if(_game != udtGame::CPMA && csIndex == idConfigStringIndex::Intermission(_protocol))
	{	
		ProcessIntermissionConfigString(tokenizer.GetArg(2));
	}
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
	return _matchStartTime;
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

void udtGeneralAnalyzer::ProcessQ3ServerInfoConfigString(const char* configString)
{
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
	s32 tw = -1;
	s32 ts = -1;
	udtVMScopedStackAllocator tempAllocScope(*_tempAllocator);
	if(!ParseConfigStringValueInt(tw, *_tempAllocator, "tw", configString) || 
	   !ParseConfigStringValueInt(ts, *_tempAllocator, "ts", configString))
	{
		return;
	}

	// CPMA problem:
	// Can go from InProgress to WarmUp for CTFS/CA rounds.
	// So we only keep the very first round start until we get a real match end.

	if(tw == 0)
	{
		UpdateGameState(udtGameState::InProgress);
		if(_matchStartTime == S32_MIN)
		{
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

void udtGeneralAnalyzer::ProcessQLServerInfoConfigString(const char* configString)
{
	udtVMScopedStackAllocator scopedTempAllocator(*_tempAllocator);

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
		if(HasMatchJustStarted())
		{
			_matchStartTime = GetLevelStartTime();
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

void udtGeneralAnalyzer::ProcessGameTypeConfigString(const char* configString)
{
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

void udtGeneralAnalyzer::ProcessModNameAndVersion()
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

void udtGeneralAnalyzer::ProcessMapName()
{
	udtVMScopedStackAllocator scopedTempAllocator(*_tempAllocator);

	const char* const serverInfo = _parser->_inConfigStrings[CS_SERVERINFO].String;
	udtString mapName;
	if(serverInfo != NULL &&
	   ParseConfigStringValueString(mapName, *_tempAllocator, "mapname", serverInfo))
	{
		_mapName = udtString::NewCloneFromRef(_stringAllocator, mapName).String;
	}
}

s32 udtGeneralAnalyzer::GetLevelStartTime()
{
	const s32 csIndex = idConfigStringIndex::LevelStartTime(_protocol);
	udtBaseParser::udtConfigString* const cs = _parser->FindConfigStringByIndex(csIndex);
	if(cs == NULL)
	{
		return S32_MIN;
	}

	s32 matchStartTimeMs = S32_MIN;
	if(sscanf(cs->String, "%d", &matchStartTimeMs) != 1)
	{
		return S32_MIN;
	}

	return matchStartTimeMs;
}

s32 udtGeneralAnalyzer::GetWarmUpEndTime()
{
	// The next match start time is given.
	// When a game starts, the config string gets cleared ("" string).
	const s32 csIndex = idConfigStringIndex::WarmUpEndTime(_protocol);
	udtBaseParser::udtConfigString* const cs = _parser->FindConfigStringByIndex(csIndex);
	if(cs == NULL)
	{
		return S32_MIN;
	}

	s32 warmUpEndTimeMs = S32_MIN;
	if(sscanf(cs->String, "%d", &warmUpEndTimeMs) != 1)
	{
		return S32_MIN;
	}

	return warmUpEndTimeMs;
}

bool udtGeneralAnalyzer::IsIntermission()
{
	const s32 csIndex = idConfigStringIndex::Intermission(_protocol);
	udtBaseParser::udtConfigString* const cs = _parser->FindConfigStringByIndex(csIndex);
	if(cs == NULL)
	{
		return false;
	}

	const udtString configString = udtString::NewConstRef(cs->String, cs->StringLength);

	return udtString::EqualsNoCase(configString, "1") || udtString::EqualsNoCase(configString, "qtrue");
}
