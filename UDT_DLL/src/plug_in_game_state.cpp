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
}

udtParserPlugInGameState::~udtParserPlugInGameState()
{
}

void udtParserPlugInGameState::InitAllocators(u32 demoCount)
{
	_analyzer.InitAllocators(*TempAllocator, demoCount);
}

void udtParserPlugInGameState::CopyBuffersStruct(void* buffersStruct) const
{
	*(udtParseDataGameStateBuffers*)buffersStruct = _buffers;
}

void udtParserPlugInGameState::UpdateBufferStruct()
{
	_buffers.GameStateRanges = BufferRanges.GetStartAddress();
	_buffers.GameStateCount = _gameStates.GetSize();
	_buffers.GameStates = _gameStates.GetStartAddress();
	_buffers.Players = _players.GetStartAddress();
	_buffers.PlayerCount = _players.GetSize();
	_buffers.Matches = _matches.GetStartAddress();
	_buffers.MatchCount = _matches.GetSize();
	_buffers.KeyValuePairs = _keyValuePairs.GetStartAddress();
	_buffers.KeyValuePairCount = _keyValuePairs.GetSize();
	_buffers.StringBuffer = _stringAllocator.GetStartAddress();
	_buffers.StringBufferSize = (u32)_stringAllocator.GetCurrentByteCount();
}

u32 udtParserPlugInGameState::GetItemCount() const
{
	return _gameStates.GetSize();
}

void udtParserPlugInGameState::StartDemoAnalysis()
{
	_protocol = udtProtocol::Invalid;

	_analyzer.ResetForNextDemo();

	ClearGameState();
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
	_currentGameState.FirstMatchIndex = _matches.GetSize();
	_currentGameState.FirstKeyValuePairIndex = _keyValuePairs.GetSize();
	_currentGameState.FirstPlayerIndex = _players.GetSize();

	const udtString systemInfoString = parser.GetConfigString(CS_SYSTEMINFO);
	const udtString serverInfoString = parser.GetConfigString(CS_SERVERINFO);
	const udtString backslashString = udtString::NewConstRef("\\");
	const udtString* systemAndServerStringParts[3] = { &systemInfoString, &serverInfoString, &backslashString };
	const udtString systemAndServerString = udtString::NewFromConcatenatingMultiple(_stringAllocator, systemAndServerStringParts, (u32)UDT_COUNT_OF(systemAndServerStringParts));
	ProcessDemoTakerName(info.ClientNum, parser._inConfigStrings, parser._inProtocol);
	ProcessSystemAndServerInfo(systemAndServerString);

	const s32 playerCSBaseIndex = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, parser._inProtocol);
	for(s32 i = 0; i < 64; ++i)
	{
		ProcessPlayerInfo(i, parser._inConfigStrings[playerCSBaseIndex + i], UDT_S32_MIN);
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

	const idTokenizer& tokenizer = parser.GetTokenizer();
	s32 csIndex = 0;
	if(tokenizer.GetArgCount() != 3 || 
	   !udtString::Equals(tokenizer.GetArg(0), "cs") || 
	   !StringParseInt(csIndex, tokenizer.GetArgString(1)))
	{
		return;
	}

	const s32 firstPlayerCsIndex = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, _protocol);
	if(csIndex >= firstPlayerCsIndex && csIndex < firstPlayerCsIndex + 64)
	{
		ProcessPlayerInfo(csIndex - firstPlayerCsIndex, parser._inConfigStrings[csIndex], parser._inServerTime);
	}
}

void udtParserPlugInGameState::ClearPlayerInfos()
{
	for(s32 i = 0; i < 64; ++i)
	{
		_playerInfos[i].FirstName = UDT_U32_MAX;
		_playerInfos[i].FirstNameLength = 0;
		_playerInfos[i].FirstSnapshotTimeMs = UDT_S32_MAX;
		_playerInfos[i].LastSnapshotTimeMs = UDT_S32_MIN;
		_playerInfos[i].Index = -1;
		_playerInfos[i].FirstTeam = (u32)-1;
		_playerConnected[i] = false;
	}
}

void udtParserPlugInGameState::ClearGameState()
{
	_currentGameState.FileOffset = 0;
	_currentGameState.FirstSnapshotTimeMs = UDT_S32_MAX;
	_currentGameState.LastSnapshotTimeMs = UDT_S32_MIN;
	_currentGameState.FirstMatchIndex = 0;
	_currentGameState.MatchCount = 0;
	_currentGameState.FirstKeyValuePairIndex = 0;
	_currentGameState.KeyValuePairCount = 0;
	_currentGameState.FirstPlayerIndex = 0;
	_currentGameState.PlayerCount = 0;
}

void udtParserPlugInGameState::AddCurrentMatchIfValid(bool addIfInProgress)
{
	const bool addMatch = _analyzer.HasMatchJustEnded() || 
		(addIfInProgress && _analyzer.IsMatchInProgress());
	if(!addMatch)
	{
		return;
	}

	udtMatchInfo match;
	match.MatchStartTimeMs = _analyzer.MatchStartTime();
	match.MatchEndTimeMs = _analyzer.MatchEndTime();
	match.WarmUpEndTimeMs = UDT_S32_MIN;
	
	if(_currentGameState.MatchCount > 0 &&
	   _matches[_matches.GetSize() - 1].MatchEndTimeMs >= match.MatchStartTimeMs)
	{
		return;
	}
	
	_matches.Add(match);
	++_currentGameState.MatchCount;
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
	AddCurrentMatchIfValid(true);
	AddCurrentPlayersIfValid();
	_gameStates.Add(_currentGameState);

	ClearGameState();
	ClearPlayerInfos();
}

void udtParserPlugInGameState::ProcessDemoTakerName(s32 playerIndex, const udtString* configStrings, udtProtocol::Id protocol)
{
	_currentGameState.DemoTakerPlayerIndex = playerIndex;
	_currentGameState.DemoTakerName = UDT_U32_MAX; // Not available in all demo protocols.
	_currentGameState.DemoTakerNameLength = 0;

	if(playerIndex < 0 || playerIndex >= ID_MAX_CLIENTS)
	{
		return;
	}

	const s32 firstPlayerCsIndex = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, protocol);
	const udtString& cs = configStrings[firstPlayerCsIndex + playerIndex];
	if(udtString::IsNullOrEmpty(cs))
	{
		return;
	}

	udtVMScopedStackAllocator allocatorScope(*TempAllocator);

	udtString clan, name;
	bool hasClan;
	if(GetClanAndPlayerName(clan, name, hasClan, *TempAllocator, protocol, cs.GetPtr()))
	{
		WriteStringToApiStruct(_currentGameState.DemoTakerName, udtString::NewCleanCloneFromRef(_stringAllocator, protocol, name));
	}
}

void udtParserPlugInGameState::ProcessSystemAndServerInfo(const udtString& configStrings)
{
	char* const searchString = configStrings.GetWritePtr();
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
			info.Name = configStrings.GetOffset() + keyStart;
			info.NameLength = (u32)strlen(_stringAllocator.GetStringAt(info.Name));
			info.Value = configStrings.GetOffset() + valueStart;
			info.ValueLength = (u32)strlen(_stringAllocator.GetStringAt(info.Value));
			_keyValuePairs.Add(info);
		}

		keyStart = valueEnd + 1;
	}

	_currentGameState.KeyValuePairCount = _keyValuePairs.GetSize() - previousCount;
}

void udtParserPlugInGameState::ProcessPlayerInfo(s32 playerIndex, const udtString& configString, s32 serverTimeMs)
{
	udtVMScopedStackAllocator tempAllocScope(*TempAllocator);

	const bool csValid = !udtString::IsNullOrEmpty(configString);
	const bool connected = _playerConnected[playerIndex];

	// Player connected?
	if(csValid && !connected)
	{
		udtString clan, name, finalName;
		bool hasClan;
		if(!GetClanAndPlayerName(clan, name, hasClan, *TempAllocator, _protocol, configString.GetPtr()))
		{
			finalName = udtString::NewClone(_stringAllocator, "N/A");
		}
		else
		{
			finalName = udtString::NewCleanCloneFromRef(_stringAllocator, _protocol, name);
		}

		s32 team = -1;
		if(!ParseConfigStringValueInt(team, *TempAllocator, "t", configString.GetPtr()))
		{
			team = -1;
		}

		_playerInfos[playerIndex].Index = playerIndex;
		WriteStringToApiStruct(_playerInfos[playerIndex].FirstName, finalName);
		_playerInfos[playerIndex].FirstTeam = team;
		if(serverTimeMs != UDT_S32_MIN)
		{
			_playerInfos[playerIndex].FirstSnapshotTimeMs = serverTimeMs;
			_playerInfos[playerIndex].LastSnapshotTimeMs = serverTimeMs;
		}

		_playerConnected[playerIndex] = true;
	}
	// Player disconnected?
	else if(!csValid && connected)
	{
		_players.Add(_playerInfos[playerIndex]);
		++_currentGameState.PlayerCount;

		_playerInfos[playerIndex].Index = -1;
		_playerInfos[playerIndex].FirstName = UDT_U32_MAX;
		_playerInfos[playerIndex].FirstNameLength = 0;
		_playerInfos[playerIndex].FirstSnapshotTimeMs = UDT_S32_MAX;
		_playerInfos[playerIndex].LastSnapshotTimeMs = UDT_S32_MIN;
		_playerInfos[playerIndex].FirstTeam = (u32)-1;

		_playerConnected[playerIndex] = false;
	}
}
