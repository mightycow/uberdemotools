#include "plug_in_game_state.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


/*
CPMA:
- First score read (gamestate) should be from either the CS_CPMA_GAME_INFO or CS_CPMA_ROUND_INFO config strings
- CS_CPMA_GAME_INFO: tw  -1 means in warm-up           --> ts last restart time --- if ts is 0, the match countdown is starting
- CS_CPMA_GAME_INFO: tw   0 means in match             --> ts match start time
- CS_CPMA_GAME_INFO: tw > 0 means in warm-up countdown --> ts last restart time --- tw countdown end time

BaseQ3:
- No command is sent to signify that the match ended
- When a match starts, Q3 sends the commands "cs 21" (CS_LEVEL_START_TIME) and "map_restart"
- Can only infer the match end time from start time and time/score limits

Quake Live 73 (and 90?):
- cs 0 > g_roundWarmupDelay timeInMs
- cs 0 > g_gameState PRE_GAME COUNT_DOWN IN_PROGRESS 
- When a match ends: cs 14 1 (14 is CS_INTERMISSION_73)
*/


udtParserPlugInGameState::udtParserPlugInGameState() 
	: _gameType(udtGameType::BaseQ3)
	, _gameStateQL(udtGameStateQL::Invalid)
	, _firstGameState(true)
	, _nextSnapshotIsWarmUpEnd(false)
{
	ClearGameState();
	ClearMatch();
}

udtParserPlugInGameState::~udtParserPlugInGameState()
{
}

void udtParserPlugInGameState::InitAllocators(u32 demoCount)
{
	FinalAllocator.Init((uptr)(1 << 16) * (uptr)demoCount);
	_matches.Init((uptr)(1 << 16) * (uptr)demoCount);
	_gameStates.SetAllocator(FinalAllocator);
}

void udtParserPlugInGameState::StartDemoAnalysis()
{
	_gameType = udtGameType::BaseQ3;
	_gameStateQL = udtGameStateQL::Invalid;
	_firstGameState = true;
	_nextSnapshotIsWarmUpEnd = false;

	ClearGameState();
	ClearMatch();
}

void udtParserPlugInGameState::FinishDemoAnalysis()
{
	AddCurrentGameState();
}

void udtParserPlugInGameState::ProcessQlServerInfo(const char* commandString, udtBaseParser& parser)
{
	const udtGameStateQL::Id oldState = _gameStateQL;
	udtGameStateQL::Id newState = udtGameStateQL::Invalid;

	udtVMLinearAllocator& tempAllocator = parser._context->TempAllocator;
	udtVMScopedStackAllocator scopedTempAllocator(tempAllocator);

	char* gameStateString = NULL;
	if(!ParseConfigStringValueString(gameStateString, tempAllocator, "g_gameState", commandString))
	{
		return;
	}

	if(StringEquals(gameStateString, "PRE_GAME"))
	{
		newState = udtGameStateQL::PreGame;
	}
	else if(StringEquals(gameStateString, "COUNT_DOWN"))
	{
		newState = udtGameStateQL::CountDown;
	}
	else if(StringEquals(gameStateString, "IN_PROGRESS"))
	{
		newState = udtGameStateQL::InProgress;
	}
	_gameStateQL = newState;

	if(_currentMatch.WarmUpEndTimeMs == S32_MIN && oldState != udtGameStateQL::CountDown && newState == udtGameStateQL::CountDown)
	{
		_nextSnapshotIsWarmUpEnd = true;
	}

	if(_currentMatch.MatchStartTimeMs == S32_MIN && oldState == udtGameStateQL::CountDown && newState == udtGameStateQL::InProgress)
	{
		_currentMatch.MatchStartTimeMs = parser._inServerTime;
	}
}

void udtParserPlugInGameState::ProcessCpmaGameInfo(const char* commandString, udtBaseParser& parser)
{
	s32 tw = -1;
	s32 ts = -1;
	if(ParseConfigStringValueInt(tw, "tw", commandString) && ParseConfigStringValueInt(ts, "ts", commandString))
	{
		ProcessCpmaTwTs(tw, ts, parser._inServerTime);
	}
}

void udtParserPlugInGameState::ProcessCpmaTwTs(s32 tw, s32 ts, s32 serverTimeMs)
{
	if(_currentMatch.MatchStartTimeMs == S32_MIN && tw == 0)
	{
		_currentMatch.MatchStartTimeMs = ts;
	}

	if(_currentMatch.WarmUpEndTimeMs == S32_MIN && tw == -1 && ts == 0)
	{
		// Might not be able to query the server time right now (if processing a gamestate update).
		// So we just grab the next snapshot's server time.
		_nextSnapshotIsWarmUpEnd = true;
	}
	
	if(_currentMatch.MatchStartTimeMs == S32_MIN && _currentMatch.WarmUpEndTimeMs == S32_MIN && tw > 0)
	{
		_currentMatch.WarmUpEndTimeMs = serverTimeMs;
	}
	
	// We're back to warm-up, so a match might have just ended.
	if(tw == -1 && ts != 0)
	{
		AddCurrentMatchIfValid();
	}
}

void udtParserPlugInGameState::ProcessGamestateMessage(const udtGamestateCallbackArg& /*info*/, udtBaseParser& parser)
{
	if(_firstGameState)
	{
		_gameType = udtGameType::BaseQ3;
		if(parser._protocol >= udtProtocol::Dm73)
		{
			_gameType = udtGameType::QL;
		}
		else
		{
			udtVMLinearAllocator& tempAllocator = parser._context->TempAllocator;
			udtVMScopedStackAllocator scopedTempAllocator(tempAllocator);

			char* gameName = NULL;
			udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(CS_SERVERINFO);
			if(cs != NULL && 
			   ParseConfigStringValueString(gameName, tempAllocator, "gamename", cs->String) &&
			   StringEquals(gameName, "cpma"))
			{
				_gameType = udtGameType::CPMA;
			}
		}
	}
	else
	{
		AddCurrentGameState();
	}
	_firstGameState = false;

	_currentGameState.FileOffset = parser._inFileOffset;
	_currentGameState.Matches = _matches.GetEndAddress();

	if(_gameType == udtGameType::CPMA)
	{
		udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(CS_CPMA_GAME_INFO);
		if(cs != NULL)
		{
			ProcessCpmaGameInfo(cs->String, parser);
		}
	}
	else if(_gameType == udtGameType::QL)
	{
		udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(CS_SERVERINFO);
		if(cs != NULL)
		{
			ProcessQlServerInfo(cs->String, parser);
		}
	}
}

void udtParserPlugInGameState::ProcessSnapshotMessage(const udtSnapshotCallbackArg& /*info*/, udtBaseParser& parser)
{
	_currentGameState.FirstSnapshotTimeMs = udt_min(_currentGameState.FirstSnapshotTimeMs, parser._inServerTime);
	_currentGameState.LastSnapshotTimeMs = udt_max(_currentGameState.LastSnapshotTimeMs, parser._inServerTime);

	if(_gameType == udtGameType::CPMA || _gameType == udtGameType::QL)
	{
		_currentMatch.MatchEndTimeMs = parser._inServerTime;
	}

	if(_nextSnapshotIsWarmUpEnd)
	{
		_nextSnapshotIsWarmUpEnd = false;
		if(_currentMatch.WarmUpEndTimeMs == S32_MIN)
		{
			_currentMatch.WarmUpEndTimeMs = parser._inServerTime;
		}
	}
}

void udtParserPlugInGameState::ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser)
{
	CommandLineTokenizer& tokenizer = parser._context->Tokenizer;
	tokenizer.Tokenize(info.String);
	s32 csIndex = 0;
	if(tokenizer.argc() != 3 || !StringEquals(tokenizer.argv(0), "cs") || !StringParseInt(csIndex, tokenizer.argv(1)))
	{
		return;
	}

	const char* const configString = tokenizer.argv(2);
	if(_gameType == udtGameType::CPMA)
	{
		if(csIndex == CS_CPMA_GAME_INFO)
		{
			ProcessCpmaGameInfo(configString, parser);
		}
	}
	else if(_gameType == udtGameType::BaseQ3)
	{
		if(csIndex == CS_LEVEL_START_TIME)
		{
			int matchStartTimeMs = S32_MIN;
			if(sscanf(configString, "%d", &matchStartTimeMs) == 1)
			{
				AddCurrentMatchIfValid();
				if(_currentMatch.MatchStartTimeMs == S32_MIN)
				{
					_currentMatch.MatchStartTimeMs = matchStartTimeMs;
				}
			}
		}
	}
	else if(_gameType == udtGameType::QL)
	{
		if(csIndex == CS_SERVERINFO)
		{
			ProcessQlServerInfo(configString, parser);
		}
		else if(csIndex == CS_INTERMISSION_73)
		{
			s32 intermissionValue = 0;
			if(StringParseInt(intermissionValue, configString) && intermissionValue == 1)
			{
				AddCurrentMatchIfValid();
			}
		}
	}
}

void udtParserPlugInGameState::ClearMatch()
{
	_currentMatch.WarmUpEndTimeMs = S32_MIN;
	_currentMatch.MatchStartTimeMs = S32_MIN;
	_currentMatch.MatchEndTimeMs = S32_MIN;
}

void udtParserPlugInGameState::ClearGameState()
{
	_currentGameState.FileOffset = 0;
	_currentGameState.FirstSnapshotTimeMs = S32_MAX;
	_currentGameState.LastSnapshotTimeMs = S32_MIN;
	_currentGameState.MatchCount = 0;
	_currentGameState.Matches = NULL;
}

void udtParserPlugInGameState::AddCurrentMatchIfValid()
{
	if(_currentMatch.MatchStartTimeMs == S32_MIN)
	{
		return;
	}

	_matches.Add(_currentMatch);
	++_currentGameState.MatchCount;
	ClearMatch();
}

void udtParserPlugInGameState::AddCurrentGameState()
{
	AddCurrentMatchIfValid();
	_gameStates.Add(_currentGameState);

	ClearGameState();
	ClearMatch();
}
