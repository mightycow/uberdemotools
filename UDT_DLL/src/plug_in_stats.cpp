#include "plug_in_stats.hpp"
#include "utils.hpp"


// @TODO: Move etc.
#define    CS_INTERMISSION_91    14


void udtParserPlugInStats::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& parser)
{
	_tokenizer = &parser._context->Tokenizer;
	_protocol = parser._inProtocol;

	const s32 firstPlayerCs = idConfigStringIndex::FirstPlayer(_protocol);
	for(s32 i = 0; i < 64; ++i)
	{
		const udtBaseParser::udtConfigString& cs = parser._inConfigStrings[firstPlayerCs + i];
		ProcessPlayerConfigString(cs.String, i);
	}
}

void udtParserPlugInStats::ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/)
{
	if(_tokenizer->GetArgCount() < 2)
	{
		return;
	}

	const udtString commandName = _tokenizer->GetArg(0);
	if(udtString::Equals(commandName, "cs"))
	{
		if(_tokenizer->GetArgCount() == 3)
		{
			s32 csIndex = -1;
			if(sscanf(_tokenizer->GetArgString(1), "%d", &csIndex) == 1)
			{
				ProcessConfigString(csIndex, _tokenizer->GetArg(2));
			}
		}
	}
	else if(udtString::Equals(commandName, "scores_tdm") && _gameEnded)
	{
		ParseQLScoresTDM();
	}
	else if(udtString::Equals(commandName, "tdmstats") && _gameEnded)
	{
		ParseQLStatsTDM();
	}
}

void udtParserPlugInStats::ProcessConfigString(s32 csIndex, const udtString& configString)
{
	if(_protocol == udtProtocol::Dm91 && 
	   csIndex == CS_INTERMISSION_91 && 
	   udtString::Equals(configString, "1"))
	{
		_gameEnded = true;
	}

	if(_protocol == udtProtocol::Dm91 &&
	   csIndex == CS_INTERMISSION_91 &&
	   udtString::Equals(configString, "0"))
	{
		if(_gameEnded)
		{
			AddCurrentStats();
		}

		_gameEnded = false;
	}

	const s32 firstPlayerCs = idConfigStringIndex::FirstPlayer(_protocol);
	if(csIndex >= firstPlayerCs && csIndex < firstPlayerCs + 64)
	{
		ProcessPlayerConfigString(configString.String, csIndex - firstPlayerCs);
	}
}

void udtParserPlugInStats::ProcessPlayerConfigString(const char* configString, s32 playerIndex)
{
	if(configString == NULL)
	{
		return;
	}

	udtString name;
	if(ParseConfigStringValueString(name, _namesAllocator, "n", configString))
	{
		udtString cleanName = udtString::NewClone(_namesAllocator, name.String, name.Length);
		udtString::CleanUp(cleanName, _protocol);
		_stats.PlayerStats[playerIndex].Name = name.String;
		_stats.PlayerStats[playerIndex].CleanName = cleanName.String;
	}
}

#define    PLAYER_FIELD(Name, Index)    { (s32)udtPlayerStatsField::Name, Index }
#define    TEAM_FIELD(Name, Index)      { (s32)udtTeamStatsField::Name, Index }

void udtParserPlugInStats::ParseQLScoresTDM()
{
	if(_tokenizer->GetArgCount() < 31)
	{
		return;
	}

	static const udtStatsField teamFields[] =
	{
		TEAM_FIELD(RedArmorPickups, 1),
		TEAM_FIELD(YellowArmorPickups, 2),
		TEAM_FIELD(GreenArmorPickups, 3),
		TEAM_FIELD(MegaHealthPickups, 4),
		TEAM_FIELD(QuadDamagePickups, 5),
		TEAM_FIELD(BattleSuitPickups, 6),
		TEAM_FIELD(RegenPickups, 7),
		TEAM_FIELD(HastePickups, 8),
		TEAM_FIELD(InvisPickups, 9),
		TEAM_FIELD(QuadTime, 10),
		TEAM_FIELD(BattleSuitTime, 11),
		TEAM_FIELD(RegenTime, 12),
		TEAM_FIELD(HasteTime, 13),
		TEAM_FIELD(InvisTime, 14),
	};

	static const udtStatsField playerFields[] =
	{
		PLAYER_FIELD(TeamIndex, 1),
		PLAYER_FIELD(Score, 2),
		PLAYER_FIELD(Ping, 3),
		PLAYER_FIELD(Time, 4),
		PLAYER_FIELD(Kills, 5),
		PLAYER_FIELD(Deaths, 6),
		PLAYER_FIELD(Accuracy, 7),
		PLAYER_FIELD(BestWeapon, 8),
		PLAYER_FIELD(Impressives, 9),
		PLAYER_FIELD(Excellents, 10),
		PLAYER_FIELD(Gauntlets, 11),
		PLAYER_FIELD(TeamKills, 12),
		PLAYER_FIELD(TeamKilled, 13),
		PLAYER_FIELD(DamageGiven, 14)
	};

	udtParseDataStats* const stats = &_stats;
	ParseFields(stats->TeamStats[0].Fields, teamFields, (s32)UDT_COUNT_OF(teamFields));
	ParseFields(stats->TeamStats[1].Fields, teamFields, (s32)UDT_COUNT_OF(teamFields), 14);

	const s64 teamFlags = CreateBitMask(teamFields, (s32)UDT_COUNT_OF(teamFields)) | ((s64)1 << (s64)udtTeamStatsField::Score);
	const s32 playerScores = GetValue(29);
	stats->TeamStats[0].Fields[udtTeamStatsField::Score] = GetValue(30);
	stats->TeamStats[1].Fields[udtTeamStatsField::Score] = GetValue(31);
	stats->TeamStats[0].Flags |= teamFlags;
	stats->TeamStats[1].Flags |= teamFlags;

	if(_tokenizer->GetArgCount() != (u32)(32 + playerScores * (s32)UDT_COUNT_OF(playerFields)))
	{
		return;
	}

	s32 offset = 32;
	for(s32 i = 0; i < playerScores; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			udtPlayerStats& playerStats = stats->PlayerStats[clientNumber];
			ParseFields(playerStats.Fields, playerFields, (s32)UDT_COUNT_OF(playerFields), offset);
			playerStats.Flags |= CreateBitMask(playerFields, (s32)UDT_COUNT_OF(playerFields));
		}

		offset += 1 + (s32)UDT_COUNT_OF(playerFields);
	}
}

void udtParserPlugInStats::ParseQLStatsTDM()
{
	if(_tokenizer->GetArgCount() != 13)
	{
		return;
	}

	static const udtStatsField fields[] =
	{
		PLAYER_FIELD(Suicides, 2),
		PLAYER_FIELD(TeamKills, 3),
		PLAYER_FIELD(TeamKilled, 4),
		PLAYER_FIELD(DamageGiven, 5),
		PLAYER_FIELD(DamageReceived, 6),
		PLAYER_FIELD(RedArmorPickups, 7),
		PLAYER_FIELD(YellowArmorPickups, 8),
		PLAYER_FIELD(GreenArmorPickups, 9),
		PLAYER_FIELD(MegaHealthPickups, 10),
		PLAYER_FIELD(QuadDamagePickups, 11),
		PLAYER_FIELD(BattleSuitPickups, 12)
	};

	const s32 clientNumber = GetValue(1);
	if(clientNumber < 0 || clientNumber >= 64)
	{
		return;
	}

	udtParseDataStats* const stats = &_stats;
	udtPlayerStats* const playerStats = &stats->PlayerStats[clientNumber];

	ParseFields(playerStats->Fields, fields, (s32)UDT_COUNT_OF(fields));
	playerStats->Flags |= CreateBitMask(fields, (s32)UDT_COUNT_OF(fields));
}

void udtParserPlugInStats::ParseFields(s32* dest, const udtStatsField* fields, s32 fieldCount, s32 tokenOffset)
{
	for(s32 i = 0; i < fieldCount; ++i)
	{
		s32* const field = dest + fields[i].Index;
		*field = GetValue(fields[i].TokenIndex + tokenOffset);
	}
}

s32 udtParserPlugInStats::GetValue(s32 index)
{
	return (s32)atoi(_tokenizer->GetArgString((u32)index));
}

s64 udtParserPlugInStats::CreateBitMask(const udtStatsField* fields, s32 fieldCount)
{
	s64 bitMask = 0;
	for(s32 i = 0; i < fieldCount; ++i)
	{
		bitMask |= (s64)1 << (s64)fields[i].Index;
	}

	return bitMask;
}

void udtParserPlugInStats::AddCurrentStats()
{
	bool valid = _stats.TeamStats[0].Flags != 0 || _stats.TeamStats[1].Flags != 0;
	if(!valid)
	{
		for(s32 i = 0; i < 64; ++i)
		{
			if(_stats.PlayerStats[i].Flags != 0)
			{
				valid = true;
				break;
			}
		}
	}

	if(valid)
	{
		_statsArray.Add(_stats);
		memset(&_stats, 0, sizeof(_stats));
	}
}
