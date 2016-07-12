#include "plug_in_scores.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


static void ParseConfigStringInt(s32& value, udtBaseParser& parser, s32 csIndex)
{
	const char* const cs = parser.GetConfigString(csIndex).GetPtr();
	if(cs == NULL)
	{
		return;
	}

	StringParseInt(value, cs);
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
}

void udtParserPlugInScores::FinishDemoAnalysis()
{
	const u32 firstIdx = BufferRanges.GetSize() > 0 ? BufferRanges[0].FirstIndex : 0;
	if(_scores[firstIdx].Id1 != _scores[firstIdx].Id2)
	{
		return;
	}

	u32 id1 = 0;
	u32 id2 = 0;
	u32 onePastlastIdxToFix = 0;
	for(u32 i = firstIdx + 1; i < _scores.GetSize(); ++i)
	{
		if(_scores[i].Id1 != _scores[i].Id2)
		{
			id1 = _scores[i].Id1;
			id2 = _scores[i].Id2;
			onePastlastIdxToFix = i;
			break;
		}
	}

	if(id1 == id2)
	{
		return;
	}

	for(u32 i = firstIdx + 1; i < onePastlastIdxToFix; ++i)
	{
		_scores[i].Id1 = id1;
		_scores[i].Id2 = id2;
	}
}

void udtParserPlugInScores::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& parser)
{
	const s32 csIndexScore1 = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::Scores1, parser._inProtocol, _mod);
	const s32 csIndexScore2 = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::Scores2, parser._inProtocol, _mod);
	const s32 csIndexFirstPlayer = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, parser._inProtocol, _mod);
	_score1 = 0;
	_score2 = 0;
	_clientNumber1 = 0;
	_clientNumber2 = 0;
	_csIndexScore1 = 685; // @TODO: handle other QL versions properly
	_csIndexScore2 = 686; // @TODO: handle other QL versions properly
	_csIndexClient1 = 687; // @TODO: handle other QL versions properly
	_csIndexClient2 = 688; // @TODO: handle other QL versions properly
	_parser = &parser;
	++_gameStateIndex;
	_protocol = parser._inProtocol;
	if(_protocol <= udtProtocol::Dm68)
	{
		DetectMod();
	}
	DetectGameType();
	if(_mod == udtMod::CPMA)
	{
		ProcessCPMAScores();
	}
	else if(_protocol <= udtProtocol::Dm68)
	{
		ParseConfigStringInt(_score1, parser, csIndexScore1);
		ParseConfigStringInt(_score2, parser, csIndexScore2);
	}
	else
	{
		ParseConfigStringInt(_score1, parser, _csIndexScore1);
		ParseConfigStringInt(_score2, parser, _csIndexScore2);
		ParseConfigStringInt(_clientNumber1, parser, _csIndexClient1);
		ParseConfigStringInt(_clientNumber2, parser, _csIndexClient2);
	}
	for(u32 i = 0; i < 64; ++i)
	{
		ProcessPlayerConfigString(i, parser.GetConfigString(csIndexFirstPlayer + i).GetPtr());
	}
	GetGameStateNumbers();
	AddScore();
	_scoreChanged = false;
}

void udtParserPlugInScores::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	if(arg.IsConfigString)
	{
		const s32 csIndexScore1 = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::Scores1, _protocol, _mod);
		const s32 csIndexScore2 = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::Scores2, _protocol, _mod);
		const s32 csIndexFirstPlayer = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, _protocol, _mod);

		if(arg.ConfigStringIndex >= csIndexFirstPlayer && arg.ConfigStringIndex < csIndexFirstPlayer + 64)
		{
			ProcessPlayerConfigString(arg.ConfigStringIndex - csIndexFirstPlayer, parser.GetConfigString(arg.ConfigStringIndex).GetPtr());
		}

		if(_mod == udtMod::CPMA)
		{
			if(arg.ConfigStringIndex == CS_CPMA_GAME_INFO)
			{
				const s32 prevScore1 = _score1;
				const s32 prevScore2 = _score2;
				ProcessCPMAScores();
				if(_score1 != prevScore1 ||
				   _score2 != prevScore2)
				{
					_scoreChanged = true;
				}
			}
			else if(arg.ConfigStringIndex == CS_CPMA_ROUND_INFO)
			{
				const s32 prevScore1 = _score1;
				const s32 prevScore2 = _score2;
				ProcessCPMARoundScores();
				if(_score1 != prevScore1 ||
				   _score2 != prevScore2)
				{
					_scoreChanged = true;
				}
			}
		}
		else if(_protocol <= udtProtocol::Dm68)
		{
			if(arg.ConfigStringIndex == csIndexScore1)
			{
				ParseConfigStringInt(_score1, parser, csIndexScore1);
				_scoreChanged = true;
			}
			else if(arg.ConfigStringIndex == csIndexScore2)
			{
				ParseConfigStringInt(_score2, parser, csIndexScore2);
				_scoreChanged = true;
			}
		}
		else
		{
			if(arg.ConfigStringIndex == _csIndexScore1)
			{
				ParseConfigStringInt(_score1, parser, _csIndexScore1);
				_scoreChanged = true;
			}
			else if(arg.ConfigStringIndex == _csIndexScore2)
			{
				ParseConfigStringInt(_score2, parser, _csIndexScore2);
				_scoreChanged = true;
			}
			else if(arg.ConfigStringIndex == _csIndexClient1)
			{
				ParseConfigStringInt(_clientNumber1, parser, _csIndexClient1);
			}
			else if(arg.ConfigStringIndex == _csIndexClient2)
			{
				ParseConfigStringInt(_clientNumber2, parser, _csIndexClient2);
			}
		}
	}
	else
	{
		const idTokenizer& tokenizer = parser.GetTokenizer();
		if(_mod == udtMod::CPMA && 
		   tokenizer.GetArgCount() >= 3 && 
		   udtString::EqualsNoCase(tokenizer.GetArg(0), "dmscores"))
		{
			StringParseInt(_clientNumber1, tokenizer.GetArgString(1)); // First place client number.
			StringParseInt(_clientNumber2, tokenizer.GetArgString(2)); // Second place client number.
		}
		else if(_mod != udtMod::CPMA &&
				tokenizer.GetArgCount() >= 4 &&
				udtString::EqualsNoCase(tokenizer.GetArg(0), "scores"))
		{
			ParseScoresCommand(tokenizer);
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

	s32 idTeam;
	u32 udtTeam;
	if(ParseConfigStringValueInt(idTeam, *TempAllocator, "t", cs) &&
	   GetUDTNumber(udtTeam, udtMagicNumberType::Team, idTeam, _protocol, _mod))
	{
		_players[index].Team = (u8)udtTeam;
	}
}

void udtParserPlugInScores::ProcessCPMAScores()
{
	const char* const cs = _parser->GetConfigString(CS_CPMA_GAME_INFO).GetPtr();
	if(cs == NULL)
	{
		return;
	}

	udtVMScopedStackAllocator allocatorScope(*TempAllocator);

	ParseConfigStringValueInt(_score1, *TempAllocator, "sr", cs);
	ParseConfigStringValueInt(_score2, *TempAllocator, "sb", cs);
}

void udtParserPlugInScores::ProcessCPMARoundScores()
{
	const char* const cs = _parser->GetConfigString(CS_CPMA_ROUND_INFO).GetPtr();
	if(cs == NULL)
	{
		return;
	}

	udtVMScopedStackAllocator allocatorScope(*TempAllocator);

	ParseConfigStringValueInt(_score1, *TempAllocator, "sr", cs);
	ParseConfigStringValueInt(_score2, *TempAllocator, "sb", cs);
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
	scores.GameStateIndex = _gameStateIndex;
	scores.ServerTimeMs = _parser->_inServerTime;
	scores.Flags = 0;
	WriteNullStringToApiStruct(scores.Name1);
	WriteNullStringToApiStruct(scores.Name2);
	if(IsTeamMode(_gameType))
	{
		scores.Flags |= (u32)udtParseDataScoreMask::TeamBased;
		scores.Id1 = 0;
		scores.Id2 = 1;
		scores.Score1 = _score1;
		scores.Score2 = _score2;
	}
	else
	{
		const u32 score1 = _score1;
		const u32 score2 = _score2;
		const u32 client1 = (u32)_clientNumber1;
		const u32 client2 = (u32)_clientNumber2;
		if(_mod == udtMod::CPMA)
		{
			scores.Id1 = client1;
			scores.Id2 = client2;
		}
		else
		{
			const bool score1Higher = score1 >= score2;
			scores.Id1 = score1Higher ? client1 : client2;
			scores.Id2 = score1Higher ? client2 : client1;
		}
		scores.Score1 = udt_max(score1, score2);
		scores.Score2 = udt_min(score1, score2);
		if(scores.Id1 < 64)
		{
			WriteStringToApiStruct(scores.Name1, _players[scores.Id1].Name);
		}
		if(scores.Id2 < 64)
		{
			WriteStringToApiStruct(scores.Name2, _players[scores.Id2].Name);
		}
	}
	_scores.Add(scores);
}

void udtParserPlugInScores::ParseScoresCommand(const idTokenizer& tokenizer)
{
	if(_protocol >= udtProtocol::Dm73)
	{
		ParseScoresCommandImpl(tokenizer, 18);
	}
	else if(_protocol == udtProtocol::Dm3)
	{
		ParseScoresCommandImpl(tokenizer, 6);
	}
	else
	{
		ParseScoresCommandImpl(tokenizer, 14);
	}
}

void udtParserPlugInScores::ParseScoresCommandImpl(const idTokenizer& tokenizer, u32 statsPerPlayer)
{
	s32 scoreCount = 0;
	if(!StringParseInt(scoreCount, tokenizer.GetArgString(1)) ||
	   scoreCount < 0 ||
	   tokenizer.GetArgCount() < 4 + (u32)scoreCount * statsPerPlayer)
	{
		return;
	}

	s32 score1 = UDT_S32_MIN;
	s32 score2 = UDT_S32_MIN;
	s32 id1 = -1;
	s32 id2 = -1;
	for(u32 i = 0; i < (u32)scoreCount; ++i)
	{
		s32 id;
		s32 score;
		if(!StringParseInt(id, tokenizer.GetArgString(i * statsPerPlayer + 4)) ||
		   !StringParseInt(score, tokenizer.GetArgString(i * statsPerPlayer + 5)))
		{
			continue;
		}

		if(score > score1)
		{
			score2 = score1;
			id2 = id1;
			score1 = score;
			id1 = id;
		}
		else if(score > score2)
		{
			score2 = score;
			id2 = id;
		}
	}

	_score1 = score1;
	_score2 = score2;
	_clientNumber1 = id1;
	_clientNumber2 = id2;
	_scoreChanged = true;
}

void udtParserPlugInScores::GetGameStateNumbers()
{
	if(IsTeamMode(_gameType) ||
	   _clientNumber1 != _clientNumber2)
	{
		return;
	}

	u32 id1 = 64;
	u32 id2 = 64;
	for(u32 i = 0; i < 64; ++i)
	{
		const Player& player = _players[i];
		if(player.Present == 0 ||
		   player.Team != udtTeam::Free)
		{
			continue;
		}

		if(id1 >= 64)
		{
			id1 = i;
		}
		else if(id2 >= 64)
		{
			id2 = i;
			break;
		}
	}

	_clientNumber1 = id1;
	_clientNumber2 = id2;
}
