#include "plug_in_game_state.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


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
{
	_protocol = udtProtocol::Invalid;

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

	_analyzer.SetTempAllocator(*TempAllocator);
}

void udtParserPlugInGameState::StartDemoAnalysis()
{
	_protocol = udtProtocol::Invalid;

	_analyzer.ResetForNextDemo();

	ClearGameState();
	ClearMatch();
	ClearPlayerInfos();
}

void udtParserPlugInGameState::FinishDemoAnalysis()
{
	_analyzer.FinishDemoAnalysis();

	AddCurrentGameState();
}

void udtParserPlugInGameState::ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser)
{
	_analyzer.ProcessGamestateMessage(info, parser);
	if(_analyzer.GameStateIndex() > 0)
	{
		AddCurrentGameState();
	}

	_protocol = parser._inProtocol;

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
}

void udtParserPlugInGameState::ProcessSnapshotMessage(const udtSnapshotCallbackArg& info, udtBaseParser& parser)
{
	_analyzer.ProcessSnapshotMessage(info, parser);

	_currentGameState.FirstSnapshotTimeMs = udt_min(_currentGameState.FirstSnapshotTimeMs, parser._inServerTime);
	_currentGameState.LastSnapshotTimeMs = udt_max(_currentGameState.LastSnapshotTimeMs, parser._inServerTime);

	for(s32 i = 0; i < 64; ++i)
	{
		_playerInfos[i].FirstSnapshotTimeMs = udt_min(_playerInfos[i].FirstSnapshotTimeMs, parser._inServerTime);
		_playerInfos[i].LastSnapshotTimeMs = udt_max(_playerInfos[i].LastSnapshotTimeMs, parser._inServerTime);
	}
}

void udtParserPlugInGameState::ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser)
{
	_analyzer.ProcessCommandMessage(info, parser);
	AddCurrentMatchIfValid();

	CommandLineTokenizer& tokenizer = parser._context->Tokenizer;
	tokenizer.Tokenize(info.String);
	s32 csIndex = 0;
	if(tokenizer.GetArgCount() != 3 || 
	   !udtString::Equals(tokenizer.GetArg(0), "cs") || 
	   !StringParseInt(csIndex, tokenizer.GetArgString(1)))
	{
		return;
	}

	const s32 firstPlayerCsIndex = idConfigStringIndex::FirstPlayer(_protocol);
	if(csIndex >= firstPlayerCsIndex && csIndex < firstPlayerCsIndex + 64)
	{
		ProcessPlayerInfo(csIndex - firstPlayerCsIndex, parser._inConfigStrings[csIndex]);
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
	if(!_analyzer.HasMatchJustEnded())
	{
		return;
	}

	_currentMatch.MatchStartTimeMs = _analyzer.MatchStartTime();
	_currentMatch.MatchEndTimeMs = _analyzer.MatchEndTime();
	_currentMatch.WarmUpEndTimeMs = S32_MIN;

	_matches.Add(_currentMatch);
	++_currentGameState.MatchCount;
	ClearMatch();

	_analyzer.SetInWarmUp();
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
