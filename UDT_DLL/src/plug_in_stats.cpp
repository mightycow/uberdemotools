#include "plug_in_stats.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


static_assert((s32)udtTeamStatsField::Count <= (s32)(UDT_TEAM_STATS_MASK_BYTE_COUNT * 8), "Too many team stats fields for the bit mask size");
static_assert((s32)udtPlayerStatsField::Count <= (s32)(UDT_PLAYER_STATS_MASK_BYTE_COUNT * 8), "Too many player stats fields for the bit mask size");


#define    UDT_MAX_STATS    4


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

static s32 CPMACharToInt(char c)
{
	if(c >= 'A'  &&  c <= 'Z')
	{
		return c - 39;
	}

	if(c >= 'a'  &&  c <= 'z')
	{
		return c - 97;
	}

	return 0;
}

static bool IsTeamMode(udtGameType::Id gameType)
{
	return gameType >= udtGameType::FirstTeamMode;
}

static u32 PopCount(u64 bitMask)
{
	u32 count = 0;
	for(u32 i = 0; i < 64; ++i)
	{
		if((bitMask & ((u64)1 << (u64)i)) != 0)
		{
			++count;
		}
	}

	return count;
}

static u32 PopCount(const u8* flags, u32 byteCount)
{
	const u32 bitCount = byteCount << 3;
	u32 count = 0;
	for(u32 i = 0; i < bitCount; ++i)
	{
		const u32 byteIndex = i >> 3;
		const u32 bitIndex = i & 7;
		if((flags[byteIndex] & ((u8)1 << (u8)bitIndex)) != 0)
		{
			++count;
		}
	}

	return count;
}


udtParserPlugInStats::udtParserPlugInStats()
{
	_tokenizer = NULL;
	_protocol = udtProtocol::Invalid;
	_followedClientNumber = -1;
	_maxAllowedStats = UDT_MAX_STATS;
	_gameEnded = false;
	ClearStats();
}

udtParserPlugInStats::~udtParserPlugInStats()
{
}

void udtParserPlugInStats::InitAllocators(u32 demoCount)
{
	FinalAllocator.Init((uptr)UDT_MAX_STATS * (uptr)sizeof(udtParseDataStats) * (uptr)demoCount);
	_allocator.Init((uptr)(1 << 20) * (uptr)demoCount);
	_statsArray.SetAllocator(FinalAllocator);
	_analyzer.InitAllocators(*TempAllocator, demoCount);
	_maxAllowedStats = demoCount * (u32)UDT_MAX_STATS;
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
	_followedClientNumber = -1;
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
	_followedClientNumber = -1;

	_firstPlaceClientNumber = -1;
	_secondPlaceClientNumber = -1;
	_cpmaScoreRed = S32_MIN;
	_cpmaScoreBlue = S32_MIN;
	_firstPlaceScore = S32_MIN;
	_secondPlaceScore = S32_MIN;

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
	// We can't add a match right after it ends because some scores and stats info will be sent after it ended.
	// So we add stats when a match starts (that isn't the first one) or the demo ended.
	_analyzer.ProcessCommandMessage(arg, parser);
	if(_analyzer.HasMatchJustStarted() && _gameEnded)
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
	s32 csIndex = -1;
	if(_tokenizer->GetArgCount() == 3 && 
	   udtString::Equals(commandName, "cs") &&
	   StringParseInt(csIndex, _tokenizer->GetArgString(1)))
	{
		ProcessConfigString(csIndex, _tokenizer->GetArg(2));
	}

	// For some demos we don't have more commands when the match is over.
	// We just decide that incomplete data is better than no data at all.
	// If that changes, we can uncomment the following line again.
	//if(_analyzer.IsMatchInProgress() || !_gameEnded) return;

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
		HANDLER("xscores", ParseCPMAXScores),
		HANDLER("dmscores", ParseCPMADMScores),
		HANDLER("tdmscores", ParseQLScoresTDMVeryOld),
		HANDLER("tdmscores2", ParseQLScoresTDMOld),
		HANDLER("statsinfo", ParseOSPStatsInfo)
	};
#undef HANDLER
	/*
	@TODO:
	QL  : adscores scores_ad rrscores castats cascores scores_ft scores_race scores_rr scores_ca
	OSP : xstats1 bstats
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
	idPlayerStateBase* const ps = GetPlayerState(arg.Snapshot, _protocol);
	if(ps != NULL)
	{ 
		_followedClientNumber = ps->clientNum;
	}

	_analyzer.ProcessSnapshotMessage(arg, parser);
}

void udtParserPlugInStats::ProcessConfigString(s32 csIndex, const udtString& configString)
{	
	const s32 firstPlayerCs = idConfigStringIndex::FirstPlayer(_protocol);
	if(csIndex >= firstPlayerCs && csIndex < firstPlayerCs + 64)
	{
		ProcessPlayerConfigString(configString.String, csIndex - firstPlayerCs);
	}
	else if(_analyzer.Mod() == udtMod::CPMA && csIndex == CS_CPMA_GAME_INFO)
	{
		udtVMScopedStackAllocator allocatorScope(*TempAllocator);
		ParseConfigStringValueInt(_cpmaScoreRed, *TempAllocator, "sr", configString.String);
		ParseConfigStringValueInt(_cpmaScoreBlue, *TempAllocator, "sb", configString.String);

		udtString redTeamName, blueTeamName;
		if(ParseConfigStringValueString(redTeamName, *TempAllocator, "nr", configString.String) &&
		   redTeamName.Length != 0)
		{
			_stats.CustomRedName = udtString::NewCleanCloneFromRef(_allocator, _protocol, redTeamName).String;
		}
		if(ParseConfigStringValueString(blueTeamName, *TempAllocator, "nb", configString.String) &&
		   blueTeamName.Length != 0)
		{
			_stats.CustomBlueName = udtString::NewCleanCloneFromRef(_allocator, _protocol, blueTeamName).String;
		}
	}
	else if(_analyzer.Mod() != udtMod::CPMA && csIndex == CS_SCORES1)
	{
		StringParseInt(_firstPlaceScore, configString.String);
	}
	else if(_analyzer.Mod() != udtMod::CPMA && csIndex == CS_SCORES2)
	{
		StringParseInt(_secondPlaceScore, configString.String);
	}
}

void udtParserPlugInStats::ProcessPlayerConfigString(const char* configString, s32 playerIndex)
{
	if(configString == NULL || *configString == '\0')
	{
		// Player disconnected, invalidate all fields.
		_playerTeamIndices[playerIndex] = -1;
		_playerStats[playerIndex].Name = NULL;
		_playerStats[playerIndex].CleanName = NULL;
		return;
	}

	udtVMScopedStackAllocator allocatorScope(*TempAllocator);

	udtString cpmaModel, cpmaName;
	if(_analyzer.Mod() == udtMod::CPMA &&
	   ParseConfigStringValueString(cpmaModel, *TempAllocator, "model", configString) &&
	   udtString::IsNullOrEmpty(cpmaModel) &&
	   ParseConfigStringValueString(cpmaName, *TempAllocator, "n", configString) &&
	   udtString::Equals(cpmaName, "UnnamedPlayer"))
	{
		// In a duel demo I have, a new player appears in team free when there's already 2 players there,
		// which is impossible. Not sure if this is CPMA specific.
		return;
	}

	s32 teamIndex = -1;
	if(ParseConfigStringValueInt(teamIndex, *TempAllocator, "t", configString))
	{
		_playerTeamIndices[playerIndex] = teamIndex;
	}

	udtString clan, name;
	bool hasClan;
	if(GetClanAndPlayerName(clan, name, hasClan, *TempAllocator, _protocol, configString))
	{
		_playerStats[playerIndex].Name = udtString::NewCloneFromRef(_allocator, name).String;
		_playerStats[playerIndex].CleanName = udtString::NewCleanCloneFromRef(_allocator, _protocol, name).String;
	}
}

#define    PLAYER_FIELD(Name, Index)    { (s32)udtPlayerStatsField::Name, Index }
#define    TEAM_FIELD(Name, Index)      { (s32)udtTeamStatsField::Name, Index }

#define    PLAYER_VALUE(Name, Value)    { (s32)udtPlayerStatsField::Name, Value }
#define    TEAM_VALUE(Name, Value)      { (s32)udtTeamStatsField::Name, Value }

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
	
	ParseTeamFields(0, teamFields, (s32)UDT_COUNT_OF(teamFields));
	ParseTeamFields(1, teamFields, (s32)UDT_COUNT_OF(teamFields), 14);
	ParseTeamFields(0, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields));
	ParseTeamFields(1, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 1);

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
			ParsePlayerFields(clientNumber, playerFields, (s32)UDT_COUNT_OF(playerFields), offset);
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

	ParsePlayerFields(clientNumber, fields, (s32)UDT_COUNT_OF(fields));

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

	ParsePlayerFields(clientNumber, weaponFields, (s32)UDT_COUNT_OF(weaponFields), 13);
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
			ParsePlayerFields(clientNumber, fields, (s32)UDT_COUNT_OF(fields), offset);
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

	ParseTeamFields(0, teamFields, (s32)UDT_COUNT_OF(teamFields), 1);
	ParseTeamFields(1, teamFields, (s32)UDT_COUNT_OF(teamFields), 18);
	ParseTeamFields(0, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 36);
	ParseTeamFields(1, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 37);

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
			ParsePlayerFields(clientNumber, playerFields, (s32)UDT_COUNT_OF(playerFields), offset);
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

	ParsePlayerFields(clientNumber, fields, (s32)UDT_COUNT_OF(fields));
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

	ParseTeamFields(0, teamFields, (s32)UDT_COUNT_OF(teamFields), 2);
	ParseTeamFields(1, teamFields, (s32)UDT_COUNT_OF(teamFields), 3);

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
			ParsePlayerFields(clientNumber, playerFields, (s32)UDT_COUNT_OF(playerFields), offset + 1);
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
			ParsePlayerFields(clientNumber, fields, parseFieldCount, offset);
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

	ParseTeamFields(0, teamFields, (s32)UDT_COUNT_OF(teamFields), 2);
	ParseTeamFields(1, teamFields, (s32)UDT_COUNT_OF(teamFields), 3);

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
			ParsePlayerFields(clientNumber, playerFields, (s32)UDT_COUNT_OF(playerFields), offset + 1);
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

	ParseTeamFields(0, teamFields, (s32)UDT_COUNT_OF(teamFields), 2);
	ParseTeamFields(1, teamFields, (s32)UDT_COUNT_OF(teamFields), 3);

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
			ParsePlayerFields(clientNumber, playerFields, (s32)UDT_COUNT_OF(playerFields), offset + 1);
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

	static const udtStatsField playerFields0[] =
	{
		PLAYER_FIELD(DamageGiven, 0),
		PLAYER_FIELD(DamageReceived, 1),
		PLAYER_FIELD(ArmorTaken, 2),
		PLAYER_FIELD(HealthTaken, 3),
		PLAYER_FIELD(MegaHealthPickups, 4),
		PLAYER_FIELD(RedArmorPickups, 5),
		PLAYER_FIELD(YellowArmorPickups, 6),
		PLAYER_FIELD(GreenArmorPickups, 7)
		// We ignore more stuff that follows.
	};

	static const udtStatsField playerFields1[] =
	{
		PLAYER_FIELD(Score, 0),
		PLAYER_FIELD(Kills, 1),
		PLAYER_FIELD(Deaths, 2),
		PLAYER_FIELD(Suicides, 3)
		// We ignore more stuff that follows.
	};

	s32 offset = 3;
	for(s32 i = 0; i < 9; ++i)
	{
		if((weaponMask & (1 << (i + 1))) != 0)
		{
			ParsePlayerFields(clientNumber, weaponFields[i], (s32)UDT_COUNT_OF(weaponFields[i]), offset);
			offset += 6;
		}
	}

	ParsePlayerFields(clientNumber, playerFields0, (s32)UDT_COUNT_OF(playerFields0), offset);
	offset += 8;

	for(u32 i = (u32)offset, count = _tokenizer->GetArgCount(); i < count; ++i)
	{
		const udtString token = _tokenizer->GetArg(i);
		if(token.Length >= 4 && (token.String[0] < '0' || token.String[0] > '9'))
		{
			offset = (s32)i + 1;
			break;
		}
	}

	if(_tokenizer->GetArgCount() < ((u32)offset + 4))
	{
		return;
	}

	ParsePlayerFields(clientNumber, playerFields1, (s32)UDT_COUNT_OF(playerFields1), offset);
}

void udtParserPlugInStats::ParseCPMAXScores()
{
	if(_tokenizer->GetArgCount() < 2)
	{
		return;
	}

	const s32 clientCount = GetValue(1);
	if(clientCount <= 0 || 
	   clientCount >= 64 ||
	   _tokenizer->GetArgCount() < (u32)(2 + clientCount * 7))
	{
		return;
	}

	static const udtStatsField playerFields[] =
	{
		PLAYER_FIELD(Score, 0),
		// We skip a string containing the ping and other stuff.
		PLAYER_FIELD(Time, 2)
		// We skip...
		// power-ups
		// net / captures
		// defends
	};

	s32 offset = 2;
	for(s32 i = 0; i < clientCount; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			ParsePlayerFields(clientNumber, playerFields, (s32)UDT_COUNT_OF(playerFields), offset + 1);

			const udtString textField = _tokenizer->GetArg((u32)offset + 2);
			if(textField.Length >= 4)
			{
				// It seems that 3-4 and 5-6 give the low and high values of the ping range.
				const s32 ping = 32 * CPMACharToInt(textField.String[2]) + CPMACharToInt(textField.String[3]);
				SetPlayerField(clientNumber, udtPlayerStatsField::Ping, ping);
			}
		}

		offset += 7;
	}
}

void udtParserPlugInStats::ParseCPMADMScores()
{
	if(_tokenizer->GetArgCount() < 3)
	{
		return;
	}

	_firstPlaceClientNumber = GetValue(1);
	_secondPlaceClientNumber = GetValue(2);
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

	ParseTeamFields(0, teamFields, (s32)UDT_COUNT_OF(teamFields));
	ParseTeamFields(1, teamFields, (s32)UDT_COUNT_OF(teamFields), 8);

	const s32 playerScores = ((s32)_tokenizer->GetArgCount() - 17) / 5;

	s32 offset = 17;
	for(s32 i = 0; i < playerScores; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			ParsePlayerFields(clientNumber, playerFields, (s32)UDT_COUNT_OF(playerFields), offset);
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

	ParseTeamFields(0, teamFields, (s32)UDT_COUNT_OF(teamFields));
	ParseTeamFields(1, teamFields, (s32)UDT_COUNT_OF(teamFields), 14);

	const s32 playerScores = ((s32)_tokenizer->GetArgCount() - 29) / 6;

	s32 offset = 29;
	for(s32 i = 0; i < playerScores; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			ParsePlayerFields(clientNumber, playerFields, (s32)UDT_COUNT_OF(playerFields), offset);
		}

		offset += 6;
	}
}

void udtParserPlugInStats::ParseOSPStatsInfo()
{
	// 22 fields including the weapon mask.
	if(_followedClientNumber < 0 || 
	   _followedClientNumber >= 64 ||
	   _tokenizer->GetArgCount() < 23)
	{
		return;
	}

	static const udtStatsField fields[] =
	{
		PLAYER_FIELD(Score, 2),
		PLAYER_FIELD(Suicides, 3),
		PLAYER_FIELD(Kills, 4),
		PLAYER_FIELD(Deaths, 5),
		PLAYER_FIELD(DamageGiven, 9),
		PLAYER_FIELD(DamageReceived, 10),
		PLAYER_FIELD(MegaHealthPickups, 18),
		PLAYER_FIELD(GreenArmorPickups, 19),
		PLAYER_FIELD(RedArmorPickups, 20),
		PLAYER_FIELD(YellowArmorPickups, 21)
	};

	ParsePlayerFields(_followedClientNumber, fields, (s32)UDT_COUNT_OF(fields));

	const s32 armorTaken = (GetValue(11) >> 16) & 0xFFFF;
	const s32 healthTaken = (GetValue(12) >> 16) & 0xFFFF;
	const udtStatsFieldValue newValues[2] = { PLAYER_VALUE(ArmorTaken, armorTaken), PLAYER_VALUE(HealthTaken, healthTaken) };
	SetPlayerFields(_followedClientNumber, newValues, (s32)UDT_COUNT_OF(newValues));

	const s32 weaponMask = GetValue(22);
	s32 weaponCount = 0; // Yes, this loop implements popcnt.
	for(s32 i = 0; i < 32; ++i)
	{
		if((weaponMask & (1 << i)) != 0)
		{
			++weaponCount;
		}
	}

	if(_tokenizer->GetArgCount() < (u32)(23 + 4 * weaponCount))
	{
		return;
	}

#define WEAPON_FIELDS(Weapon) \
	{ \
		(s32)udtPlayerStatsField::Weapon##Hits, \
		(s32)udtPlayerStatsField::Weapon##Drops, \
		(s32)udtPlayerStatsField::Weapon##Shots, \
		(s32)udtPlayerStatsField::Weapon##Pickups, \
		(s32)udtPlayerStatsField::Weapon##Kills, \
		(s32)udtPlayerStatsField::Weapon##Deaths  \
	}

	static const s32 weaponFieldIndices[11][6] =
	{
		WEAPON_FIELDS(Gauntlet), // Weapon 0 is nothing.
		WEAPON_FIELDS(Gauntlet),
		WEAPON_FIELDS(MachineGun),
		WEAPON_FIELDS(Shotgun),
		WEAPON_FIELDS(GrenadeLauncher),
		WEAPON_FIELDS(RocketLauncher),
		WEAPON_FIELDS(LightningGun),
		WEAPON_FIELDS(Railgun),
		WEAPON_FIELDS(PlasmaGun),
		WEAPON_FIELDS(BFG),
		WEAPON_FIELDS(GrapplingHook)
	};

#undef WEAPON_FIELDS

	s32 offset = 23;
	for(s32 i = 1; i < 11; ++i)
	{
		if((weaponMask & (1 << i)) == 0)
		{
			continue;
		}

		const s32* const indices = weaponFieldIndices[i];
		const s32 hitsAndDrops = GetValue(offset++);
		const s32 shotsAndPickups = GetValue(offset++);
		const s32 kills = GetValue(offset++);
		const s32 deaths = GetValue(offset++);
		const s32 hits = hitsAndDrops & 0xFFFF;
		const s32 drops = (hitsAndDrops >> 16) & 0xFFFF;
		const s32 shots = shotsAndPickups & 0xFFFF;
		const s32 pickups = (shotsAndPickups >> 16) & 0xFFFF;
		const udtStatsFieldValue newWeaponValues[] =
		{
			{ indices[0], hits },
			{ indices[1], drops },
			{ indices[2], shots },
			{ indices[3], pickups },
			{ indices[4], kills },
			{ indices[5], deaths }
		};

		SetPlayerFields(_followedClientNumber, newWeaponValues, (s32)UDT_COUNT_OF(newWeaponValues));
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

void udtParserPlugInStats::SetFields(u8* destMask, s32* destFields, const udtStatsFieldValue* fields, s32 fieldCount)
{
	for(s32 i = 0; i < fieldCount; ++i)
	{
		const s32 fieldIndex = fields[i].Index;
		s32* const field = destFields + fieldIndex;
		*field = fields[i].Value;

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
	// @NOTE: We can't use ValidTeams and ValidPlayers yet
	// because we haven't built the bit masks yet!

	// Any valid team stats?
	for(s32 i = 0; i < 2; ++i)
	{
		const u8* const flags = GetTeamFlags(i);
		for(s32 j = 0; j < UDT_TEAM_STATS_MASK_BYTE_COUNT; ++j)
		{
			if(flags[j] != 0)
			{
				return true;
			}
		}
	}

	// Any valid player stats?
	for(s32 i = 0; i < 64; ++i)
	{
		const u8* const flags = GetPlayerFlags(i);
		for(s32 j = 0; j < UDT_PLAYER_STATS_MASK_BYTE_COUNT; ++j)
		{
			if(flags[j] != 0)
			{
				return true;
			}
		}
	}

	return false;
}

void udtParserPlugInStats::AddCurrentStats()
{
	if(_statsArray.GetSize() >= _maxAllowedStats ||
	   !AreStatsValid())
	{
		return;
	}

	//
	// Fix up the stats and save them.
	//

	_stats.MatchDurationMs = (u32)(_analyzer.MatchEndTime() - _analyzer.MatchStartTime() - _analyzer.TotalTimeOutDuration());

	if(_stats.GameType == udtGameType::Invalid &&
	   _analyzer.GameType() != udtGameType::Invalid)
	{
		_stats.GameType = _analyzer.GameType();
	}

	for(s32 i = 0; i < 64; ++i)
	{
		// Fix the weapon index.
		s32& bestWeapon = GetPlayerFields(i)[udtPlayerStatsField::BestWeapon];
		bestWeapon = GetUDTWeaponFromIdWeapon(bestWeapon, _protocol);
		if(bestWeapon == -1)
		{
			bestWeapon = (s32)udtWeapon::Gauntlet;
		
		}
		
		// Fix the team index when needed and possible.
		if(!IsBitSet(GetPlayerFlags(i), (s32)udtPlayerStatsField::TeamIndex) &&
		   _playerTeamIndices[i] != -1)
		{
			SetPlayerField(i, udtPlayerStatsField::TeamIndex, _playerTeamIndices[i]);
		}
		
		ComputePlayerAccuracies(i);
	}

	// Make sure damage given and damage received are defined for both players in a duel.
	if((udtGameType::Id)_stats.GameType == udtGameType::Duel)
	{
		s32 playerIndices[2] = { -1, -1 };
		s32 playersFound = 0;
		for(s32 i = 0; i < 64; ++i)
		{
			if(IsBitSet(GetPlayerFlags(i), (s32)udtPlayerStatsField::TeamIndex) &&
			   GetPlayerFields(i)[udtPlayerStatsField::TeamIndex] == (s32)udtTeam::Free)
			{
				playerIndices[playersFound++] = i;
				if(playersFound == 2)
				{
					break;
				}
			}
		}

		if(playersFound == 2)
		{
			for(s32 i = 0; i < 2; ++i)
			{
				s32& damageGiven = GetPlayerFields(playerIndices[i])[udtPlayerStatsField::DamageGiven];
				if(damageGiven > 0)
				{
					SetPlayerField(playerIndices[i ^ 1], udtPlayerStatsField::DamageReceived, damageGiven);
				}

				s32& damageReceived = GetPlayerFields(playerIndices[i])[udtPlayerStatsField::DamageReceived];
				if(damageReceived > 0)
				{
					SetPlayerField(playerIndices[i ^ 1], udtPlayerStatsField::DamageGiven, damageReceived);
				}
			}
		}
	}

	for(u32 i = 0; i < 2; ++i)
	{
		u8* const flags = GetTeamFlags((s32)i);
		for(u32 j = 0; j < UDT_TEAM_STATS_MASK_BYTE_COUNT; ++j)
		{
			if(flags[j] != 0)
			{
				_stats.ValidTeams |= ((u64)1 << (u64)i);
				break;
			}
		}
	}

	for(u32 i = 0; i < 64; ++i)
	{
		u8* const flags = GetPlayerFlags((s32)i);
		for(u32 j = 0; j < UDT_PLAYER_STATS_MASK_BYTE_COUNT; ++j)
		{
			if(flags[j] != 0)
			{
				_stats.ValidPlayers |= ((u64)1 << (u64)i);
				break;
			}
		}
	}

	if(!IsTeamMode((udtGameType::Id)_stats.GameType))
	{
		_stats.ValidTeams = 0;
	}

	const u32 teamCount = PopCount(_stats.ValidTeams);
	const u32 playerCount = PopCount(_stats.ValidPlayers);
	u32 teamFieldCount = 0;
	u32 playerFieldCount = 0;
	for(s32 i = 0; i < 2; ++i)
	{
		teamFieldCount += PopCount(GetTeamFlags(i), UDT_TEAM_STATS_MASK_BYTE_COUNT);
	}
	for(s32 i = 0; i < 64; ++i)
	{
		playerFieldCount += PopCount(GetPlayerFlags(i), UDT_PLAYER_STATS_MASK_BYTE_COUNT);
	}

	u8* teamFlags = teamCount == 0 ? NULL : (u8*)_allocator.Allocate((uptr)teamCount * (uptr)UDT_TEAM_STATS_MASK_BYTE_COUNT);
	u8* playerFlags = playerCount == 0 ? NULL : (u8*)_allocator.Allocate((uptr)playerCount * (uptr)UDT_PLAYER_STATS_MASK_BYTE_COUNT);
	s32* teamFields = teamFieldCount == 0 ? NULL : (s32*)_allocator.Allocate((uptr)teamFieldCount * 4);
	s32* playerFields = playerFieldCount == 0 ? NULL : (s32*)_allocator.Allocate((uptr)playerFieldCount * 4);
	udtPlayerStats* playerStats = playerCount == 0 ? NULL : (udtPlayerStats*)_allocator.Allocate((uptr)playerCount * (uptr)sizeof(udtPlayerStats));

	_stats.TeamFlags = teamFlags;
	_stats.PlayerFlags = playerFlags;
	_stats.TeamFields = teamFields;
	_stats.PlayerFields = playerFields;
	_stats.PlayerStats = playerStats;

	for(s32 i = 0; i < 2; ++i)
	{
		if((_stats.ValidTeams & ((u64)1 << (u64)i)) == 0)
		{
			continue;
		}

		memcpy(teamFlags, GetTeamFlags(i), UDT_TEAM_STATS_MASK_BYTE_COUNT);

		s32 fieldsWritten = 0;
		for(s32 j = 0; j < (s32)udtTeamStatsField::Count; ++j)
		{
			if(IsBitSet(teamFlags, j))
			{
				*teamFields++ = GetTeamFields(i)[j];
				++fieldsWritten;
			}
		}

		teamFlags += UDT_TEAM_STATS_MASK_BYTE_COUNT;
	}
	
	for(u32 i = 0; i < 64; ++i)
	{
		if((_stats.ValidPlayers & ((u64)1 << (u64)i)) == 0)
		{
			continue;
		}

		memcpy(playerFlags, GetPlayerFlags(i), UDT_PLAYER_STATS_MASK_BYTE_COUNT);

		for(s32 j = 0; j < (s32)udtPlayerStatsField::Count; ++j)
		{
			if(IsBitSet(playerFlags, j))
			{
				*playerFields++ = GetPlayerFields(i)[j];
			}
		}

		playerFlags += UDT_PLAYER_STATS_MASK_BYTE_COUNT;
		*playerStats++ = _playerStats[i];
	}

	if(IsTeamMode((udtGameType::Id)_stats.GameType))
	{
		s32 redScore = 0;
		s32 blueScore = 0;
		if((_stats.ValidTeams & 3) == 3 &&
		   IsBitSet(GetTeamFlags(0), (s32)udtTeamStatsField::Score) &&
		   IsBitSet(GetTeamFlags(1), (s32)udtTeamStatsField::Score))
		{
			redScore = GetTeamFields(0)[udtTeamStatsField::Score];
			blueScore = GetTeamFields(1)[udtTeamStatsField::Score];
			_stats.FirstPlaceScore = udt_max(redScore, blueScore);
			_stats.SecondPlaceScore = udt_min(redScore, blueScore);
			_stats.FirstPlaceName = redScore > blueScore ? "RED" : "BLUE";
			_stats.SecondPlaceName = redScore > blueScore ? "BLUE" : "RED";
		}
		else if(_analyzer.Mod() == udtMod::CPMA)
		{
			redScore = _cpmaScoreRed;
			blueScore = _cpmaScoreBlue;
			_stats.FirstPlaceScore = udt_max(redScore, blueScore);
			_stats.SecondPlaceScore = udt_min(redScore, blueScore);
			_stats.FirstPlaceName = redScore > blueScore ? "RED" : "BLUE";
			_stats.SecondPlaceName = redScore > blueScore ? "BLUE" : "RED";
		}
		else
		{
			// @TODO: How do we know if first place is red or blue?
			_stats.FirstPlaceScore = S32_MIN;
			_stats.SecondPlaceScore = S32_MIN;
			_stats.FirstPlaceName = NULL;
			_stats.SecondPlaceName = NULL;
		}
	}
	else
	{
		s32 firstPlaceScore = S32_MIN;
		s32 secondPlaceScore = S32_MIN;
		s32 firstPlaceIndex = -1;
		s32 secondPlaceIndex = -1;
		for(s32 i = 0; i < 64; ++i)
		{
			if((_stats.ValidPlayers & ((u64)1 << (u64)i)) != 0 &&
			   IsBitSet(GetPlayerFlags(i), (s32)udtPlayerStatsField::Score))
			{
				if(IsBitSet(GetPlayerFlags(i), (s32)udtPlayerStatsField::TeamIndex) && 
				   GetPlayerFields(i)[udtPlayerStatsField::TeamIndex] == (s32)udtTeam::Free)
				{
					const s32 score = GetPlayerFields(i)[udtPlayerStatsField::Score];
					if(score > firstPlaceScore)
					{
						secondPlaceScore = firstPlaceScore;
						firstPlaceScore = score;
						secondPlaceIndex = firstPlaceIndex;
						firstPlaceIndex = i;
					}
					else if(score > secondPlaceScore)
					{
						secondPlaceScore = score;
						secondPlaceIndex = i;
					}
				}
			}
		}

		if(firstPlaceScore != S32_MIN &&
		   secondPlaceScore != S32_MIN &&
		   firstPlaceIndex != -1 &&
		   secondPlaceIndex != -1)
		{
			_stats.FirstPlaceScore = firstPlaceScore;
			_stats.SecondPlaceScore = secondPlaceScore;
			_stats.FirstPlaceName = _playerStats[firstPlaceIndex].CleanName;
			_stats.SecondPlaceName = _playerStats[secondPlaceIndex].CleanName;
		}
		else if(_analyzer.Mod() != udtMod::CPMA &&
				_firstPlaceScore != S32_MIN &&
				_secondPlaceScore != S32_MIN &&
				_firstPlaceClientNumber >= 0 &&
				_firstPlaceClientNumber < 64 &&
				_secondPlaceClientNumber >= 0 &&
				_secondPlaceClientNumber < 64)
		{
			_stats.FirstPlaceScore = _firstPlaceScore;
			_stats.SecondPlaceScore = _secondPlaceScore;
			_stats.FirstPlaceName = _playerStats[_firstPlaceClientNumber].CleanName;
			_stats.SecondPlaceName = _playerStats[_secondPlaceClientNumber].CleanName;
		}
		else if(_analyzer.Mod() == udtMod::CPMA &&
				_cpmaScoreRed != S32_MIN &&
				_cpmaScoreBlue != S32_MIN &&
				_firstPlaceClientNumber >= 0 &&
				_firstPlaceClientNumber < 64 &&
				_secondPlaceClientNumber >= 0 &&
				_secondPlaceClientNumber < 64)
		{
			_stats.FirstPlaceScore = udt_max(_cpmaScoreRed, _cpmaScoreBlue);
			_stats.SecondPlaceScore = udt_min(_cpmaScoreRed, _cpmaScoreBlue);
			_stats.FirstPlaceName = _playerStats[_firstPlaceClientNumber].CleanName;
			_stats.SecondPlaceName = _playerStats[_secondPlaceClientNumber].CleanName;
		}
		else
		{
			_stats.FirstPlaceScore = S32_MIN;
			_stats.SecondPlaceScore = S32_MIN;
			_stats.FirstPlaceName = NULL;
			_stats.SecondPlaceName = NULL;
		}
	}
	
	_stats.GamePlay = (u32)_analyzer.GamePlay();
	_stats.Map = _analyzer.MapName();
	_stats.OverTimeCount = _analyzer.OvertimeCount();
	_stats.OverTimeType = (u32)_analyzer.OvertimeType();
	_stats.Mod = _analyzer.Mod();
	_stats.ModVersion = _analyzer.ModVersion();
	_stats.Forfeited = _analyzer.Forfeited() ? 1 : 0;
	_stats.TimeOutCount = _analyzer.TimeOutCount();
	_stats.TotalTimeOutDurationMs = _analyzer.TotalTimeOutDuration();
	_stats.MercyLimited = _analyzer.MercyLimited();
	_statsArray.Add(_stats);

	// Clear the stats for the next match.
	ClearStats();
}

void udtParserPlugInStats::ClearStats()
{
	memset(&_stats, 0, sizeof(_stats));
	memset(_playerStats, 0, sizeof(_playerStats));
	memset(_playerFields, 0, sizeof(_playerFields));
	memset(_playerFlags, 0, sizeof(_playerFlags));
	memset(_teamFields, 0, sizeof(_teamFields));
	memset(_teamFlags, 0, sizeof(_teamFlags));

	_stats.GameType = (u32)udtGameType::Invalid;
	for(s32 i = 0; i < 64; ++i)
	{
		_playerTeamIndices[i] = -1;
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

void udtParserPlugInStats::ComputePlayerAccuracies(s32 clientNumber)
{
#define COMPUTE_ACC(Weapon) ComputePlayerAccuracy(clientNumber, (s32)udtPlayerStatsField::Weapon##Accuracy, (s32)udtPlayerStatsField::Weapon##Hits, (s32)udtPlayerStatsField::Weapon##Shots)
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

void udtParserPlugInStats::ComputePlayerAccuracy(s32 clientNumber, s32 acc, s32 hits, s32 shots)
{
	u8* const flags = GetPlayerFlags(clientNumber);
	if(IsBitSet(flags, acc) ||
	   !IsBitSet(flags, hits) ||
	   !IsBitSet(flags, shots))
	{
		return;
	}

	s32* const fields = GetPlayerFields(clientNumber);
	const s32 hitCount = fields[hits];
	const s32 shotCount = fields[shots];
	fields[acc] = shotCount == 0 ? 0 : (((100 * hitCount) + (shotCount / 2)) / shotCount);
	SetBit(flags, acc);
}
