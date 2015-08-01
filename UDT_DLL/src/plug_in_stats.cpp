#include "plug_in_stats.hpp"
#include "utils.hpp"


static_assert((s32)udtTeamStatsField::Count <= (s32)(UDT_TEAM_STATS_MASK_BYTE_COUNT * 8), "Too many team stats fields for the bit mask size");
static_assert((s32)udtPlayerStatsField::Count <= (s32)(UDT_PLAYER_STATS_MASK_BYTE_COUNT * 8), "Too many player stats fields for the bit mask size");


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

	if(!_gameEnded)
	{
		return;
	}
	
	struct CommandHandler
	{
		typedef void (udtParserPlugInStats::*MemberFunction)();
		const char* Name;
		MemberFunction Function;
	};

#define HANDLER(Name, Function) { Name, &udtParserPlugInStats::Function }
	static const CommandHandler handlers[] =
	{
		HANDLER("scores_tdm", ParseQLScoresTDM),
		HANDLER("tdmstats", ParseQLStatsTDM),
		HANDLER("scores_duel", ParseQLScoresDuel),
		HANDLER("scores_ctf", ParseQLScoresCTF),
		HANDLER("ctfstats", ParseQLStatsCTF)
	};
#undef HANDLER
	/*
	@TODO:
	QL  : scores adscores scores_ad rrscores tdmscores tdmscores2 castats cascores dscores scores_ft scores_race scores_rr scores_ca
	CPMA: mstats dmscores mm2 xscores xstats2
	OSP : are there any? :p
	*/

	for(s32 i = 0; i < (s32)UDT_COUNT_OF(handlers); ++i)
	{
		if(udtString::Equals(commandName, handlers[i].Name))
		{
			(this->*(handlers[i].Function))();
			break;
		}
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

	static const udtStatsField teamScoreFields[] =
	{
		TEAM_FIELD(Score, 30)
	};

	udtParseDataStats* const stats = &_stats;
	ParseFields(stats->TeamStats[0].Flags, stats->TeamStats[0].Fields, teamFields, (s32)UDT_COUNT_OF(teamFields));
	ParseFields(stats->TeamStats[1].Flags, stats->TeamStats[1].Fields, teamFields, (s32)UDT_COUNT_OF(teamFields), 14);
	ParseFields(stats->TeamStats[0].Flags, stats->TeamStats[0].Fields, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields));
	ParseFields(stats->TeamStats[1].Flags, stats->TeamStats[1].Fields, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 1);

	const s32 playerScores = GetValue(29);
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
			ParseFields(playerStats.Flags, playerStats.Fields, playerFields, (s32)UDT_COUNT_OF(playerFields), offset);
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

	ParseFields(playerStats->Flags, playerStats->Fields, fields, (s32)UDT_COUNT_OF(fields));
}

void udtParserPlugInStats::ParseQLScoresDuel()
{
	// @TODO:
}

void udtParserPlugInStats::ParseQLScoresCTF()
{
	// @TODO:
}

void udtParserPlugInStats::ParseQLStatsCTF()
{
	// @TODO:
}

void udtParserPlugInStats::ParseFields(u8* destMask, s32* destFields, const udtStatsField* fields, s32 fieldCount, s32 tokenOffset)
{
	for(s32 i = 0; i < fieldCount; ++i)
	{
		const s32 fieldIndex = fields[i].Index;
		s32* const field = destFields + fieldIndex;
		*field = GetValue(fields[i].TokenIndex + tokenOffset);
		
		const s32 byteIndex = fieldIndex >> 3;
		const s32 bitIndex = fieldIndex & 7;
		destMask[byteIndex] |= (u8)1 << (u8)bitIndex;
	}
}

s32 udtParserPlugInStats::GetValue(s32 index)
{
	return (s32)atoi(_tokenizer->GetArgString((u32)index));
}

void udtParserPlugInStats::AddCurrentStats()
{
	bool valid = false;
	for(s32 i = 0; i < 2; ++i)
	{
		for(s32 j = 0; j < (s32)UDT_COUNT_OF(_stats.PlayerStats[i].Flags); ++j)
		{
			if(_stats.PlayerStats[i].Flags[j] != 0)
			{
				valid = true;
				break;
			}
		}
	}

	if(!valid)
	{
		for(s32 i = 0; i < 64; ++i)
		{
			for(s32 j = 0; j < (s32)UDT_COUNT_OF(_stats.TeamStats[i].Flags); ++j)
			{
				if(_stats.TeamStats[i].Flags[j] != 0)
				{
					valid = true;
					break;
				}
			}
		}
	}

	if(valid)
	{
		_statsArray.Add(_stats);
		memset(&_stats, 0, sizeof(_stats));
	}
}
