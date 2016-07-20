#include "plug_in_scores.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


#define  CS_SCORE_1  6
#define  CS_SCORE_2  7
#define  CS_QL_SCORE_NAME_1  659
#define  CS_QL_SCORE_NAME_2  660
#define  CS_PERS_IDX_SCORE  0
#define  SCORE_NO_ONE  -9999


static void ParseConfigStringInt(s32& value, udtBaseParser& parser, s32 csIndex)
{
	const char* const cs = parser.GetConfigString(csIndex).GetPtr();
	if(cs == NULL)
	{
		return;
	}

	StringParseInt(value, cs);
}

static void CloneConfigString(udtString& string, udtVMLinearAllocator& alloc, udtBaseParser& parser, s32 csIndex)
{
	const udtString cs = parser.GetConfigString(csIndex);
	if(udtString::IsNullOrEmpty(cs))
	{
		string = udtString::NewConstRef("");
		return;
	}

	string = udtString::NewCloneFromRef(alloc, cs);
}

static bool HasClanName(udtProtocol::Id protocol)
{
	return protocol >= udtProtocol::Dm73 && protocol <= udtProtocol::Dm90;
}


udtParserPlugInScores::udtParserPlugInScores()
{
}

udtParserPlugInScores::~udtParserPlugInScores()
{
}

void udtParserPlugInScores::InitAllocators(u32)
{
}

void udtParserPlugInScores::CopyBuffersStruct(void* buffersStruct) const
{
	*(udtParseDataScoreBuffers*)buffersStruct = _buffers;
}

void udtParserPlugInScores::UpdateBufferStruct()
{
	_buffers.ScoreCount = _scores.GetSize();
	_buffers.ScoreRanges = BufferRanges.GetStartAddress();
	_buffers.Scores = _scores.GetStartAddress();
	_buffers.StringBuffer = _stringAllocator.GetStartAddress();
	_buffers.StringBufferSize = (u32)_stringAllocator.GetCurrentByteCount();
}

u32 udtParserPlugInScores::GetItemCount() const
{
	return _scores.GetSize();
}

void udtParserPlugInScores::StartDemoAnalysis()
{
	memset(_players, 0, sizeof(_players));
	_parser = NULL;
	_gameStateIndex = -1;
	_gameType = udtGameType::Count;
	_protocol = udtProtocol::Invalid;
	_mod = udtMod::None;
	_tempAllocator.Clear();
}

void udtParserPlugInScores::FinishDemoAnalysis()
{
	// @NOTE: The current range hasn't been added yet.
	const u32 rangeCount = BufferRanges.GetSize();
	const u32 firstIdx = rangeCount == 0 ? 0 : BufferRanges[rangeCount - 1].FirstIndex + BufferRanges[rangeCount - 1].Count;
	if(firstIdx >= _scores.GetSize())
	{
		return;
	}

	if(firstIdx + 1 < _scores.GetSize() &&
	   _scores[firstIdx].Score1 == _scores[firstIdx + 1].Score1 &&
	   _scores[firstIdx].Score2 == _scores[firstIdx + 1].Score2)
	{
		_scores.Remove(firstIdx);
	}
	else
	{
		_scores[firstIdx].ServerTimeMs = _firstSnapshotTimeMs;
	}
}

void udtParserPlugInScores::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& parser)
{
	const s32 csIndexFirstPlayer = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, parser._inProtocol, _mod);
	_score1 = SCORE_NO_ONE;
	_score2 = SCORE_NO_ONE;
	_clientNumber1 = 64;
	_clientNumber2 = 64;
	_followedNumber = 64;
	_followedScore = SCORE_NO_ONE;
	_firstSnapshotTimeMs = UDT_S32_MIN;
	_name1 = udtString::NewNull();
	_name2 = udtString::NewNull();
	_parser = &parser;
	++_gameStateIndex;
	_protocol = parser._inProtocol;
	if(_protocol <= udtProtocol::Dm68)
	{
		DetectMod();
	}
	DetectGameType();
	for(u32 i = 0; i < 64; ++i)
	{
		ProcessPlayerConfigString(i, parser.GetConfigString(csIndexFirstPlayer + i).GetPtr());
	}
	if(_mod == udtMod::CPMA)
	{
		ProcessCPMAScores(CS_CPMA_GAME_INFO);
	}
	else if(_protocol <= udtProtocol::Dm68)
	{
		ParseConfigStringInt(_score1, parser, CS_SCORE_1);
		ParseConfigStringInt(_score2, parser, CS_SCORE_2);
	}
	else
	{
		ParseConfigStringInt(_score1, parser, CS_SCORE_1);
		ParseConfigStringInt(_score2, parser, CS_SCORE_2);
		CloneConfigString(_name1, _tempAllocator, parser, CS_QL_SCORE_NAME_1);
		CloneConfigString(_name2, _tempAllocator, parser, CS_QL_SCORE_NAME_2);
	}
	AddScore();
	_scoreChanged = false;
}

void udtParserPlugInScores::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	if(_firstSnapshotTimeMs == UDT_S32_MIN)
	{
		_firstSnapshotTimeMs = parser._inServerTime;
	}

	idPlayerStateBase* const ps = GetPlayerState(arg.Snapshot, _protocol);
	if(ps == NULL)
	{
		return;
	}

	_followedNumber = ps->clientNum;
	_followedScore = ps->persistant[CS_PERS_IDX_SCORE];
}

void udtParserPlugInScores::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	if(!arg.IsConfigString)
	{
		const idTokenizer& tokenizer = parser.GetTokenizer();
		if(_mod == udtMod::CPMA &&
		   tokenizer.GetArgCount() >= 3 &&
		   udtString::EqualsNoCase(tokenizer.GetArg(0), "dmscores"))
		{
			StringParseInt(_clientNumber1, tokenizer.GetArgString(1)); // First place client number.
			StringParseInt(_clientNumber2, tokenizer.GetArgString(2)); // Second place client number.
		}

		return;
	}

	const s32 csIndexFirstPlayer = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, _protocol, _mod);
	if(arg.ConfigStringIndex >= csIndexFirstPlayer && arg.ConfigStringIndex < csIndexFirstPlayer + 64)
	{
		ProcessPlayerConfigString(arg.ConfigStringIndex - csIndexFirstPlayer, parser.GetConfigString(arg.ConfigStringIndex).GetPtr());
	}

	if(_mod == udtMod::CPMA)
	{
		if(arg.ConfigStringIndex == CS_CPMA_GAME_INFO ||
		   arg.ConfigStringIndex == CS_CPMA_ROUND_INFO)
		{
			const s32 prevScore1 = _score1;
			const s32 prevScore2 = _score2;
			ProcessCPMAScores(arg.ConfigStringIndex);
			if(_score1 != prevScore1 ||
			   _score2 != prevScore2)
			{
				_scoreChanged = true;
			}
		}
	}
	else
	{
		if(arg.ConfigStringIndex == CS_SCORE_1)
		{
			ParseConfigStringInt(_score1, parser, CS_SCORE_1);
			_scoreChanged = true;
		}
		else if(arg.ConfigStringIndex == CS_SCORE_2)
		{
			ParseConfigStringInt(_score2, parser, CS_SCORE_2);
			_scoreChanged = true;
		}

		if(_protocol >= udtProtocol::Dm73)
		{
			if(arg.ConfigStringIndex == CS_QL_SCORE_NAME_1)
			{
				CloneConfigString(_name1, _tempAllocator, parser, CS_QL_SCORE_NAME_1);
				_scoreChanged = true;
			}
			else if(arg.ConfigStringIndex == CS_QL_SCORE_NAME_2)
			{
				CloneConfigString(_name2, _tempAllocator, parser, CS_QL_SCORE_NAME_2);
				_scoreChanged = true;
			}
		}
	}
}

void udtParserPlugInScores::ProcessMessageBundleEnd(const udtMessageBundleCallbackArg&, udtBaseParser&)
{
	if(_scoreChanged)
	{
		AddScore();
		_scoreChanged = false;
	}
}

void udtParserPlugInScores::ProcessPlayerConfigString(u32 index, const char* cs)
{
	if(udtString::IsNullOrEmpty(cs))
	{
		_players[index].Present = 0;
		return;
	}

	_players[index].Present = 1;

	udtVMScopedStackAllocator allocatorScope(*TempAllocator);

	udtString name;
	if(ParseConfigStringValueString(name, *TempAllocator, "n", cs))
	{
		_players[index].Name = udtString::NewCloneFromRef(_stringAllocator, name);
	}

	if(HasClanName(_protocol))
	{
		udtString clan;
		if(ParseConfigStringValueString(clan, *TempAllocator, "cn", cs) &&
		   !udtString::IsNullOrEmpty(clan))
		{
			_players[index].Clan = udtString::NewCloneFromRef(_stringAllocator, clan);
		}
		else
		{
			_players[index].Clan = udtString::NewNull();
		}
	}

	s32 idTeam;
	u32 udtTeam;
	if(ParseConfigStringValueInt(idTeam, *TempAllocator, "t", cs) &&
	   GetUDTNumber(udtTeam, udtMagicNumberType::Team, idTeam, _protocol, _mod))
	{
		_players[index].Team = (u8)udtTeam;
	}
}

void udtParserPlugInScores::ProcessCPMAScores(s32 csIndex)
{
	const char* const cs = _parser->GetConfigString(csIndex).GetPtr();
	if(cs == NULL)
	{
		return;
	}

	udtVMScopedStackAllocator allocatorScope(*TempAllocator);

	ParseConfigStringValueInt(_score1, *TempAllocator, "sb", cs);
	ParseConfigStringValueInt(_score2, *TempAllocator, "sr", cs);
}

void udtParserPlugInScores::DetectGameType()
{
	const char* const cs = _parser->GetConfigString(CS_SERVERINFO).GetPtr();
	if(cs == NULL)
	{
		return;
	}

	udtVMScopedStackAllocator scopedTempAllocator(*TempAllocator);

	s32 idGT = 0;
	u32 udtGT = 0;
	if(ParseConfigStringValueInt(idGT, *TempAllocator, "g_gametype", cs) &&
	   GetUDTNumber(udtGT, udtMagicNumberType::GameType, idGT, _protocol, _mod))
	{
		_gameType = (udtGameType::Id)udtGT;
	}
}

void udtParserPlugInScores::DetectMod()
{
	const char* const cs = _parser->GetConfigString(CS_SERVERINFO).GetPtr();
	if(cs == NULL)
	{
		return;
	}

	udtVMScopedStackAllocator scopedTempAllocator(*TempAllocator);

	udtString varValue;
	if(ParseConfigStringValueString(varValue, *TempAllocator, "gamename", cs) &&
	   udtString::Equals(varValue, "cpma"))
	{
		_mod = udtMod::CPMA;
	}
	else if(ParseConfigStringValueString(varValue, *TempAllocator, "gamename", cs) &&
			udtString::ContainsNoCase(varValue, "osp"))
	{
		_mod = udtMod::OSP;
	}
	else if(ParseConfigStringValueString(varValue, *TempAllocator, "gameversion", cs) &&
			udtString::ContainsNoCase(varValue, "osp"))
	{
		_mod = udtMod::OSP;
	}
}

void udtParserPlugInScores::AddScore()
{
	udtParseDataScore scores;
	scores.Score1 = _score1;
	scores.Score2 = _score2;
	scores.Id1 = 64;
	scores.Id2 = 64;
	scores.GameStateIndex = _gameStateIndex;
	scores.ServerTimeMs = _parser->_inServerTime;
	scores.Flags = 0;
	WriteNullStringToApiStruct(scores.Name1);
	WriteNullStringToApiStruct(scores.Name2);
	WriteNullStringToApiStruct(scores.CleanName1);
	WriteNullStringToApiStruct(scores.CleanName2);
	if(IsTeamMode(_gameType))
	{
		scores.Flags |= (u32)udtParseDataScoreMask::TeamBased;
		scores.Id1 = 0;
		scores.Id2 = 1;
	}
	else
	{
		if(_mod == udtMod::CPMA)
		{
			GetScoresCPMA(scores);
		}
		else if(_protocol <= udtProtocol::Dm68)
		{
			GetScoresQ3(scores);
		}
		else
		{
			GetScoresQL(scores);
		}
	}
	// @NOTE: We use udtString::NewCleanCloneFromRef because it checks for the case where
	// the allocator to use for the clone is the same as the one used for the input string.
	// Before, we incorrectly used a pointer to a string inside a buffer that could get relocated.
	if(scores.Name1 != UDT_U32_MAX)
	{
		const udtString name1 = udtString::NewFromAllocAndOffset(_stringAllocator, scores.Name1, scores.Name1Length);
		WriteStringToApiStruct(scores.CleanName1, udtString::NewCleanCloneFromRef(_stringAllocator, _protocol, name1));
	}
	if(scores.Name2 != UDT_U32_MAX)
	{
		const udtString name2 = udtString::NewFromAllocAndOffset(_stringAllocator, scores.Name2, scores.Name2Length);
		WriteStringToApiStruct(scores.CleanName2, udtString::NewCleanCloneFromRef(_stringAllocator, _protocol, name2));
	}
	_scores.Add(scores);
}

void udtParserPlugInScores::GetScoresCPMA(udtParseDataScore& scores)
{
	const s32 score1 = _score1;
	const s32 score2 = _score2;
	u32 client1 = (u32)_clientNumber1;
	u32 client2 = (u32)_clientNumber2;
	scores.Score1 = score1;
	scores.Score2 = score2;

	if(client1 >= 64 &&
	   client2 >= 64)
	{
		const s32 followedNumber = _followedNumber;
		if(score1 == score2)
		{
			u32 found = 0;
			for(u32 i = 0; i < 64 && found != 3; ++i)
			{
				if(!_players[i].Present ||
				   _players[i].Team != (u8)udtTeam::Free)
				{
					continue;
				}

				if(client1 >= 64)
				{
					client1 = i;
					found |= 1;
				}
				else if(client2 >= 64)
				{
					client2 = i;
					found |= 2;
				}
			}
		}
		else if(followedNumber >= 0 &&
				followedNumber < 64 &&
				_players[followedNumber].Team == (u8)udtTeam::Free)
		{
			bool found = false;
			u32* clientId = NULL;
			if(_followedScore == score1)
			{
				found = true;
				clientId = &client2;
				client1 = followedNumber;
			}
			else if(_followedScore == score2)
			{
				found = true;
				clientId = &client1;
				client2 = followedNumber;
			}

			if(found)
			{
				for(u32 i = 0; i < 64; ++i)
				{
					if(i != (u32)followedNumber &&
					   _players[i].Present &&
					   _players[i].Team == (u8)udtTeam::Free)
					{
						*clientId = i;
						break;
					}
				}
			}
		}
	}

	scores.Id1 = client1;
	scores.Id2 = client2;
	if(client1 < 64)
	{
		WriteStringToApiStruct(scores.Name1, _players[client1].Name);
	}
	if(client2 < 64)
	{
		WriteStringToApiStruct(scores.Name2, _players[client2].Name);
	}
}

void udtParserPlugInScores::GetScoresQ3(udtParseDataScore& scores)
{
	const s32 followedNumber = _followedNumber;
	if(followedNumber < 0 ||
	   followedNumber >= 64 ||
	   _players[followedNumber].Team != (u8)udtTeam::Free)
	{
		return;
	}

	const Player& player = _players[followedNumber];
	u32* id;
	u32* otherId;
	u32* name;
	u32* otherName;
	if(_followedScore == scores.Score1)
	{
		id = &scores.Id1;
		otherId = &scores.Id2;
		name = &scores.Name1;
		otherName = &scores.Name2;
	}
	else if(_followedScore == scores.Score2)
	{
		id = &scores.Id2;
		otherId = &scores.Id1;
		name = &scores.Name2;
		otherName = &scores.Name1;
	}
	else
	{
		return;
	}

	*id = (u32)followedNumber;
	WriteStringToApiStruct(*name, player.Name);

	if(_gameType != udtGameType::Duel)
	{
		return;
	}

	for(u32 i = 0; i < 64; ++i)
	{
		const Player& otherPlayer = _players[i];
		if(i != (u32)followedNumber &&
		   otherPlayer.Present &&
		   otherPlayer.Team == (u8)udtTeam::Free)
		{
			*otherId = i;
			WriteStringToApiStruct(*otherName, otherPlayer.Name);
			break;
		}
	}
}

void udtParserPlugInScores::GetScoresQL(udtParseDataScore& scores)
{
	udtVMScopedStackAllocator allocScope(*TempAllocator);

	udtString name1 = udtString::NewCloneFromRef(*TempAllocator, _name1);
	udtString name2 = udtString::NewCloneFromRef(*TempAllocator, _name2);
	udtString::CleanUp(name1, _protocol);
	udtString::CleanUp(name2, _protocol);
	udtString::MakeLowerCase(name1);
	udtString::MakeLowerCase(name2);
	
	u32 found = 0;
	for(u32 i = 0; i < 64 && found != 3; ++i)
	{
		if(!_players[i].Present)
		{
			continue;
		}

		udtString name;
		GetScoreName(name, *TempAllocator, _players[i]);
		if(udtString::IsNullOrEmpty(name))
		{
			continue;
		}

		if(udtString::Equals(name1, name))
		{
			scores.Id1 = i;
			WriteStringToApiStruct(scores.Name1, _players[i].Name);
			found |= 1;
		}
		else if(udtString::Equals(name2, name))
		{
			scores.Id2 = i;
			WriteStringToApiStruct(scores.Name2, _players[i].Name);
			found |= 2;
		}
	}

	if(scores.Id1 >= 64)
	{
		WriteStringToApiStruct(scores.Name1, udtString::NewCloneFromRef(_stringAllocator, _name1));
	}

	if(scores.Id2 >= 64)
	{
		WriteStringToApiStruct(scores.Name2, udtString::NewCloneFromRef(_stringAllocator, _name2));
	}
}

void udtParserPlugInScores::GetScoreName(udtString& name, udtVMLinearAllocator& alloc, const udtParserPlugInScores::Player& player)
{
	if(udtString::IsNullOrEmpty(player.Clan))
	{
		udtString playerName = udtString::NewCloneFromRef(alloc, player.Name);
		udtString::CleanUp(playerName, _protocol);
		udtString::MakeLowerCase(playerName);
		name = playerName;
		return;
	}

	udtString playerName = udtString::NewCloneFromRef(alloc, player.Name);
	udtString clanName = udtString::NewCloneFromRef(alloc, player.Clan);
	udtString::CleanUp(playerName, _protocol);
	udtString::CleanUp(clanName, _protocol);
	udtString::MakeLowerCase(playerName);
	udtString::MakeLowerCase(clanName);

	const udtString space = udtString::NewConstRef(" ");
	const udtString* strings[] = { &clanName, &space, &playerName };
	name = udtString::NewFromConcatenatingMultiple(alloc, strings, (u32)UDT_COUNT_OF(strings));
}
