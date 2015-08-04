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

void udtGeneralAnalyzer::SetTempAllocator(udtVMLinearAllocator& tempAllocator)
{
	_tempAllocator = &tempAllocator;
}

void udtGeneralAnalyzer::ResetForNextDemo()
{
	_gameStateIndex = -1;
	_matchStartTime = S32_MIN;
	_matchEndTime = S32_MIN;
	_gameType = udtGameType::Q3;
	_gameState = udtGameState::WarmUp;
	_lastGameState = udtGameState::WarmUp;
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

void udtGeneralAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	arg;
	parser;

	if(_lastGameState == _gameState && 
	   _gameState != udtGameState::InProgress)
	{
		// Do it ourselves so the user doesn't have to.
		ClearMatch();
	}
}

void udtGeneralAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	arg;
	parser;

	++_gameStateIndex;
	_parser = &parser;
	_protocol = parser._inProtocol;

	_processingGameState = true;

	// First game state message?
	if(_gameStateIndex == 0)
	{
		_gameType = udtGameType::Q3;
		if(_protocol >= udtProtocol::Dm73)
		{
			_gameType = udtGameType::QL;
		}
		else
		{
			udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(CS_SERVERINFO);
			if(cs != NULL)
			{
				ProcessQ3ServerInfoConfigString(cs->String);
			}
		}
	}

	if(_gameType == udtGameType::CPMA)
	{
		udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(CS_CPMA_GAME_INFO);
		if(cs != NULL)
		{
			ProcessCPMAGameInfoConfigString(cs->String);
		}
	}
	else if(_gameType == udtGameType::QL)
	{
		udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(CS_SERVERINFO);
		if(cs != NULL)
		{
			ProcessQLServerInfoConfigString(cs->String);
		}
	}
	else if(_gameType == udtGameType::Q3 || _gameType == udtGameType::OSP)
	{
		_matchStartTime = GetLevelStartTime();
	}

	_processingGameState = false;
}

void udtGeneralAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	arg;
	parser;

	CommandLineTokenizer& tokenizer = parser._context->Tokenizer;
	tokenizer.Tokenize(arg.String);

	if(_gameType == udtGameType::CPMA && 
	   tokenizer.GetArgCount() == 2 && 
	   udtString::Equals(tokenizer.GetArg(0), "print"))
	{
		u32 index = 0;
		const udtString printMessage = tokenizer.GetArg(1);
		if(udtString::ContainsNoCase(index, printMessage, "match over") ||
		   udtString::ContainsNoCase(index, printMessage, "timelimit hit"))
		{
			UpdateGameState(udtGameState::WarmUp);
			if(MatchJustEnded())
			{
				_matchEndTime = parser._inServerTime;
			}
		}
	}

	if(_gameType != udtGameType::CPMA &&
	   tokenizer.GetArgCount() == 1 &&
	   udtString::Equals(tokenizer.GetArg(0), "map_restart"))
	{
		UpdateGameState(udtGameState::InProgress);
		if(MatchJustStarted())
		{
			_matchStartTime = GetLevelStartTime();
			_matchEndTime = S32_MIN;
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

	if(_gameType == udtGameType::CPMA && csIndex == CS_CPMA_GAME_INFO)
	{
		ProcessCPMAGameInfoConfigString(configString);
	}
	else if(_gameType != udtGameType::CPMA && csIndex == idConfigStringIndex::LevelStartTime(_protocol))
	{
		_matchStartTime = GetLevelStartTime();
	}
	else if(_gameType == udtGameType::QL && csIndex == CS_SERVERINFO)
	{
		ProcessQLServerInfoConfigString(configString);
	}
	else if(_gameType != udtGameType::CPMA && csIndex == idConfigStringIndex::Intermission(_protocol))
	{	
		ProcessIntermissionConfigString(tokenizer.GetArg(2));
	}
}

bool udtGeneralAnalyzer::MatchJustStarted() const
{
	return _lastGameState != udtGameState::InProgress && _gameState == udtGameState::InProgress;
}

bool udtGeneralAnalyzer::MatchJustEnded() const
{
	// The stricter rule is to avoid CPMA round endings to look like match endings.
	return _lastGameState == udtGameState::InProgress && _gameState == udtGameState::WarmUp;
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

void udtGeneralAnalyzer::SetInWarmUp()
{
	_lastGameState = udtGameState::WarmUp;
	_gameState = udtGameState::WarmUp;
	_matchStartTime = S32_MIN;
	_matchEndTime = S32_MIN;
}

void udtGeneralAnalyzer::ClearMatch()
{
	_matchStartTime = S32_MIN;
	_matchEndTime = S32_MIN;
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
	udtString gameName;
	if(ParseConfigStringValueString(gameName, *_tempAllocator, "gamename", configString) &&
	   udtString::Equals(gameName, "cpma"))
	{
		_gameType = udtGameType::CPMA;
	}
	else if(ParseConfigStringValueString(gameName, *_tempAllocator, "gamename", configString) &&
			udtString::ContainsNoCase(charIndex, gameName, "osp"))
	{
		_gameType = udtGameType::OSP;
	}
	else if(ParseConfigStringValueString(gameName, *_tempAllocator, "gameversion", configString) &&
			udtString::ContainsNoCase(charIndex, gameName, "osp"))
	{
		_gameType = udtGameType::OSP;
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
		_matchEndTime = S32_MIN;
		if(_matchStartTime == S32_MIN)
		{
			_matchStartTime = ts;
		}
	}
	else if(tw > 0)
	{
		UpdateGameState(udtGameState::CountDown);
		_matchStartTime = S32_MIN;
		_matchEndTime = S32_MIN;
	}
	else
	{
		UpdateGameState(udtGameState::WarmUp);
		if(MatchJustEnded())
		{
			_matchEndTime = _parser->_inServerTime;
		}
		else
		{
			_matchStartTime = S32_MIN;
			_matchEndTime = S32_MIN;
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
		_matchStartTime = S32_MIN;
		_matchEndTime = S32_MIN;
		UpdateGameState(udtGameState::WarmUp);
	}
	else if(udtString::Equals(gameStateString, "COUNT_DOWN"))
	{
		_matchStartTime = S32_MIN;
		_matchEndTime = S32_MIN;
		UpdateGameState(udtGameState::CountDown);
	}
	else if(udtString::Equals(gameStateString, "IN_PROGRESS"))
	{
		_matchEndTime = S32_MIN;
		UpdateGameState(udtGameState::InProgress);
		if(MatchJustStarted())
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
		if(MatchJustEnded())
		{
			_matchEndTime = _parser->_inServerTime;
		}
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
	if(sscanf(cs->String, "%d", &matchStartTimeMs) == 1)
	{
		_matchStartTime = (s32)matchStartTimeMs;
	}

	return (s32)matchStartTimeMs;
}
