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


static const char* FilteredKeys[] =
{
	"sv_referencedPakNames",
	"sv_referencedPaks",
	"sv_pakNames",
	"sv_paks" // QL only.
};

static bool IsInterestingKey(const char* keyName)
{
	const udtString keyNameString = udtString::NewConstRef(keyName);
	for(u32 i = 0; i < (u32)UDT_COUNT_OF(FilteredKeys); ++i)
	{
		if(udtString::EqualsNoCase(keyNameString, FilteredKeys[i]))
		{
			return false;
		}
	}

	return true;
}


udtParserPlugInGameState::udtParserPlugInGameState() 
	: _gameType(udtGameType::BaseQ3)
	, _gameStateQL(udtGameStateQL::Invalid)
	, _protocol(udtProtocol::Invalid)
	, _firstGameState(true)
	, _nextSnapshotIsWarmUpEnd(false)
{
	ClearGameState();
	ClearPlayerInfos();
	ClearMatch();
}

udtParserPlugInGameState::~udtParserPlugInGameState()
{
}

void udtParserPlugInGameState::InitAllocators(u32 demoCount)
{
	// With the string allocator, we allocate:
	// - 2 config strings
	// - the demo taker's name
	// - player names for everyone who connects during the game
	FinalAllocator.Init((uptr)(1 << 16) * (uptr)demoCount);
	_matches.Init((uptr)(1 << 16) * (uptr)demoCount);
	_keyValuePairs.Init((uptr)(1 << 16) * (uptr)demoCount);
	_players.Init((uptr)(1 << 16) * (uptr)demoCount);
	_stringAllocator.Init((uptr)(1 << 16) * (uptr)demoCount);
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
	ClearPlayerInfos();
}

void udtParserPlugInGameState::FinishDemoAnalysis()
{
	AddCurrentGameState();
}

void udtParserPlugInGameState::ProcessQlServerInfo(const char* commandString, udtBaseParser& parser)
{
	const udtGameStateQL::Id oldState = _gameStateQL;
	udtGameStateQL::Id newState = udtGameStateQL::Invalid;

	udtVMLinearAllocator& tempAllocator = *TempAllocator;
	udtVMScopedStackAllocator scopedTempAllocator(tempAllocator);

	udtString gameStateString;
	if(!ParseConfigStringValueString(gameStateString, tempAllocator, "g_gameState", commandString))
	{
		return;
	}

	if(udtString::Equals(gameStateString, "PRE_GAME"))
	{
		newState = udtGameStateQL::PreGame;
	}
	else if(udtString::Equals(gameStateString, "COUNT_DOWN"))
	{
		newState = udtGameStateQL::CountDown;
	}
	else if(udtString::Equals(gameStateString, "IN_PROGRESS"))
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
	udtVMScopedStackAllocator tempAllocScope(*TempAllocator);
	if(ParseConfigStringValueInt(tw, *TempAllocator, "tw", commandString) && ParseConfigStringValueInt(ts, *TempAllocator, "ts", commandString))
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

void udtParserPlugInGameState::ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser)
{
	_protocol = parser._inProtocol;

	if(_firstGameState)
	{
		_gameType = udtGameType::BaseQ3;
		if(parser._inProtocol >= udtProtocol::Dm73)
		{
			_gameType = udtGameType::QL;
		}
		else
		{
			udtVMLinearAllocator& tempAllocator = *TempAllocator;
			udtVMScopedStackAllocator scopedTempAllocator(tempAllocator);

			udtString gameName;
			udtBaseParser::udtConfigString* const cs = parser.FindConfigStringByIndex(CS_SERVERINFO);
			if(cs != NULL && 
			   ParseConfigStringValueString(gameName, tempAllocator, "gamename", cs->String) &&
			   udtString::Equals(gameName, "cpma"))
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
	_currentGameState.KeyValuePairs = _keyValuePairs.GetEndAddress();
	_currentGameState.Players = _players.GetEndAddress();

	const udtBaseParser::udtConfigString& systemInfoCs = parser._inConfigStrings[CS_SYSTEMINFO];
	const udtBaseParser::udtConfigString& serverInfoCs = parser._inConfigStrings[CS_SERVERINFO];
	const udtString systemInfoString = udtString::NewConstRef(systemInfoCs.String, systemInfoCs.StringLength);
	const udtString serverInfoString = udtString::NewConstRef(serverInfoCs.String, serverInfoCs.StringLength);
	const udtString backslashString = udtString::NewConstRef("\\");
	const udtString* systemAndServerStringParts[3] = { &systemInfoString, &serverInfoString, &backslashString };
	const udtString systemAndServerString = udtString::NewFromConcatenatingMultiple(_stringAllocator, systemAndServerStringParts, (u32)UDT_COUNT_OF(systemAndServerStringParts));
	ProcessDemoTakerName(info.ClientNum, parser._inConfigStrings, parser._inProtocol);
	ProcessSystemAndServerInfo(systemAndServerString);

	const s32 playerCSBaseIndex = idConfigStringIndex::FirstPlayer(parser._inProtocol);
	for(s32 i = 0; i < 64; ++i)
	{
		ProcessPlayerInfo(i, parser._inConfigStrings[playerCSBaseIndex + i]);
	}

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

	for(s32 i = 0; i < 64; ++i)
	{
		_playerInfos[i].FirstSnapshotTimeMs = udt_min(_playerInfos[i].FirstSnapshotTimeMs, parser._inServerTime);
		_playerInfos[i].LastSnapshotTimeMs = udt_max(_playerInfos[i].LastSnapshotTimeMs, parser._inServerTime);
	}
}

void udtParserPlugInGameState::ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser)
{
	CommandLineTokenizer& tokenizer = parser._context->Tokenizer;
	tokenizer.Tokenize(info.String);
	s32 csIndex = 0;
	if(tokenizer.GetArgCount() != 3 || !udtString::Equals(tokenizer.GetArg(0), "cs") || !StringParseInt(csIndex, tokenizer.GetArgString(1)))
	{
		return;
	}

	const char* const configString = tokenizer.GetArgString(2);
	if(_gameType == udtGameType::QL && csIndex >= CS_PLAYERS_73p && csIndex < CS_PLAYERS_73p + 64)
	{
		ProcessPlayerInfo(csIndex - CS_PLAYERS_73p, parser._inConfigStrings[csIndex]);
	}
	else if(_gameType != udtGameType::QL && csIndex >= CS_PLAYERS_68 && csIndex < CS_PLAYERS_68 + 64)
	{
		ProcessPlayerInfo(csIndex - CS_PLAYERS_68, parser._inConfigStrings[csIndex]);
	}

	if(_gameType == udtGameType::CPMA)
	{
		if(csIndex == CS_CPMA_GAME_INFO)
		{
			ProcessCpmaGameInfo(configString, parser);
		}
	}
	else if(_gameType == udtGameType::BaseQ3)
	{
		if(csIndex == CS_LEVEL_START_TIME_68)
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
		else if(csIndex == idConfigStringIndex::Intermission(_protocol))
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

void udtParserPlugInGameState::ClearPlayerInfos()
{
	for(s32 i = 0; i < 64; ++i)
	{
		_playerInfos[i].FirstName = NULL;
		_playerInfos[i].FirstSnapshotTimeMs = S32_MAX;
		_playerInfos[i].LastSnapshotTimeMs = S32_MIN;
		_playerInfos[i].Index = -1;
		_playerInfos[i].FirstTeam = (u32)-1;
	}
}

void udtParserPlugInGameState::ClearGameState()
{
	_currentGameState.FileOffset = 0;
	_currentGameState.FirstSnapshotTimeMs = S32_MAX;
	_currentGameState.LastSnapshotTimeMs = S32_MIN;
	_currentGameState.MatchCount = 0;
	_currentGameState.Matches = NULL;
	_currentGameState.KeyValuePairCount = 0;
	_currentGameState.KeyValuePairs = NULL;
	_currentGameState.PlayerCount = 0;
	_currentGameState.Players = NULL;
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

void udtParserPlugInGameState::AddCurrentPlayersIfValid()
{
	for(s32 i = 0; i < 64; ++i)
	{
		if(_playerInfos[i].Index == i)
		{
			_players.Add(_playerInfos[i]);
			++_currentGameState.PlayerCount;
		}
	}

	ClearPlayerInfos();
}

void udtParserPlugInGameState::AddCurrentGameState()
{
	AddCurrentMatchIfValid();
	AddCurrentPlayersIfValid();
	_gameStates.Add(_currentGameState);

	ClearGameState();
	ClearMatch();
	ClearPlayerInfos();
}

void udtParserPlugInGameState::ProcessDemoTakerName(s32 playerIndex, const udtBaseParser::udtConfigString* configStrings, udtProtocol::Id protocol)
{
	_currentGameState.DemoTakerPlayerIndex = playerIndex;
	_currentGameState.DemoTakerName = "N/A"; // Pessimism...

	if(playerIndex < 0 || playerIndex >= MAX_CLIENTS)
	{
		return;
	}

	const s32 firstPlayerCsIndex = idConfigStringIndex::FirstPlayer(protocol);
	const udtBaseParser::udtConfigString cs = configStrings[firstPlayerCsIndex + playerIndex];
	if(cs.String == NULL || cs.StringLength == 0)
	{
		return;
	}

	udtString name;
	if(ParseConfigStringValueString(name, _stringAllocator, "n", cs.String))
	{
		udtString::CleanUp(name, protocol);
		_currentGameState.DemoTakerName = name.String;
	}
}

void udtParserPlugInGameState::ProcessSystemAndServerInfo(const udtString& configStrings)
{
	char* const searchString = configStrings.String;
	const u32 previousCount = _keyValuePairs.GetSize();
	u32 keyStart = 1;
	for(;;)
	{
		u32 keyEnd = 0;
		if(!udtString::FindFirstCharacterMatch(keyEnd, udtString::NewSubstringRef(configStrings, keyStart), '\\'))
		{
			break;
		}
		keyEnd += keyStart;
		const u32 valueStart = keyEnd + 1;

		u32 valueEnd = 0;
		if(!udtString::FindFirstCharacterMatch(valueEnd, udtString::NewSubstringRef(configStrings, valueStart), '\\'))
		{
			break;
		}
		valueEnd += valueStart;

		searchString[keyEnd] = '\0';
		searchString[valueEnd] = '\0';

		if(IsInterestingKey(searchString + keyStart))
		{
			udtGameStateKeyValuePair info;
			info.Name = searchString + keyStart;
			info.Value = searchString + valueStart;
			_keyValuePairs.Add(info);
		}

		keyStart = valueEnd + 1;
	}

	_currentGameState.KeyValuePairCount = _keyValuePairs.GetSize() - previousCount;
}

void udtParserPlugInGameState::ProcessPlayerInfo(s32 playerIndex, const udtBaseParser::udtConfigString& configString)
{
	udtVMScopedStackAllocator tempAllocScope(*TempAllocator);

	// Player connected?
	if(_playerInfos[playerIndex].Index != playerIndex && configString.String != NULL && configString.StringLength > 0)
	{
		udtString name;
		if(!ParseConfigStringValueString(name, _stringAllocator, "n", configString.String))
		{
			name = udtString::NewConstRef("N/A");
		}
		else
		{
			udtString::CleanUp(name, _protocol);
		}

		s32 team = -1;
		if(!ParseConfigStringValueInt(team, *TempAllocator, "t", configString.String))
		{
			team = -1;
		}

		_playerInfos[playerIndex].Index = playerIndex;
		_playerInfos[playerIndex].FirstName = name.String;
		_playerInfos[playerIndex].FirstTeam = team;
	}
	// Player disconnected?
	else if(_playerInfos[playerIndex].Index == playerIndex && (configString.String == NULL || configString.StringLength == 0))
	{
		_players.Add(_playerInfos[playerIndex]);
		++_currentGameState.PlayerCount;

		_playerInfos[playerIndex].Index = -1;
		_playerInfos[playerIndex].FirstName = NULL;
		_playerInfos[playerIndex].FirstSnapshotTimeMs = S32_MAX;
		_playerInfos[playerIndex].LastSnapshotTimeMs = S32_MIN;
		_playerInfos[playerIndex].FirstTeam = (u32)-1;
	}
}
