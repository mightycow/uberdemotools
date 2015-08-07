#include "plug_in_stats.hpp"
#include "utils.hpp"


static_assert((s32)udtTeamStatsField::Count <= (s32)(UDT_TEAM_STATS_MASK_BYTE_COUNT * 8), "Too many team stats fields for the bit mask size");
static_assert((s32)udtPlayerStatsField::Count <= (s32)(UDT_PLAYER_STATS_MASK_BYTE_COUNT * 8), "Too many player stats fields for the bit mask size");


#define UDT_MAX_STATS 4


udtParserPlugInStats::udtParserPlugInStats()
{
	_tokenizer = NULL;
	_protocol = udtProtocol::Invalid;
	_gameEnded = false;
	ClearStats();
}

udtParserPlugInStats::~udtParserPlugInStats()
{
}

void udtParserPlugInStats::InitAllocators(u32 demoCount)
{
	FinalAllocator.Init((uptr)UDT_MAX_STATS * (uptr)sizeof(udtParseDataStats) * (uptr)demoCount);
	_namesAllocator.Init((uptr)(1 << 14) * (uptr)demoCount);
	_statsArray.SetAllocator(FinalAllocator);
	_analyzer.SetTempAllocator(*TempAllocator);
}

u32 udtParserPlugInStats::GetElementSize() const
{
	return (u32)sizeof(udtParseDataStats);
};

void udtParserPlugInStats::StartDemoAnalysis()
{
	_analyzer.ResetForNextDemo();

	_tokenizer = NULL;
	_protocol = udtProtocol::Invalid;
	_gameEnded = false;
	ClearStats();
}

void udtParserPlugInStats::FinishDemoAnalysis()
{
	_analyzer.FinishDemoAnalysis();
	if(!_analyzer.IsMatchInProgress() && _gameEnded)
	{
		AddCurrentStats();
	}
}

void udtParserPlugInStats::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessGamestateMessage(arg, parser);

	_tokenizer = &parser._context->Tokenizer;
	_protocol = parser._inProtocol;

	ClearStats();

	const s32 firstPlayerCs = idConfigStringIndex::FirstPlayer(_protocol);
	for(s32 i = 0; i < 64; ++i)
	{
		const udtBaseParser::udtConfigString& cs = parser._inConfigStrings[firstPlayerCs + i];
		ProcessPlayerConfigString(cs.String, i);
	}
}

void udtParserPlugInStats::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessCommandMessage(arg, parser);
	if(_analyzer.HasMatchJustStarted())
	{
		_gameEnded = false;
		AddCurrentStats();
		_analyzer.SetInProgress();
	}
	else if(_analyzer.HasMatchJustEnded())
	{
		_gameEnded = true;
	}

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

	if(_analyzer.IsMatchInProgress() || !_gameEnded)
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
		HANDLER("scores", ParseScores),
		HANDLER("dscores", ParseQLScoresDuelOld),
		HANDLER("xstats2", ParseCPMAXStats2),
		HANDLER("tdmscores", ParseQLScoresTDMVeryOld),
		HANDLER("tdmscores2", ParseQLScoresTDMOld)
	};
#undef HANDLER
	/*
	@TODO:
	QL  : adscores scores_ad rrscores castats cascores scores_ft scores_race scores_rr scores_ca
	CPMA: xscores
	OSP : statsinfo xstats1 bstats
	mstats:  full stats for one player sent multiple times during a game
	xstats2: full stats for one player sent at the end of a game, encoded the same as mstats
	xscores: team and player scores sent multiple times during a game
	*/

	for(s32 i = 0; i < (s32)UDT_COUNT_OF(handlers); ++i)
	{
		if(udtString::EqualsNoCase(commandName, handlers[i].Name))
		{
			(this->*(handlers[i].Function))();
			break;
		}
	}
}

void udtParserPlugInStats::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessSnapshotMessage(arg, parser);
}

void udtParserPlugInStats::ProcessConfigString(s32 csIndex, const udtString& configString)
{	
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

void udtParserPlugInStats::ParseScores()
{
	if(_protocol >= udtProtocol::Dm73)
	{
		ParseQLScoresOld();
	}
	else if(_protocol == udtProtocol::Dm3)
	{
		ParseQ3ScoresDM3();
	}
	else
	{
		ParseQ3Scores();
	}
}

void udtParserPlugInStats::ParseQLScoresTDM()
{
	_stats.GameType = (u32)udtGameType::TDM;

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
	_stats.GameType = (u32)udtGameType::TDM;

	// If more than 13 tokens, weapon stats follow.
	if(_tokenizer->GetArgCount() < 13)
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

	if(_tokenizer->GetArgCount() < (13 + 14*5))
	{
		return;
	}

#define WEAPON_FIELDS(Weapon, Offset) \
	PLAYER_FIELD(Weapon##Hits, Offset), \
	PLAYER_FIELD(Weapon##Shots, Offset + 1), \
	PLAYER_FIELD(Weapon##Accuracy, Offset + 2), \
	PLAYER_FIELD(Weapon##Damage, Offset + 3), \
	PLAYER_FIELD(Weapon##Kills, Offset + 4) \

	const udtStatsField weaponFields[] =
	{
		WEAPON_FIELDS(Gauntlet, 0),
		WEAPON_FIELDS(MachineGun, 5),
		WEAPON_FIELDS(Shotgun, 10),
		WEAPON_FIELDS(GrenadeLauncher, 15),
		WEAPON_FIELDS(RocketLauncher, 20),
		WEAPON_FIELDS(LightningGun, 25),
		WEAPON_FIELDS(Railgun, 30),
		WEAPON_FIELDS(PlasmaGun, 35),
		WEAPON_FIELDS(BFG, 40),
		WEAPON_FIELDS(GrapplingHook, 45),
		WEAPON_FIELDS(NailGun, 50),
		WEAPON_FIELDS(ProximityMineLauncher, 55),
		WEAPON_FIELDS(ChainGun, 60),
		WEAPON_FIELDS(HeavyMachineGun, 65)
	};

#undef WEAPON_FIELDS

	ParseFields(playerStats->Flags, playerStats->Fields, weaponFields, (s32)UDT_COUNT_OF(weaponFields), 13);
}

void udtParserPlugInStats::ParseQLScoresDuel()
{
	_stats.GameType = (u32)udtGameType::Duel;

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
	_stats.GameType = (u32)udtGameType::CTF;

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
	_stats.GameType = (u32)udtGameType::CTF;

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
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			udtPlayerStats& playerStats = _stats.PlayerStats[clientNumber];
			ParseFields(playerStats.Flags, playerStats.Fields, playerFields, (s32)UDT_COUNT_OF(playerFields), offset + 1);
		}

		offset += 18;
	}
}

void udtParserPlugInStats::ParseQLScoresDuelOld()
{
	_stats.GameType = (u32)udtGameType::Duel;

	// HMG stats were added in dm_90 and then removed in dm_91...
	const u32 expectedTokenCount = (_protocol == udtProtocol::Dm90) ? (u32)177 : (u32)167;
	if(_tokenizer->GetArgCount() < expectedTokenCount)
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
		PLAYER_FIELD(Ping, 0),
		PLAYER_FIELD(Kills, 1),
		PLAYER_FIELD(Deaths, 2),
		PLAYER_FIELD(Accuracy, 3),
		PLAYER_FIELD(DamageGiven, 4),
		PLAYER_FIELD(Impressives, 5),
		PLAYER_FIELD(Excellents, 6),
		PLAYER_FIELD(Gauntlets, 7),
		// We skip 2 fields.
		PLAYER_FIELD(RedArmorPickups, 10),
		PLAYER_FIELD(RedArmorPickupTime, 11),
		PLAYER_FIELD(YellowArmorPickups, 12),
		PLAYER_FIELD(YellowArmorPickupTime, 13),
		PLAYER_FIELD(GreenArmorPickups, 14),
		PLAYER_FIELD(GreenArmorPickupTime, 15),
		PLAYER_FIELD(MegaHealthPickups, 16),
		PLAYER_FIELD(MegaHealthPickupTime, 17),
		WEAPON_FIELDS(Gauntlet, 18),
		WEAPON_FIELDS(MachineGun, 23),
		WEAPON_FIELDS(Shotgun, 28),
		WEAPON_FIELDS(GrenadeLauncher, 33),
		WEAPON_FIELDS(RocketLauncher, 38),
		WEAPON_FIELDS(LightningGun, 43),
		WEAPON_FIELDS(Railgun, 48),
		WEAPON_FIELDS(PlasmaGun, 53),
		WEAPON_FIELDS(BFG, 58),
		WEAPON_FIELDS(GrapplingHook, 63),
		WEAPON_FIELDS(NailGun, 68),
		WEAPON_FIELDS(ProximityMineLauncher, 73),
		WEAPON_FIELDS(ChainGun, 78),
		WEAPON_FIELDS(HeavyMachineGun, 83)
	};

#undef WEAPON_FIELDS

	// HMG stats were added in dm_90 and then removed in dm_91...
	const s32 realFieldCount = (_protocol == udtProtocol::Dm90) ? (u32)88 : (u32)83;
	const s32 parseFieldCount = realFieldCount - 2;
	
	s32 offset = 1;
	for(s32 i = 0; i < 2; ++i)
	{
		const s32 clientNumber = (s32)_playerIndices[i];
		if(clientNumber >= 0 && clientNumber < 64)
		{
			udtPlayerStats& playerStats = _stats.PlayerStats[clientNumber];
			ParseFields(playerStats.Flags, playerStats.Fields, fields, parseFieldCount, offset);
		}

		offset += realFieldCount;
	}
}

void udtParserPlugInStats::ParseQ3Scores()
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

	if((s32)_tokenizer->GetArgCount() != 4 + (scoreCount * 14))
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
		PLAYER_FIELD(Captures, 12)
	};

	s32 offset = 4;
	for(s32 i = 0; i < scoreCount; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			udtPlayerStats& playerStats = _stats.PlayerStats[clientNumber];
			ParseFields(playerStats.Flags, playerStats.Fields, playerFields, (s32)UDT_COUNT_OF(playerFields), offset + 1);
		}

		offset += 14;
	}
}

void udtParserPlugInStats::ParseQ3ScoresDM3()
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

	if((s32)_tokenizer->GetArgCount() != 4 + (scoreCount * 6))
	{
		return;
	}

	static const udtStatsField playerFields[] =
	{
		PLAYER_FIELD(Score, 0),
		PLAYER_FIELD(Ping, 1),
		PLAYER_FIELD(Time, 2)
		// There's 2 more fields but I'm not sure what they are.
	};

	s32 offset = 4;
	for(s32 i = 0; i < scoreCount; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			udtPlayerStats& playerStats = _stats.PlayerStats[clientNumber];
			ParseFields(playerStats.Flags, playerStats.Fields, playerFields, (s32)UDT_COUNT_OF(playerFields), offset + 1);
		}

		offset += 6;
	}
}

void udtParserPlugInStats::ParseCPMAXStats2()
{
	if(_tokenizer->GetArgCount() < 3)
	{
		return;
	}

	const s32 clientNumber = GetValue(1);
	if(clientNumber < 0 || clientNumber >= 64)
	{
		return;
	}

	const s32 weaponMask = GetValue(2);

	s32 weaponCount = 0;
	for(s32 i = 0; i < 9; ++i)
	{
		if((weaponMask & (1 << (i + 1))) != 0)
		{
			++weaponCount;
		}
	}

	if(_tokenizer->GetArgCount() < (u32)(3 + 15 + weaponCount * 6))
	{
		return;
	}

#define WEAPON_FIELDS(Weapon) \
	PLAYER_FIELD(Weapon##Hits, 0), \
	PLAYER_FIELD(Weapon##Shots, 1), \
	PLAYER_FIELD(Weapon##Kills, 2), \
	PLAYER_FIELD(Weapon##Deaths, 3), \
	PLAYER_FIELD(Weapon##Pickups, 4), \
	PLAYER_FIELD(Weapon##Drops, 5)

	static const udtStatsField weaponFields[9][6] =
	{
		{ WEAPON_FIELDS(Gauntlet) },
		{ WEAPON_FIELDS(MachineGun) },
		{ WEAPON_FIELDS(Shotgun) },
		{ WEAPON_FIELDS(GrenadeLauncher) },
		{ WEAPON_FIELDS(RocketLauncher) },
		{ WEAPON_FIELDS(LightningGun) },
		{ WEAPON_FIELDS(Railgun) },
		{ WEAPON_FIELDS(PlasmaGun) },
		{ WEAPON_FIELDS(BFG) }
	};

#undef WEAPON_FIELDS

	static const udtStatsField playerFields[] =
	{
		PLAYER_FIELD(DamageGiven, 0),
		PLAYER_FIELD(DamageReceived, 1),
		PLAYER_FIELD(ArmorTaken, 2),
		PLAYER_FIELD(HealthTaken, 3),
		PLAYER_FIELD(MegaHealthPickups, 4),
		PLAYER_FIELD(RedArmorPickups, 5),
		PLAYER_FIELD(YellowArmorPickups, 6),
		PLAYER_FIELD(GreenArmorPickups, 7),
		// We skip some stuff...
		PLAYER_FIELD(Score, 11),
		PLAYER_FIELD(Kills, 12),
		PLAYER_FIELD(Deaths, 13),
		PLAYER_FIELD(Suicides, 14)
		// We ignore more stuff that follows.
	};

	udtPlayerStats& playerStats = _stats.PlayerStats[clientNumber];

	s32 offset = 3;
	for(s32 i = 0; i < 9; ++i)
	{
		if((weaponMask & (1 << (i + 1))) != 0)
		{
			ParseFields(playerStats.Flags, playerStats.Fields, weaponFields[i], (s32)UDT_COUNT_OF(weaponFields[i]), offset);
			offset += 6;
		}
	}

	ParseFields(playerStats.Flags, playerStats.Fields, playerFields, (s32)UDT_COUNT_OF(playerFields), offset);
}

void udtParserPlugInStats::ParseQLScoresTDMVeryOld()
{
	_stats.GameType = (u32)udtGameType::TDM;

	if(_tokenizer->GetArgCount() < 17)
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
		TEAM_FIELD(QuadDamageTime, 7),
		TEAM_FIELD(BattleSuitTime, 8)
	};

	static const udtStatsField playerFields[] =
	{
		PLAYER_FIELD(TeamIndex, 1),
		PLAYER_FIELD(TeamKills, 2),
		PLAYER_FIELD(TeamKilled, 3),
		PLAYER_FIELD(DamageGiven, 4)
	};

	udtParseDataStats* const stats = &_stats;
	ParseFields(stats->TeamStats[0].Flags, stats->TeamStats[0].Fields, teamFields, (s32)UDT_COUNT_OF(teamFields));
	ParseFields(stats->TeamStats[1].Flags, stats->TeamStats[1].Fields, teamFields, (s32)UDT_COUNT_OF(teamFields), 8);

	const s32 playerScores = ((s32)_tokenizer->GetArgCount() - 17) / 5;

	s32 offset = 17;
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

void udtParserPlugInStats::ParseQLScoresTDMOld()
{
	_stats.GameType = (u32)udtGameType::TDM;

	if(_tokenizer->GetArgCount() < 29)
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
		// We skip the subscriber field...
		PLAYER_FIELD(TeamKills, 3),
		PLAYER_FIELD(TeamKilled, 4),
		PLAYER_FIELD(DamageGiven, 5)
	};

	udtParseDataStats* const stats = &_stats;
	ParseFields(stats->TeamStats[0].Flags, stats->TeamStats[0].Fields, teamFields, (s32)UDT_COUNT_OF(teamFields));
	ParseFields(stats->TeamStats[1].Flags, stats->TeamStats[1].Fields, teamFields, (s32)UDT_COUNT_OF(teamFields), 14);

	const s32 playerScores = ((s32)_tokenizer->GetArgCount() - 29) / 6;

	s32 offset = 29;
	for(s32 i = 0; i < playerScores; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			udtPlayerStats& playerStats = stats->PlayerStats[clientNumber];
			ParseFields(playerStats.Flags, playerStats.Fields, playerFields, (s32)UDT_COUNT_OF(playerFields), offset);
		}

		offset += 6;
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

bool udtParserPlugInStats::AreStatsValid()
{
	// Any valid team stats?
	for(s32 i = 0; i < 2; ++i)
	{
		for(s32 j = 0; j < (s32)UDT_COUNT_OF(_stats.TeamStats[i].Flags); ++j)
		{
			if(_stats.TeamStats[i].Flags[j] != 0)
			{
				return true;
			}
		}
	}

	// Any valid player stats?
	for(s32 i = 0; i < 64; ++i)
	{
		for(s32 j = 0; j < (s32)UDT_COUNT_OF(_stats.PlayerStats[i].Flags); ++j)
		{
			if(_stats.PlayerStats[i].Flags[j] != 0)
			{
				return true;
			}
		}
	}

	return false;
}

void udtParserPlugInStats::AddCurrentStats()
{
	if(_statsArray.GetSize() == UDT_MAX_STATS ||
	   !AreStatsValid())
	{
		return;
	}

	//
	// Fix up the stats and save them.
	//

	_stats.MatchDurationMs = (u32)(_analyzer.MatchEndTime() - _analyzer.MatchStartTime());

	for(s32 i = 0; i < 64; ++i)
	{
		s32& bestWeapon = _stats.PlayerStats[i].Fields[udtPlayerStatsField::BestWeapon];
		bestWeapon = GetUDTWeaponFromIdWeapon(bestWeapon, _protocol);
		if(bestWeapon == -1)
		{
			bestWeapon = (s32)udtWeapon::Gauntlet;
		}
		ComputeAccuracies(_stats.PlayerStats[i]);
	}

	if(_stats.GameType == udtGameType::Invalid && 
	   _analyzer.GetGameType() != udtGameType::Invalid)
	{
		_stats.GameType = _analyzer.GetGameType();
	}

	_statsArray.Add(_stats);

	// Clear the stats for the next match.
	ClearStats();
}

void udtParserPlugInStats::ClearStats()
{
	memset(&_stats, 0, sizeof(_stats));

	_stats.GameType = (u32)udtGameType::Invalid;

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

void udtParserPlugInStats::ComputeAccuracies(udtPlayerStats& playerStats)
{
#define COMPUTE_ACC(Weapon) ComputeAccuracy(playerStats, (s32)udtPlayerStatsField::Weapon##Accuracy, (s32)udtPlayerStatsField::Weapon##Hits, (s32)udtPlayerStatsField::Weapon##Shots)
	COMPUTE_ACC(Gauntlet);
	COMPUTE_ACC(MachineGun);
	COMPUTE_ACC(Shotgun);
	COMPUTE_ACC(GrenadeLauncher);
	COMPUTE_ACC(RocketLauncher);
	COMPUTE_ACC(LightningGun);
	COMPUTE_ACC(Railgun);
	COMPUTE_ACC(PlasmaGun);
	COMPUTE_ACC(BFG);
	COMPUTE_ACC(GrapplingHook);
	COMPUTE_ACC(NailGun);
	COMPUTE_ACC(ProximityMineLauncher);
	COMPUTE_ACC(ChainGun);
	COMPUTE_ACC(HeavyMachineGun);
#undef COMPUTE_ACC
}

static bool IsBitSet(const u8* flags, s32 index)
{
	const s32 byteIndex = index >> 3;
	const s32 bitIndex = index & 7;

	return (flags[byteIndex] & ((u8)1 << (u8)bitIndex)) != 0;
}

static void SetBit(u8* flags, s32 index)
{
	const s32 byteIndex = index >> 3;
	const s32 bitIndex = index & 7;

	flags[byteIndex] |= (u8)1 << (u8)bitIndex;
}

void udtParserPlugInStats::ComputeAccuracy(udtPlayerStats& playerStats, s32 acc, s32 hits, s32 shots)
{
	if(IsBitSet(playerStats.Flags, acc) || 
	   !IsBitSet(playerStats.Flags, hits) ||
	   !IsBitSet(playerStats.Flags, shots))
	{
		return;
	}

	const s32 hitCount = playerStats.Fields[hits];
	const s32 shotCount = playerStats.Fields[shots];
	playerStats.Fields[acc] = shotCount == 0 ? 0 : ((100 * hitCount) / shotCount);
	SetBit(playerStats.Flags, acc);
}
