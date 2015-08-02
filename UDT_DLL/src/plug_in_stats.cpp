#include "plug_in_stats.hpp"
#include "utils.hpp"


static_assert((s32)udtTeamStatsField::Count <= (s32)(UDT_TEAM_STATS_MASK_BYTE_COUNT * 8), "Too many team stats fields for the bit mask size");
static_assert((s32)udtPlayerStatsField::Count <= (s32)(UDT_PLAYER_STATS_MASK_BYTE_COUNT * 8), "Too many player stats fields for the bit mask size");


void udtParserPlugInStats::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& parser)
{
	_tokenizer = &parser._context->Tokenizer;
	_protocol = parser._inProtocol;

	ClearStats();

	const char* intermissionCs = parser._inConfigStrings[idConfigStringIndex::Intermission(_protocol)].String;
	if(intermissionCs != NULL)
	{
		_gameEnded = strcmp(intermissionCs, "1") == 0;
	}

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
		HANDLER("ctfstats", ParseQLStatsCTF),
		HANDLER("scores", ParseQLScoresOld)
	};
#undef HANDLER
	/*
	@TODO:
	QL  : adscores scores_ad rrscores tdmscores tdmscores2 castats cascores dscores scores_ft scores_race scores_rr scores_ca
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
	if(csIndex == idConfigStringIndex::Intermission(_protocol) &&
	   udtString::Equals(configString, "1"))
	{
		_gameEnded = true;
	}

	if(csIndex == idConfigStringIndex::Intermission(_protocol) &&
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

	s32 teamIndex = -1;
	if(ParseConfigStringValueInt(teamIndex, _namesAllocator, "t", configString))
	{
		_stats.PlayerStats[playerIndex].TeamIndex = teamIndex;
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
	if(_tokenizer->GetArgCount() < 32)
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
		TEAM_FIELD(QuadDamageTime, 10),
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
	stats->GameType = (u32)udtGameType::TDM;
	ParseFields(stats->TeamStats[0].Flags, stats->TeamStats[0].Fields, teamFields, (s32)UDT_COUNT_OF(teamFields));
	ParseFields(stats->TeamStats[1].Flags, stats->TeamStats[1].Fields, teamFields, (s32)UDT_COUNT_OF(teamFields), 14);
	ParseFields(stats->TeamStats[0].Flags, stats->TeamStats[0].Fields, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields));
	ParseFields(stats->TeamStats[1].Flags, stats->TeamStats[1].Fields, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 1);

	const s32 playerScores = GetValue(29);
	if(_tokenizer->GetArgCount() != (u32)(32 + playerScores * (1 + (s32)UDT_COUNT_OF(playerFields))))
	{
		return;
	}

	s32 offset = 32;
	for(s32 i = 0; i < playerScores; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
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

	s32 clientNumber = -1;
	if(!GetClientNumberFromScoreIndex(clientNumber, 1))
	{
		return;
	}

	udtParseDataStats* const stats = &_stats;
	udtPlayerStats* const playerStats = &stats->PlayerStats[clientNumber];

	ParseFields(playerStats->Flags, playerStats->Fields, fields, (s32)UDT_COUNT_OF(fields));
}

void udtParserPlugInStats::ParseQLScoresDuel()
{
	if(_tokenizer->GetArgCount() < 2)
	{
		return;
	}

#define WEAPON_FIELDS(Weapon, Offset) \
	PLAYER_FIELD(Weapon##Hits, Offset), \
	PLAYER_FIELD(Weapon##Shots, Offset + 1), \
	PLAYER_FIELD(Weapon##Accuracy, Offset + 2), \
	PLAYER_FIELD(Weapon##Damage, Offset + 3), \
	PLAYER_FIELD(Weapon##Kills, Offset + 4) \

	static const udtStatsField fields[] =
	{
		PLAYER_FIELD(Score, 0),
		PLAYER_FIELD(Ping, 1),
		PLAYER_FIELD(Time, 2),
		PLAYER_FIELD(Kills, 3),
		PLAYER_FIELD(Deaths, 4),
		PLAYER_FIELD(Accuracy, 5),
		PLAYER_FIELD(BestWeapon, 6),
		PLAYER_FIELD(DamageGiven, 7),
		PLAYER_FIELD(Impressives, 8),
		PLAYER_FIELD(Excellents, 9),
		PLAYER_FIELD(Gauntlets, 10),
		PLAYER_FIELD(Perfect, 11),
		PLAYER_FIELD(RedArmorPickups, 12),
		PLAYER_FIELD(RedArmorPickupTime, 13),
		PLAYER_FIELD(YellowArmorPickups, 14),
		PLAYER_FIELD(YellowArmorPickupTime, 15),
		PLAYER_FIELD(GreenArmorPickups, 16),
		PLAYER_FIELD(GreenArmorPickupTime, 17),
		PLAYER_FIELD(MegaHealthPickups, 18),
		PLAYER_FIELD(MegaHealthPickupTime, 19),
		WEAPON_FIELDS(Gauntlet, 20),
		WEAPON_FIELDS(MachineGun, 25),
		WEAPON_FIELDS(Shotgun, 30),
		WEAPON_FIELDS(GrenadeLauncher, 35),
		WEAPON_FIELDS(RocketLauncher, 40),
		WEAPON_FIELDS(LightningGun, 45),
		WEAPON_FIELDS(Railgun, 50),
		WEAPON_FIELDS(PlasmaGun, 55),
		WEAPON_FIELDS(BFG, 60),
		WEAPON_FIELDS(GrapplingHook, 65),
		WEAPON_FIELDS(NailGun, 70),
		WEAPON_FIELDS(ProximityMineLauncher, 75),
		WEAPON_FIELDS(ChainGun, 80),
		WEAPON_FIELDS(HeavyMachineGun, 85)
	};

#undef WEAPON_FIELDS

	const s32 scoreCount = GetValue(1);
	if((s32)_tokenizer->GetArgCount() != 2 + (scoreCount * (1 + (s32)UDT_COUNT_OF(fields))))
	{
		return;
	}

	_stats.GameType = (u32)udtGameType::Duel;

	s32 offset = 2;
	for(s32 i = 0; i < scoreCount; ++i)
	{
		const s32 clientNumber = GetValue(offset++);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			udtPlayerStats& stats = _stats.PlayerStats[clientNumber];
			ParseFields(stats.Flags, stats.Fields, fields, (s32)UDT_COUNT_OF(fields), offset);
		}
		offset += (s32)UDT_COUNT_OF(fields);
	}
}

void udtParserPlugInStats::ParseQLScoresCTF()
{
	if(_tokenizer->GetArgCount() < 38)
	{
		return;
	}

	static const udtStatsField teamFields[] =
	{
		TEAM_FIELD(RedArmorPickups, 0),
		TEAM_FIELD(YellowArmorPickups, 1),
		TEAM_FIELD(GreenArmorPickups, 2),
		TEAM_FIELD(MegaHealthPickups, 3),
		TEAM_FIELD(QuadDamagePickups, 4),
		TEAM_FIELD(BattleSuitPickups, 5),
		TEAM_FIELD(RegenPickups, 6),
		TEAM_FIELD(HastePickups, 7),
		TEAM_FIELD(InvisPickups, 8),
		TEAM_FIELD(FlagPickups, 9),
		TEAM_FIELD(MedkitPickups, 10),
		TEAM_FIELD(QuadDamageTime, 11),
		TEAM_FIELD(BattleSuitTime, 12),
		TEAM_FIELD(RegenTime, 13),
		TEAM_FIELD(HasteTime, 14),
		TEAM_FIELD(InvisTime, 15),
		TEAM_FIELD(FlagTime, 16)
	};

	static const udtStatsField playerFields[] =
	{
		PLAYER_FIELD(TeamIndex, 0),
		PLAYER_FIELD(Score, 1),
		PLAYER_FIELD(Ping, 2),
		PLAYER_FIELD(Time, 3),
		PLAYER_FIELD(Kills, 4),
		PLAYER_FIELD(Deaths, 5),
		PLAYER_FIELD(Accuracy, 6),
		PLAYER_FIELD(BestWeapon, 7),
		PLAYER_FIELD(Impressives, 8),
		PLAYER_FIELD(Excellents, 9),
		PLAYER_FIELD(Gauntlets, 10),
		PLAYER_FIELD(Defends, 11),
		PLAYER_FIELD(Assists, 12),
		PLAYER_FIELD(Captures, 13),
		PLAYER_FIELD(Perfect, 14)
	};

	static const udtStatsField teamScoreFields[] =
	{
		TEAM_FIELD(Score, 0)
	};

	udtParseDataStats* const stats = &_stats;
	stats->GameType = (u32)udtGameType::CTF;
	ParseFields(stats->TeamStats[0].Flags, stats->TeamStats[0].Fields, teamFields, (s32)UDT_COUNT_OF(teamFields), 1);
	ParseFields(stats->TeamStats[1].Flags, stats->TeamStats[1].Fields, teamFields, (s32)UDT_COUNT_OF(teamFields), 18);
	ParseFields(stats->TeamStats[0].Flags, stats->TeamStats[0].Fields, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 36);
	ParseFields(stats->TeamStats[1].Flags, stats->TeamStats[1].Fields, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 37);

	const s32 playerScores = GetValue(35);
	if(_tokenizer->GetArgCount() != (u32)(38 + playerScores * (2 + (s32)UDT_COUNT_OF(playerFields))))
	{
		return;
	}

	s32 offset = 38;
	for(s32 i = 0; i < playerScores; ++i)
	{
		const s32 clientNumber = GetValue(offset++);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			udtPlayerStats& playerStats = stats->PlayerStats[clientNumber];
			ParseFields(playerStats.Flags, playerStats.Fields, playerFields, (s32)UDT_COUNT_OF(playerFields), offset);
		}

		offset += 1 + (s32)UDT_COUNT_OF(playerFields); // +1 because we skip the "Is Alive" field.
	}
}

void udtParserPlugInStats::ParseQLStatsCTF()
{
	if(_tokenizer->GetArgCount() != 14)
	{
		return;
	}

	static const udtStatsField fields[] =
	{
		PLAYER_FIELD(Suicides, 2),
		PLAYER_FIELD(DamageGiven, 3),
		PLAYER_FIELD(DamageReceived, 4),
		PLAYER_FIELD(RedArmorPickups, 5),
		PLAYER_FIELD(YellowArmorPickups, 6),
		PLAYER_FIELD(GreenArmorPickups, 7),
		PLAYER_FIELD(MegaHealthPickups, 8),
		PLAYER_FIELD(QuadDamagePickups, 9),
		PLAYER_FIELD(BattleSuitPickups, 10),
		PLAYER_FIELD(RegenPickups, 11),
		PLAYER_FIELD(HastePickups, 12),
		PLAYER_FIELD(InvisPickups, 13)
	};

	s32 clientNumber = -1;
	if(!GetClientNumberFromScoreIndex(clientNumber, 1))
	{
		return;
	}

	udtParseDataStats* const stats = &_stats;
	udtPlayerStats* const playerStats = &stats->PlayerStats[clientNumber];

	ParseFields(playerStats->Flags, playerStats->Fields, fields, (s32)UDT_COUNT_OF(fields));
}

void udtParserPlugInStats::ParseQLScoresOld()
{
	if(_tokenizer->GetArgCount() < 2)
	{
		return;
	}

	s32 scoreCount = GetValue(1);
	if(scoreCount < 0)
	{
		return;
	}

	scoreCount = udt_min(scoreCount, 64);

	static const udtStatsField teamFields[] =
	{
		TEAM_FIELD(Score, 0)
	};

	ParseFields(_stats.TeamStats[0].Flags, _stats.TeamStats[0].Fields, teamFields, (s32)UDT_COUNT_OF(teamFields), 2);
	ParseFields(_stats.TeamStats[1].Flags, _stats.TeamStats[1].Fields, teamFields, (s32)UDT_COUNT_OF(teamFields), 3);

	if((s32)_tokenizer->GetArgCount() != 4 + (scoreCount * 18))
	{
		return;
	}

	static const udtStatsField playerFields[] =
	{
		PLAYER_FIELD(Score, 0),
		PLAYER_FIELD(Ping, 1),
		PLAYER_FIELD(Time, 2),
		// We skip the flag scores and power-ups.
		PLAYER_FIELD(Accuracy, 5),
		PLAYER_FIELD(Impressives, 6),
		PLAYER_FIELD(Excellents, 7),
		PLAYER_FIELD(Gauntlets, 8),
		PLAYER_FIELD(Defends, 9),
		PLAYER_FIELD(Assists, 10),
		PLAYER_FIELD(Perfect, 11),
		PLAYER_FIELD(Captures, 12),
		// We skip alive.
		PLAYER_FIELD(Kills, 14),
		PLAYER_FIELD(Deaths, 15),
		PLAYER_FIELD(BestWeapon, 16)
	};

	s32 offset = 4;
	for(s32 i = 0; i < scoreCount; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber < 0 || clientNumber >= 64)
		{
			continue;
		}

		udtPlayerStats& playerStats = _stats.PlayerStats[clientNumber];
		ParseFields(playerStats.Flags, playerStats.Fields, playerFields, (s32)UDT_COUNT_OF(playerFields), offset + 1);

		offset += 18;
	}
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
		for(s32 j = 0; j < (s32)UDT_COUNT_OF(_stats.TeamStats[i].Flags); ++j)
		{
			if(_stats.TeamStats[i].Flags[j] != 0)
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
			for(s32 j = 0; j < (s32)UDT_COUNT_OF(_stats.PlayerStats[i].Flags); ++j)
			{
				if(_stats.PlayerStats[i].Flags[j] != 0)
				{
					valid = true;
					break;
				}
			}
		}
	}

	if(valid)
	{
		// Fix up the stats and save them.
		for(s32 i = 0; i < 64; ++i)
		{
			s32& bestWeapon = _stats.PlayerStats[i].Fields[udtPlayerStatsField::BestWeapon];
			bestWeapon = GetUDTWeaponFromIdWeapon(bestWeapon, _protocol);
			if(bestWeapon == -1)
			{
				bestWeapon = (s32)udtWeapon::Gauntlet;
			}
		}
		_statsArray.Add(_stats);

		// Clear the stats for the next match.
		ClearStats();
	}
}

void udtParserPlugInStats::ClearStats()
{
	memset(&_stats, 0, sizeof(_stats));

	_stats.GameType = u32(~0);

	for(s32 i = 0; i < 64; ++i)
	{
		_stats.PlayerStats[i].TeamIndex = -1;
	}
}

bool udtParserPlugInStats::GetClientNumberFromScoreIndex(s32& clientNumber, s32 fieldIndex)
{
	const s32 scoreIndex = GetValue(fieldIndex);
	if(scoreIndex < 0 || scoreIndex >= 64)
	{
		return false;
	}

	clientNumber = (s32)_playerIndices[scoreIndex];
	if(clientNumber < 0 || clientNumber >= 64)
	{
		return false;
	}

	return true;
}
