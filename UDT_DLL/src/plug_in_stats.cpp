#include "plug_in_stats.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


static_assert((s32)udtTeamStatsField::Count <= (s32)(UDT_TEAM_STATS_MASK_BYTE_COUNT * 8), "Too many team stats fields for the bit mask size");
static_assert((s32)udtPlayerStatsField::Count <= (s32)(UDT_PLAYER_STATS_MASK_BYTE_COUNT * 8), "Too many player stats fields for the bit mask size");


/*
CPMA stats/scores commands:
- mstats           full stats for one player sent multiple times
- xstats2          full stats for one player sent once at the end of a game only, encoded the same as mstats
- mstatsa          same as mstats but winner/loser have client numbers 0/1
- xstats2a         same as xstats2 but winner/loser have client numbers 0/1
- duelendscores    sent at the end of a 1v1/hm match, contains winner name, loser name, victory type
- scores           team and player scores sent multiple times during a game
- dmscores         first and second place client numbers for non-team games sent whenever it changes
*/


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

static u32 PopCount(u32 bitMask)
{
	u32 count = 0;
	for(u32 i = 0; i < 32; ++i)
	{
		if((bitMask & ((u32)1 << (u32)i)) != 0)
		{
			++count;
		}
	}

	return count;
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
	_plugInTokenizer = NULL;
	_protocol = udtProtocol::Invalid;
	_followedClientNumber = -1;
	_disableStatsOverrides = false;
	_lastMatchEndTime = UDT_S32_MIN;
	ClearStats();
}

udtParserPlugInStats::~udtParserPlugInStats()
{
}

void udtParserPlugInStats::InitAllocators(u32 demoCount)
{
	_analyzer.InitAllocators(*TempAllocator, demoCount);

	_redString = udtString::NewClone(_stringAllocator, "RED");
	_blueString = udtString::NewClone(_stringAllocator, "BLUE");
}

void udtParserPlugInStats::CopyBuffersStruct(void* buffersStruct) const
{
	*(udtParseDataStatsBuffers*)buffersStruct = _buffers;
}

void udtParserPlugInStats::UpdateBufferStruct()
{
	_buffers.MatchStatsRanges = BufferRanges.GetStartAddress();
	_buffers.MatchStats = _statsArray.GetStartAddress();
	_buffers.MatchCount = _statsArray.GetSize();
	_buffers.TimeOutStartAndEndTimes = _timeOutTimes.GetStartAddress();
	_buffers.TimeOutRangeCount = _timeOutTimes.GetSize() / 2;
	_buffers.TeamFlags = _teamFlagsArray.GetStartAddress();
	_buffers.TeamFlagCount = _teamFlagsArray.GetSize();
	_buffers.PlayerFlags = _playerFlagsArray.GetStartAddress();
	_buffers.PlayerFlagCount = _playerFlagsArray.GetSize();
	_buffers.TeamFields = _teamFieldsArray.GetStartAddress();
	_buffers.TeamFieldCount = _teamFieldsArray.GetSize();
	_buffers.PlayerFields = _playerFieldsArray.GetStartAddress();
	_buffers.PlayerFieldCount = _playerFieldsArray.GetSize();
	_buffers.PlayerStats = _playerStatsArray.GetStartAddress();
	_buffers.PlayerStatsCount = _playerStatsArray.GetSize();
	_buffers.StringBuffer = _stringAllocator.GetStartAddress();
	_buffers.StringBufferSize = (u32)_stringAllocator.GetCurrentByteCount();
}

u32 udtParserPlugInStats::GetItemCount() const
{
	return _statsArray.GetSize();
}

void udtParserPlugInStats::StartDemoAnalysis()
{
	_analyzer.ResetForNextDemo();

	_tokenizer = NULL;
	_plugInTokenizer = NULL;
	_protocol = udtProtocol::Invalid;
	_followedClientNumber = -1;
	_disableStatsOverrides = false;
	_lastMatchEndTime = UDT_S32_MIN;
	ClearStats();
}

void udtParserPlugInStats::FinishDemoAnalysis()
{
	_analyzer.FinishDemoAnalysis();
	if(_analyzer.IsMatchInProgress() || _analyzer.IsInIntermission())
	{
		if(_analyzer.IsInIntermission())
		{
			_analyzer.SetIntermissionEndTime();
		}
		AddCurrentStats();
	}
}

void udtParserPlugInStats::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	if(_analyzer.GameStateIndex() >= 0 && 
	   (_analyzer.IsMatchInProgress() || _analyzer.IsInIntermission()))
	{
		if(_analyzer.IsInIntermission())
		{
			_analyzer.SetIntermissionEndTime();
		}
		AddCurrentStats();
	}
	_analyzer.ProcessGamestateMessage(arg, parser);

	_tokenizer = &parser.GetTokenizer();
	_plugInTokenizer = &parser._context->Tokenizer;
	_protocol = parser._inProtocol;
	_followedClientNumber = -1;

	_firstPlaceClientNumber = -1;
	_secondPlaceClientNumber = -1;
	_cpmaScoreRed = 0;
	_cpmaScoreBlue = 0;
	_cpmaRoundScoreRed = 0;
	_cpmaRoundScoreBlue = 0;
	_firstPlaceScore = 0;
	_secondPlaceScore = 0;
	_lastMatchEndTime = UDT_S32_MIN;

	ClearStats(true);

	const s32 firstPlayerCs = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, _protocol);
	for(s32 i = 0; i < 64; ++i)
	{
		ProcessPlayerConfigString(parser.GetConfigString(firstPlayerCs + i).GetPtr(), i);
	}

	if(_analyzer.Mod() == udtMod::CPMA)
	{
		ProcessConfigString(CS_CPMA_GAME_INFO, parser.GetConfigString(CS_CPMA_GAME_INFO));
		ProcessConfigString(CS_CPMA_ROUND_INFO, parser.GetConfigString(CS_CPMA_ROUND_INFO));
	}
	else
	{
		const s32 scores1Id = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::Scores1, _protocol);
		const s32 scores2Id = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::Scores2, _protocol);
		ProcessConfigString(scores1Id, parser.GetConfigString(scores1Id));
		ProcessConfigString(scores2Id, parser.GetConfigString(scores2Id));
	}
}

void udtParserPlugInStats::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	// We can't add a match right after it ends because some scores and stats info will be sent after it ended.
	// So we add stats when a match starts (that isn't the first one) or the demo ended.
	_analyzer.ProcessCommandMessage(arg, parser);
	if(_analyzer.CanMatchBeAdded())
	{
		AddCurrentStats();
		_analyzer.ResetForNextMatch();
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

	if(!_analyzer.IsMatchInProgress() &&
	   !_analyzer.IsInIntermission())
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
		HANDLER("mstats", ParseCPMAMStats),
		HANDLER("xstats2a", ParseCPMAXStats2a),
		HANDLER("mstatsa", ParseCPMAMStatsa),
		HANDLER("duelendscores", ParseCPMADuelEndScores),
		HANDLER("xscores", ParseCPMAXScores),
		HANDLER("dmscores", ParseCPMADMScores),
		HANDLER("tdmscores", ParseQLScoresTDMVeryOld),
		HANDLER("tdmscores2", ParseQLScoresTDMOld),
		HANDLER("statsinfo", ParseOSPStatsInfo),
		HANDLER("scores_ca", ParseQLScoresCA),
		HANDLER("ctfscores", ParseQLScoresCTFOld),
		HANDLER("cascores", ParseQLScoresCAOld),
		HANDLER("castats", ParseQLStatsCA),
		HANDLER("xstats1", ParseOSPXStats1),
		HANDLER("adscores", ParseQLScoresAD),
		HANDLER("scores_ad", ParseQLScoresAD),
		HANDLER("scores_ft", ParseQLScoresFT),
		HANDLER("rrscores", ParseQLScoresRROld),
		HANDLER("scores_rr", ParseQLScoresRR),
		HANDLER("print", ParsePrint)
	};
#undef HANDLER
	/*
	@TODO:
	QL  : scores_race ? (there is no such thing as a race match I believe...)
	OSP : bstats - can't find a demo with "bstats" anymore :-(
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

void udtParserPlugInStats::ClearMatchList()
{
	_statsArray.Clear();
	_stringAllocator.Clear();

	_teamFlagsArray.Clear();
	_playerFlagsArray.Clear();
	_teamFieldsArray.Clear();
	_playerFieldsArray.Clear();
	_playerStatsArray.Clear();
	_timeOutTimes.Clear();

	_redString = udtString::NewClone(_stringAllocator, "RED");
	_blueString = udtString::NewClone(_stringAllocator, "BLUE");
}

void udtParserPlugInStats::ProcessConfigString(s32 csIndex, const udtString& configString)
{	
	if(udtString::IsNull(configString))
	{
		return;
	}

	const s32 firstPlayerCs = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, _protocol);
	if(csIndex >= firstPlayerCs && csIndex < firstPlayerCs + 64)
	{
		ProcessPlayerConfigString(configString.GetPtr(), csIndex - firstPlayerCs);
	}
	else if(_analyzer.Mod() == udtMod::CPMA && csIndex == CS_CPMA_GAME_INFO)
	{
		udtVMScopedStackAllocator allocatorScope(*TempAllocator);
		s32 sr, sb;
		ParseConfigStringValueInt(sr, *TempAllocator, "sr", configString.GetPtr());
		ParseConfigStringValueInt(sb, *TempAllocator, "sb", configString.GetPtr());
		if(sr != -9999 || sb != -9999)
		{
			// When both scores are -9999, CPMA did an arena reset.
			_cpmaScoreRed = sr;
			_cpmaScoreBlue = sb;
		}

		udtString redTeamName, blueTeamName;
		if(ParseConfigStringValueString(redTeamName, *TempAllocator, "nr", configString.GetPtr()) &&
		   redTeamName.GetLength() != 0)
		{
			WriteStringToApiStruct(_stats.CustomRedName, udtString::NewCleanCloneFromRef(_stringAllocator, _protocol, redTeamName));
		}
		if(ParseConfigStringValueString(blueTeamName, *TempAllocator, "nb", configString.GetPtr()) &&
		   blueTeamName.GetLength() != 0)
		{
			WriteStringToApiStruct(_stats.CustomBlueName, udtString::NewCleanCloneFromRef(_stringAllocator, _protocol, blueTeamName));
		}
		
		s32 cr, cb;
		if(_analyzer.GameType() >= udtGameType::FirstTeamMode && 
		   _analyzer.IsMatchInProgress() &&
		   _cpmaRoundScoreRed != _cpmaRoundScoreBlue &&
		   ParseConfigStringValueInt(cr, *TempAllocator, "cr", configString.GetPtr()) &&
		   ParseConfigStringValueInt(cb, *TempAllocator, "cb", configString.GetPtr()) &&
		   ((_cpmaRoundScoreRed > _cpmaRoundScoreBlue && cr == 0) || (_cpmaRoundScoreBlue > _cpmaRoundScoreRed && cb == 0)))
		{
			// All the players of the leading team left.
			_stats.SecondPlaceWon = 1;
		}
	}
	else if(_analyzer.Mod() == udtMod::CPMA && csIndex == CS_CPMA_ROUND_INFO)
	{
		s32 sr, sb;
		ParseConfigStringValueInt(sr, *TempAllocator, "sr", configString.GetPtr());
		ParseConfigStringValueInt(sb, *TempAllocator, "sb", configString.GetPtr());
		if(sr != -9999 && sb != -9999)
		{
			_cpmaRoundScoreRed = sr;
			_cpmaRoundScoreBlue = sb;
		}
		else if(_analyzer.GameType() < udtGameType::FirstTeamMode && 
				_firstPlaceClientNumber >= 0 &&
				_firstPlaceClientNumber < 64 &&
				_playerTeamIndices[_firstPlaceClientNumber] == (s32)udtTeam::Spectators)
		{
			_stats.SecondPlaceWon = 1;
		}
	}
	else if(_analyzer.Mod() != udtMod::CPMA && 
			csIndex == GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::Scores1, _protocol))
	{
		s32 score = -1;
		if(StringParseInt(score, configString.GetPtr()))
		{
			if(score != -999)
			{
				_firstPlaceScore = score;
			}
			else if(_analyzer.GameType() >= udtGameType::FirstTeamMode &&
					_firstPlaceScore > _secondPlaceScore)
			{
				// In team modes, scores1 is red and scores2 is blue.
				_stats.SecondPlaceWon = 1;
			}
		}
	}
	else if(_analyzer.Mod() != udtMod::CPMA && 
			csIndex == GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::Scores2, _protocol))
	{
		s32 score = -1;
		if(StringParseInt(score, configString.GetPtr()))
		{
			if(score != -999)
			{
				_secondPlaceScore = score;
			}
			else if(_analyzer.GameType() >= udtGameType::FirstTeamMode &&
					_secondPlaceScore > _firstPlaceScore)
			{
				// In team modes, scores1 is red and scores2 is blue.
				_stats.SecondPlaceWon = 1;
			}
		}
	}
}

void udtParserPlugInStats::ProcessPlayerConfigString(const char* configString, s32 playerIndex)
{
	if(configString == NULL)
	{
		return;
	}
#if 0
	if(*configString == '\0')
	{
		// Player disconnected, invalidate all fields.
		_playerTeamIndices[playerIndex] = -1;
		_playerStats[playerIndex].Name = NULL;
		_playerStats[playerIndex].CleanName = NULL;
		return;
	}
#endif
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
		WriteStringToApiStruct(_playerStats[playerIndex].Name, udtString::NewCloneFromRef(_stringAllocator, name));
		WriteStringToApiStruct(_playerStats[playerIndex].CleanName, udtString::NewCleanCloneFromRef(_stringAllocator, _protocol, name));
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

	s32 baseOffset = 32;
	if(_tokenizer->GetArgCount() < (u32)baseOffset)
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

	static const udtStatsField playerFields90[] =
	{
		// We skip the subscriber field.
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
		PLAYER_FIELD(TeamKills, 11),
		PLAYER_FIELD(TeamKilled, 12),
		PLAYER_FIELD(DamageGiven, 13)
	};

	static const udtStatsField playerFields91[] =
	{
		PLAYER_FIELD(Score, 0),
		PLAYER_FIELD(Ping, 1),
		PLAYER_FIELD(Time, 2),
		PLAYER_FIELD(Kills, 3),
		PLAYER_FIELD(Deaths, 4),
		PLAYER_FIELD(Accuracy, 5),
		PLAYER_FIELD(BestWeapon, 6),
		PLAYER_FIELD(Impressives, 7),
		PLAYER_FIELD(Excellents, 8),
		PLAYER_FIELD(Gauntlets, 9),
		PLAYER_FIELD(TeamKills, 10),
		PLAYER_FIELD(TeamKilled, 11),
		PLAYER_FIELD(DamageGiven, 12)
	};

	static const udtStatsField teamScoreFields[] =
	{
		TEAM_FIELD(Score, 0)
	};
	
	ParseTeamFields(0, teamFields, (s32)UDT_COUNT_OF(teamFields));
	ParseTeamFields(1, teamFields, (s32)UDT_COUNT_OF(teamFields), 14);

	const s32 scoreCount = GetValue(29);

	// Some old demos don't have the team scores...
	bool hasTeamScores = true;
	if(_protocol <= udtProtocol::Dm90 &&
	   _tokenizer->GetArgCount() <= (u32)(30 + scoreCount * 16))
	{
		hasTeamScores = false;
		baseOffset = 30;
	}

	if(hasTeamScores)
	{
		ParseTeamFields(0, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 30);
		ParseTeamFields(1, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 31);
	}

	const s32 statsPerPlayer = _protocol <= udtProtocol::Dm90 ? 16 : 15;
	if(_tokenizer->GetArgCount() < (u32)(baseOffset + scoreCount * statsPerPlayer))
	{
		return;
	}

	const udtStatsField* const playerFields = _protocol <= udtProtocol::Dm90 ? playerFields90 : playerFields91;
	const s32 playerFieldCount = _protocol <= udtProtocol::Dm90 ? (s32)UDT_COUNT_OF(playerFields90) : (s32)UDT_COUNT_OF(playerFields91);

	s32 offset = baseOffset;
	for(s32 i = 0; i < scoreCount; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			SetPlayerField(clientNumber, udtPlayerStatsField::TeamIndex, GetValue(offset + 1));
			ParsePlayerFields(clientNumber, playerFields, playerFieldCount, offset + 2);
		}

		offset += statsPerPlayer;
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
			SetPlayerField(clientNumber, udtPlayerStatsField::TeamIndex, (s32)udtTeam::Free);
			ParsePlayerFields(clientNumber, fields, (s32)UDT_COUNT_OF(fields), offset);
		}
		offset += (s32)UDT_COUNT_OF(fields);
	}
}

void udtParserPlugInStats::ParseQLScoresCTF()
{
	// CTFS games use the CTF scores/stats commands too.
	if(_stats.GameType != (u32)udtGameType::CTFS)
	{
		_stats.GameType = (u32)udtGameType::CTF;
	}

	s32 baseOffset = 38;
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

	static const udtStatsField playerFields91[] =
	{
		PLAYER_FIELD(Score, 0),
		PLAYER_FIELD(Ping, 1),
		PLAYER_FIELD(Time, 2),
		PLAYER_FIELD(Kills, 3),
		PLAYER_FIELD(Deaths, 4),
		PLAYER_FIELD(Accuracy, 5),
		PLAYER_FIELD(BestWeapon, 6),
		PLAYER_FIELD(Impressives, 7),
		PLAYER_FIELD(Excellents, 8),
		PLAYER_FIELD(Gauntlets, 9),
		PLAYER_FIELD(Defends, 10),
		PLAYER_FIELD(Assists, 11),
		PLAYER_FIELD(Captures, 12),
		PLAYER_FIELD(Perfect, 13)
		// We skip the alive field.
	};

	static const udtStatsField playerFields90[] =
	{
		// We skip the subscriber field.
		PLAYER_FIELD(Score, 1),
		PLAYER_FIELD(Ping, 2),
		PLAYER_FIELD(Time, 3),
		PLAYER_FIELD(Kills, 4),
		PLAYER_FIELD(Deaths, 5),
		// We skip the power-ups field.
		PLAYER_FIELD(Accuracy, 7),
		PLAYER_FIELD(BestWeapon, 8),
		PLAYER_FIELD(Impressives, 9),
		PLAYER_FIELD(Excellents, 10),
		PLAYER_FIELD(Gauntlets, 11),
		PLAYER_FIELD(Defends, 12),
		PLAYER_FIELD(Assists, 13),
		PLAYER_FIELD(Captures, 14),
		PLAYER_FIELD(Perfect, 15)
		// We skip the alive field.
	};

	static const udtStatsField teamScoreFields[] =
	{
		TEAM_FIELD(Score, 0)
	};

	ParseTeamFields(0, teamFields, (s32)UDT_COUNT_OF(teamFields), 1);
	ParseTeamFields(1, teamFields, (s32)UDT_COUNT_OF(teamFields), 18);

	const s32 scoreCount = GetValue(35);

	// Some old demos don't have the team scores...
	bool hasTeamScores = true;
	if(_protocol <= udtProtocol::Dm90 && 
	   _tokenizer->GetArgCount() <= (u32)(36 + scoreCount * 19))
	{
		hasTeamScores = false;
		baseOffset = 36;
	}

	if(hasTeamScores)
	{
		ParseTeamFields(0, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 36);
		ParseTeamFields(1, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 37);
	}

	const s32 statsPerPlayer = _protocol <= udtProtocol::Dm90 ? 19 : 17;
	if(_tokenizer->GetArgCount() < (u32)(baseOffset + scoreCount * statsPerPlayer))
	{
		return;
	}

	const udtStatsField* const playerFields = _protocol <= udtProtocol::Dm90 ? playerFields90 : playerFields91;
	const s32 playerFieldCount = _protocol <= udtProtocol::Dm90 ? (s32)UDT_COUNT_OF(playerFields90) : (s32)UDT_COUNT_OF(playerFields91);
	
	s32 offset = baseOffset;
	for(s32 i = 0; i < scoreCount; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			SetPlayerField(clientNumber, udtPlayerStatsField::TeamIndex, GetValue(offset + 1));
			ParsePlayerFields(clientNumber, playerFields, playerFieldCount, offset + 2);
		}

		offset += statsPerPlayer;
	}
}

void udtParserPlugInStats::ParseQLStatsCTF()
{
	// CTFS games use the CTF scores/stats commands too.
	if(_stats.GameType != (u32)udtGameType::CTFS)
	{
		_stats.GameType = (u32)udtGameType::CTF;
	}

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
			SetPlayerField(clientNumber, udtPlayerStatsField::TeamIndex, (s32)udtTeam::Free);
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

void udtParserPlugInStats::ParseCPMAStats(bool endGameStats, bool newEncoding)
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
		// 8: power-up mask
		// for each power-up:
		// - number of pickups
		// - total carry time
	};

	static const udtStatsField playerFields1[] =
	{
		PLAYER_FIELD(Score, 0),
		PLAYER_FIELD(Kills, 1),
		PLAYER_FIELD(Deaths, 2),
		PLAYER_FIELD(Suicides, 3),
		PLAYER_FIELD(TeleFrags, 4),
		PLAYER_FIELD(TeamDamage, 5),
		PLAYER_FIELD(TeamKills, 6),
		PLAYER_FIELD(RocketLauncherDamage, 7),
		PLAYER_FIELD(RocketLauncherDirectHits, 8)
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
		else
		{
			for(s32 j = 0; j < (s32)UDT_COUNT_OF(weaponFields[i]); ++j)
			{
				SetPlayerField(clientNumber, (udtPlayerStatsField::Id)weaponFields[i][j].Index, 0);
			}
		}
	}

	ParsePlayerFields(clientNumber, playerFields0, (s32)UDT_COUNT_OF(playerFields0), offset);
	offset += 8;

	for(u32 i = (u32)offset, count = _tokenizer->GetArgCount(); i < count; ++i)
	{
		const udtString token = _tokenizer->GetArg(i);
		const char* const tokenString = token.GetPtr();
		if(token.GetLength() >= 4 && (tokenString[0] < '0' || tokenString[0] > '9'))
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

	if(endGameStats && newEncoding && clientNumber == 0 && _cpma150DuelEndScoresState == 1)
	{
		_cpma150DuelEndScoresState = 2;
	}

	if(endGameStats && newEncoding && clientNumber == 1 && _cpma150DuelEndScoresState == 2)
	{
		for(u32 i = 2; i < 64; ++i)
		{
			memset(_playerFields[i], 0, sizeof(_playerFields[i]));
			memset(_playerFlags[i], 0, sizeof(_playerFlags[i]));
		}
		SetPlayerField(0, udtPlayerStatsField::TeamIndex, 0);
		SetPlayerField(1, udtPlayerStatsField::TeamIndex, 0);
		_cpma150ValidDuelEndScores = true;
		_cpma150DuelEndScoresState = 0;
	}
}

void udtParserPlugInStats::ParseCPMAXStats2()
{
	ParseCPMAStats(true, false);
}

void udtParserPlugInStats::ParseCPMAMStats()
{
	ParseCPMAStats(false, false);
}

void udtParserPlugInStats::ParseCPMAXStats2a()
{
	ParseCPMAStats(true, true);
}

void udtParserPlugInStats::ParseCPMAMStatsa()
{
	ParseCPMAStats(false, true);
}

void udtParserPlugInStats::ParseCPMADuelEndScores()
{
	const udtString winnerName = udtString::NewClone(_stringAllocator, _tokenizer->GetArgString(1));
	const udtString loserName = udtString::NewClone(_stringAllocator, _tokenizer->GetArgString(2));
	const udtString winnerNameClean = udtString::NewCleanCloneFromRef(_stringAllocator, _protocol, winnerName);
	const udtString loserNameClean = udtString::NewCleanCloneFromRef(_stringAllocator, _protocol, loserName);
	WriteStringToApiStruct(_playerStats[0].Name, winnerName);
	WriteStringToApiStruct(_playerStats[1].Name, loserName);
	WriteStringToApiStruct(_playerStats[0].CleanName, winnerNameClean);
	WriteStringToApiStruct(_playerStats[1].CleanName, loserNameClean);

	_firstPlaceClientNumber = 0;
	_secondPlaceClientNumber = 1;
	_cpma150ValidDuelEndScores = false;
	_cpma150DuelEndScoresState = 1;
	_cpma150Forfeit = GetValue(3) == 1;
}

void udtParserPlugInStats::ParseCPMAXScores()
{
	if(_cpma150ValidDuelEndScores)
	{
		return;
	}

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
			if(textField.GetLength() >= 4)
			{
				// It seems that 3-4 and 5-6 give the low and high values of the ping range.
				const char* const textFieldString = textField.GetPtr();
				const s32 ping = 32 * CPMACharToInt(textFieldString[2]) + CPMACharToInt(textFieldString[3]);
				SetPlayerField(clientNumber, udtPlayerStatsField::Ping, ping);
			}
		}

		offset += 7;
	}
}

void udtParserPlugInStats::ParseCPMADMScores()
{
	if(_cpma150ValidDuelEndScores)
	{
		return;
	}

	if(_tokenizer->GetArgCount() < 3 ||
	   !_analyzer.IsMatchInProgress())
	{
		return;
	}

	_firstPlaceClientNumber = GetValue(1);
	_secondPlaceClientNumber = GetValue(2);
}

void udtParserPlugInStats::ParseQLScoresTDMVeryOld()
{
	_stats.GameType = (u32)udtGameType::TDM;

	const s32 baseOffset = 17;
	if(_tokenizer->GetArgCount() < (u32)baseOffset)
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

	const s32 statsPerPlayer = 5;
	const s32 playerScores = ((s32)_tokenizer->GetArgCount() - baseOffset) / statsPerPlayer;

	s32 offset = baseOffset;
	for(s32 i = 0; i < playerScores; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			ParsePlayerFields(clientNumber, playerFields, (s32)UDT_COUNT_OF(playerFields), offset);
		}

		offset += statsPerPlayer;
	}
}

void udtParserPlugInStats::ParseQLScoresTDMOld()
{
	_stats.GameType = (u32)udtGameType::TDM;

	const s32 baseOffset = 29;
	if(_tokenizer->GetArgCount() < (u32)baseOffset)
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

	const s32 statsPerPlayer = 6;
	const s32 playerScores = ((s32)_tokenizer->GetArgCount() - baseOffset) / statsPerPlayer;

	s32 offset = baseOffset;
	for(s32 i = 0; i < playerScores; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			ParsePlayerFields(clientNumber, playerFields, (s32)UDT_COUNT_OF(playerFields), offset);
		}

		offset += statsPerPlayer;
	}
}

void udtParserPlugInStats::ParseOSPStatsInfo()
{
	// 22 fields including the weapon mask.
	const s32 baseOffset = 23;
	if(_followedClientNumber < 0 || 
	   _followedClientNumber >= 64 ||
	   _tokenizer->GetArgCount() < (u32)baseOffset)
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
	SetPlayerField(_followedClientNumber, udtPlayerStatsField::ArmorTaken, armorTaken);
	SetPlayerField(_followedClientNumber, udtPlayerStatsField::HealthTaken, healthTaken);

	const s32 statsPerWeapon = 4;
	const s32 weaponMask = GetValue(22);
	const s32 weaponCount = (s32)PopCount((u32)weaponMask);

	if(_tokenizer->GetArgCount() < (u32)(baseOffset + weaponCount * statsPerWeapon))
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

	s32 offset = baseOffset;
	for(s32 i = 1; i < 11; ++i)
	{
		if((weaponMask & (1 << i)) == 0)
		{
			for(s32 j = 0; j < (s32)UDT_COUNT_OF(weaponFieldIndices[i]); ++j)
			{
				SetPlayerField(_followedClientNumber, (udtPlayerStatsField::Id)weaponFieldIndices[i][j], 0);
			}
			continue;
		}

		const s32* const indices = weaponFieldIndices[i];
		const s32 hitsAndDrops = GetValue(offset);
		const s32 shotsAndPickups = GetValue(offset + 1);
		const s32 kills = GetValue(offset + 2);
		const s32 deaths = GetValue(offset + 3);
		const s32 hits = hitsAndDrops & 0xFFFF;
		const s32 drops = (hitsAndDrops >> 16) & 0xFFFF;
		const s32 shots = shotsAndPickups & 0xFFFF;
		const s32 pickups = (shotsAndPickups >> 16) & 0xFFFF;
		SetPlayerField(_followedClientNumber, (udtPlayerStatsField::Id)indices[0], hits);
		SetPlayerField(_followedClientNumber, (udtPlayerStatsField::Id)indices[1], drops);
		SetPlayerField(_followedClientNumber, (udtPlayerStatsField::Id)indices[2], shots);
		SetPlayerField(_followedClientNumber, (udtPlayerStatsField::Id)indices[3], pickups);
		SetPlayerField(_followedClientNumber, (udtPlayerStatsField::Id)indices[4], kills);
		SetPlayerField(_followedClientNumber, (udtPlayerStatsField::Id)indices[5], deaths);

		offset += statsPerWeapon;
	}
}

void udtParserPlugInStats::ParseOSPXStats1()
{
	if(_tokenizer->GetArgCount() < 2)
	{
		return;
	}

	const s32 clientNumber = GetValue(1);
	if(clientNumber < 0 || clientNumber >= 64)
	{
		return;
	}

	const s32 weaponMask = GetValue(2);
	const s32 weaponCount = (s32)PopCount((u32)weaponMask);

	if(_tokenizer->GetArgCount() < (u32)(3 + 4 * weaponCount))
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

	s32 offset = 3;
	for(s32 i = 1; i < 11; ++i)
	{
		if((weaponMask & (1 << i)) == 0)
		{
			for(s32 j = 0; j < (s32)UDT_COUNT_OF(weaponFieldIndices[i]); ++j)
			{
				SetPlayerField(clientNumber, (udtPlayerStatsField::Id)weaponFieldIndices[i][j], 0);
			}
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
		SetPlayerField(clientNumber, (udtPlayerStatsField::Id)indices[0], hits);
		SetPlayerField(clientNumber, (udtPlayerStatsField::Id)indices[1], drops);
		SetPlayerField(clientNumber, (udtPlayerStatsField::Id)indices[2], shots);
		SetPlayerField(clientNumber, (udtPlayerStatsField::Id)indices[3], pickups);
		SetPlayerField(clientNumber, (udtPlayerStatsField::Id)indices[4], kills);
		SetPlayerField(clientNumber, (udtPlayerStatsField::Id)indices[5], deaths);
	}

	if(_tokenizer->GetArgCount() < (u32)(offset + 8))
	{
		return;
	}

	static const udtStatsField fields[] =
	{
		PLAYER_FIELD(ArmorTaken, 0),
		PLAYER_FIELD(HealthTaken, 1),
		PLAYER_FIELD(DamageGiven, 2),
		PLAYER_FIELD(DamageReceived, 3),
		PLAYER_FIELD(MegaHealthPickups, 4),
		PLAYER_FIELD(GreenArmorPickups, 5),
		PLAYER_FIELD(RedArmorPickups, 6),
		PLAYER_FIELD(YellowArmorPickups, 7)
	};

	ParsePlayerFields(clientNumber, fields, (s32)UDT_COUNT_OF(fields), offset);
}

void udtParserPlugInStats::ParseQLScoresCA()
{
	_stats.GameType = (u32)udtGameType::CA;

	const s32 baseOffset = 4;
	if(_tokenizer->GetArgCount() < (u32)baseOffset)
	{
		return;
	}

	static const udtStatsField teamFields[] =
	{
		TEAM_FIELD(Score, 0)
	};

	// The order is:
	// - client number
	// - team index
	// - subscriber (optional)
	// - the rest
	static const udtStatsField playerFields[] =
	{
		PLAYER_FIELD(Score, 0),
		PLAYER_FIELD(Ping, 1),
		PLAYER_FIELD(Time, 2),
		PLAYER_FIELD(Kills, 3),
		PLAYER_FIELD(Deaths, 4),
		PLAYER_FIELD(Accuracy, 5),
		PLAYER_FIELD(BestWeapon, 6),
		PLAYER_FIELD(BestWeaponAccuracy, 7),
		PLAYER_FIELD(DamageGiven, 8),
		PLAYER_FIELD(Impressives, 9),
		PLAYER_FIELD(Excellents, 10),
		PLAYER_FIELD(Gauntlets, 11),
		PLAYER_FIELD(Perfect, 12)
		// We skip the alive field.
	};

	const s32 scoreCount = GetValue(1);
	ParseTeamFields(0, teamFields, (s32)UDT_COUNT_OF(teamFields), 2);
	ParseTeamFields(1, teamFields, (s32)UDT_COUNT_OF(teamFields), 3);

	const s32 minFieldCount = 16;
	if(_tokenizer->GetArgCount() < (u32)(baseOffset + scoreCount * minFieldCount))
	{
		return;
	}

	const s32 statsPerPlayer = _tokenizer->GetArgCount() >= (u32)(baseOffset + scoreCount * (minFieldCount + 1)) ? (minFieldCount + 1) : minFieldCount;
	const s32 secondPartOffset = statsPerPlayer - minFieldCount;

	s32 offset = baseOffset;
	for(s32 i = 0; i < scoreCount; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			SetPlayerField(clientNumber, udtPlayerStatsField::TeamIndex, GetValue(offset + 1));
			ParsePlayerFields(clientNumber, playerFields, (s32)UDT_COUNT_OF(playerFields), offset + 2 + secondPartOffset);
		}

		offset += statsPerPlayer;
	}
}

void udtParserPlugInStats::ParseQLScoresCTFOld()
{
	// CTFS games use the CTF scores/stats commands too.
	if(_stats.GameType != (u32)udtGameType::CTFS)
	{
		_stats.GameType = (u32)udtGameType::CTF;
	}

	const s32 baseOffset = 35;
	if(_tokenizer->GetArgCount() < (u32)baseOffset)
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
		PLAYER_FIELD(TeamIndex, 0)
		// We skip the subscriber field.
	};

	ParseTeamFields(0, teamFields, (s32)UDT_COUNT_OF(teamFields), 1);
	ParseTeamFields(1, teamFields, (s32)UDT_COUNT_OF(teamFields), 18);

	const s32 statsPerPlayer = 3;
	const s32 scoreCount = ((s32)_tokenizer->GetArgCount() - baseOffset) / statsPerPlayer;

	s32 offset = baseOffset;
	for(s32 i = 0; i < scoreCount; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			ParsePlayerFields(clientNumber, playerFields, (s32)UDT_COUNT_OF(playerFields), offset + 1);
		}

		offset += statsPerPlayer;
	}
}

void udtParserPlugInStats::ParseQLScoresCAOld()
{
	_stats.GameType = (u32)udtGameType::CA;

	static const udtStatsField playerFields[] =
	{
		PLAYER_FIELD(TeamIndex, 0),
		// We skip the subscriber field.
		PLAYER_FIELD(DamageGiven, 2),
		PLAYER_FIELD(Accuracy, 3)
	};

	const s32 statsPerPlayer = 5;
	const s32 scoreCount = ((s32)_tokenizer->GetArgCount() - 1) / statsPerPlayer;

	s32 offset = 1;
	for(s32 i = 0; i < scoreCount; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			ParsePlayerFields(clientNumber, playerFields, (s32)UDT_COUNT_OF(playerFields), offset + 1);
		}

		offset += statsPerPlayer;
	}
}

void udtParserPlugInStats::ParseQLStatsCA()
{
	_stats.GameType = (u32)udtGameType::CA;

	if(_tokenizer->GetArgCount() < 31)
	{
		return;
	}

#define WEAPON_FIELDS(Weapon, Offset) \
	PLAYER_FIELD(Weapon##Accuracy, Offset + 0), \
	PLAYER_FIELD(Weapon##Kills, Offset + 1)

	static const udtStatsField fields[] =
	{
		PLAYER_FIELD(DamageGiven, 0),
		PLAYER_FIELD(DamageReceived, 1),
		WEAPON_FIELDS(Gauntlet, 2),
		WEAPON_FIELDS(MachineGun, 4),
		WEAPON_FIELDS(Shotgun, 6),
		WEAPON_FIELDS(GrenadeLauncher, 8),
		WEAPON_FIELDS(RocketLauncher, 10),
		WEAPON_FIELDS(LightningGun, 12),
		WEAPON_FIELDS(Railgun, 14),
		WEAPON_FIELDS(PlasmaGun, 16),
		WEAPON_FIELDS(BFG, 18),
		WEAPON_FIELDS(GrapplingHook, 20),
		WEAPON_FIELDS(NailGun, 22),
		WEAPON_FIELDS(ProximityMineLauncher, 24),
		WEAPON_FIELDS(ChainGun, 26),
		WEAPON_FIELDS(HeavyMachineGun, 28)
	};

#undef WEAPON_FIELDS

	s32 clientNumber = -1;
	if(!GetClientNumberFromScoreIndex(clientNumber, 1))
	{
		return;
	}

	ParsePlayerFields(clientNumber, fields, (s32)UDT_COUNT_OF(fields), 2);
}

void udtParserPlugInStats::ParseQLScoresAD()
{
	_stats.GameType = (u32)udtGameType::CTFS;

	if(_tokenizer->GetArgCount() != 23)
	{
		return;
	}

	static const udtStatsField fields[] =
	{
		PLAYER_FIELD(TeamIndex, 0)
	};

	ParseTeamFields(0, fields, (s32)UDT_COUNT_OF(fields), 21);
	ParseTeamFields(1, fields, (s32)UDT_COUNT_OF(fields), 22);
}

void udtParserPlugInStats::ParseQLScoresFT()
{
	_stats.GameType = (u32)udtGameType::FT;

	if(_tokenizer->GetArgCount() < 30)
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
		TEAM_FIELD(QuadDamageTime, 9),
		TEAM_FIELD(BattleSuitTime, 10),
		TEAM_FIELD(RegenTime, 11),
		TEAM_FIELD(HasteTime, 12),
		TEAM_FIELD(InvisTime, 13)
	};

	static const udtStatsField teamScoreFields[] =
	{
		TEAM_FIELD(Score, 0)
	};

	ParseTeamFields(0, teamFields, (s32)UDT_COUNT_OF(teamFields), 1);
	ParseTeamFields(1, teamFields, (s32)UDT_COUNT_OF(teamFields), 15);

	const s32 scoreCount = GetValue(29);

	// Some old demos don't have the team scores...
	bool hasTeamScores = true;
	s32 baseOffset = 32;
	if(_protocol <= udtProtocol::Dm90 &&
	   _tokenizer->GetArgCount() <= (u32)(30 + scoreCount * 18))
	{
		hasTeamScores = false;
		baseOffset = 30;
	}

	if(hasTeamScores)
	{
		ParseTeamFields(0, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 30);
		ParseTeamFields(1, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 31);
	}

	const s32 statsPerPlayer = _protocol <= udtProtocol::Dm90 ? 18 : 17;
	if(_tokenizer->GetArgCount() < (u32)(baseOffset + scoreCount * statsPerPlayer))
	{
		return;
	}

	static const udtStatsField playerFields90[] =
	{
		// We skip the subscriber field.
		PLAYER_FIELD(Score, 1),
		PLAYER_FIELD(Ping, 2),
		PLAYER_FIELD(Time, 3),
		PLAYER_FIELD(Kills, 4),
		PLAYER_FIELD(Deaths, 5),
		PLAYER_FIELD(Accuracy, 6),
		PLAYER_FIELD(BestWeapon, 7),
		PLAYER_FIELD(Impressives, 8),
		PLAYER_FIELD(Excellents, 9),
		PLAYER_FIELD(Thaws, 10),
		PLAYER_FIELD(Assists, 11),
		PLAYER_FIELD(TeamKills, 12),
		PLAYER_FIELD(TeamKilled, 13),
		PLAYER_FIELD(DamageGiven, 14)
		// We skip the alive field.
	};

	static const udtStatsField playerFields91[] =
	{
		PLAYER_FIELD(Score, 0),
		PLAYER_FIELD(Ping, 1),
		PLAYER_FIELD(Time, 2),
		PLAYER_FIELD(Kills, 3),
		PLAYER_FIELD(Deaths, 4),
		PLAYER_FIELD(Accuracy, 5),
		PLAYER_FIELD(BestWeapon, 6),
		PLAYER_FIELD(Impressives, 7),
		PLAYER_FIELD(Excellents, 8),
		PLAYER_FIELD(Gauntlets, 9),
		PLAYER_FIELD(Thaws, 10),
		PLAYER_FIELD(TeamKills, 11),
		PLAYER_FIELD(TeamKilled, 12),
		PLAYER_FIELD(DamageGiven, 13)
		// We skip the alive field.
	};

	const udtStatsField* const playerFields = _protocol <= udtProtocol::Dm90 ? playerFields90 : playerFields91;
	const s32 playerFieldCount = _protocol <= udtProtocol::Dm90 ? (s32)UDT_COUNT_OF(playerFields90) : (s32)UDT_COUNT_OF(playerFields91);

	s32 offset = baseOffset;
	for(s32 i = 0; i < scoreCount; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			SetPlayerField(clientNumber, udtPlayerStatsField::TeamIndex, GetValue(offset + 1));
			ParsePlayerFields(clientNumber, playerFields, playerFieldCount, offset + 2);
		}

		offset += statsPerPlayer;
	}
}

void udtParserPlugInStats::ParseQLScoresRROld()
{
	_stats.GameType = (u32)udtGameType::RedRover;

	const s32 baseOffset = 4;
	if(_tokenizer->GetArgCount() < (u32)baseOffset)
	{
		return;
	}

	static const udtStatsField teamScoreFields[] =
	{
		TEAM_FIELD(Score, 0)
	};

	const s32 scoreCount = GetValue(1);
	ParseTeamFields(0, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 2);
	ParseTeamFields(1, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 3);

	const s32 playerFieldCount = 19;
	if(_tokenizer->GetArgCount() < (u32)(baseOffset + scoreCount * playerFieldCount))
	{
		return;
	}

	static const udtStatsField playerFields[] =
	{
		// We skip a lot of irrelevant fields.
		PLAYER_FIELD(Score, 0),
		PLAYER_FIELD(Ping, 2),
		PLAYER_FIELD(Time, 3),
		PLAYER_FIELD(Accuracy, 6),
		PLAYER_FIELD(Impressives, 7),
		PLAYER_FIELD(Excellents, 8),
		PLAYER_FIELD(Gauntlets, 9),
		PLAYER_FIELD(Defends, 10),
		PLAYER_FIELD(Assists, 11),
		PLAYER_FIELD(Kills, 15),
		PLAYER_FIELD(Deaths, 16),
		PLAYER_FIELD(BestWeapon, 17)
	};

	s32 offset = baseOffset;
	for(s32 i = 0; i < scoreCount; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			ParsePlayerFields(clientNumber, playerFields, (s32)UDT_COUNT_OF(playerFields), offset + 1);
		}

		offset += playerFieldCount;
	}
}

void udtParserPlugInStats::ParseQLScoresRR()
{
	_stats.GameType = (u32)udtGameType::RedRover;

	const s32 baseOffset = 4;
	if(_tokenizer->GetArgCount() < (u32)baseOffset)
	{
		return;
	}

	static const udtStatsField teamScoreFields[] =
	{
		TEAM_FIELD(Score, 0)
	};

	const s32 scoreCount = GetValue(1);
	ParseTeamFields(0, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 2);
	ParseTeamFields(1, teamScoreFields, (s32)UDT_COUNT_OF(teamScoreFields), 3);

	const s32 statsPerPlayer = _protocol <= udtProtocol::Dm90 ? 19 : 18;
	if(_tokenizer->GetArgCount() < (u32)(baseOffset + scoreCount * statsPerPlayer))
	{
		return;
	}

	static const udtStatsField playerFields90[] =
	{
		PLAYER_FIELD(Score, 0),
		// We skip the round score field.
		PLAYER_FIELD(Ping, 2),
		PLAYER_FIELD(Time, 3),
		PLAYER_FIELD(Kills, 4),
		PLAYER_FIELD(Deaths, 5),
		// We skip the power-ups field.
		PLAYER_FIELD(Accuracy, 7),
		PLAYER_FIELD(BestWeapon, 8),
		PLAYER_FIELD(BestWeaponAccuracy, 9),
		PLAYER_FIELD(Impressives, 10),
		PLAYER_FIELD(Excellents, 11),
		PLAYER_FIELD(Gauntlets, 12),
		PLAYER_FIELD(Defends, 13),
		PLAYER_FIELD(Assists, 14)
		// We skip the perfect, captures and alive fields.
	};

	static const udtStatsField playerFields91[] =
	{
		PLAYER_FIELD(Score, 0),
		// We skip the round score field.
		PLAYER_FIELD(Ping, 2),
		PLAYER_FIELD(Time, 3),
		PLAYER_FIELD(Kills, 4),
		PLAYER_FIELD(Deaths, 5),
		PLAYER_FIELD(Accuracy, 6),
		PLAYER_FIELD(BestWeapon, 7),
		PLAYER_FIELD(BestWeaponAccuracy, 8),
		PLAYER_FIELD(Impressives, 9),
		PLAYER_FIELD(Excellents, 10),
		PLAYER_FIELD(Gauntlets, 11),
		PLAYER_FIELD(Defends, 12),
		PLAYER_FIELD(Assists, 13)
		// We skip the perfect, captures and alive fields.
	};

	const udtStatsField* const playerFields = _protocol <= udtProtocol::Dm90 ? playerFields90 : playerFields91;
	const s32 playerFieldCount = _protocol <= udtProtocol::Dm90 ? (s32)UDT_COUNT_OF(playerFields90) : (s32)UDT_COUNT_OF(playerFields91);

	s32 offset = baseOffset;
	for(s32 i = 0; i < scoreCount; ++i)
	{
		const s32 clientNumber = GetValue(offset);
		if(clientNumber >= 0 && clientNumber < 64)
		{
			_playerIndices[i] = (u8)clientNumber;
			ParsePlayerFields(clientNumber, playerFields, playerFieldCount, offset + 1);
		}

		offset += statsPerPlayer;
	}
}

void udtParserPlugInStats::ParsePrint()
{
	if(_analyzer.Mod() == udtMod::CPMA && 
	   _analyzer.IsInIntermission())
	{
		ParseCPMAPrint();
	}
}

void udtParserPlugInStats::ParseCPMAPrint()
{
	if(_tokenizer->GetArgCount() != 2)
	{
		return;
	}

	const udtString message = _tokenizer->GetArg(1);
	udtString cleanMessage = udtString::NewCloneFromRef(*TempAllocator, message);
	udtString::CleanUp(cleanMessage, _protocol);
	udtString::RemoveCharacter(cleanMessage, '\n');

	if(udtString::IsNullOrEmpty(cleanMessage) ||
	   udtString::StartsWith(cleanMessage, "----") ||
	   udtString::StartsWith(cleanMessage, "RED Totals") ||
	   udtString::StartsWith(cleanMessage, "BLUE Totals"))
	{
		return;
	}

	if(udtString::StartsWith(cleanMessage, "TEAM") ||
	   udtString::StartsWith(cleanMessage, "Player"))
	{
		ParseCPMAPrintHeader(cleanMessage);
	}
	else
	{
		_disableStatsOverrides = true;
		ParseCPMAPrintStats(cleanMessage);
		_disableStatsOverrides = false;
	}
}

void udtParserPlugInStats::ParseCPMAPrintHeader(const udtString& message)
{
	const bool teamStats = 
		udtString::StartsWith(message, "TEAM") &&
		!udtString::Contains(message, "Player");
	_cpmaPrintStats.TeamStats = teamStats;

	struct CPMAField
	{
		const char* Name;
		s16 PlayerField1;
		s16 TeamField1;
		s16 TeamField2;
	};
	
#define CPMA_FIELD(Name, Type)                  { Name, (s16)udtPlayerStatsField::Type, (s16)udtTeamStatsField::Type, (s16)-666 }
#define CPMA_FIELD_CUSTOM(Name, Type)           { Name, (s16)Type, (s16)Type, (s16)-666 }
#define CPMA_PLAYER_FIELD(Name, Type)           { Name, (s16)udtPlayerStatsField::Type, (s16)-666, (s16)-666 }
#define CPMA_TEAM_FIELD_EX(Name, Type, Type2)   { Name, (s16)-666, (s16)udtTeamStatsField::Type, (s16)udtTeamStatsField::Type2 }
	static const CPMAField fields[] =
	{
		// Special fields:
		CPMA_FIELD_CUSTOM("Player", -1),
		CPMA_FIELD_CUSTOM("TEAM", -2),
		// Player only:
		CPMA_PLAYER_FIELD("Time", Time),
		// Team only:
		CPMA_TEAM_FIELD_EX("QuadDmg", QuadDamagePickups, QuadDamageTime),
		CPMA_TEAM_FIELD_EX("BatSuit", BattleSuitPickups, BattleSuitTime),
		CPMA_TEAM_FIELD_EX("Invis", InvisPickups, InvisTime),
		// Team + player:
		CPMA_FIELD("Kll", Kills),
		CPMA_FIELD("Dth", Deaths),
		CPMA_FIELD("Sui", Suicides),
		CPMA_FIELD("DG", DamageGiven),
		CPMA_FIELD("DR", DamageReceived),
		CPMA_FIELD("Score", Score),
		CPMA_FIELD("TK", TeamKills),
		CPMA_FIELD("TD", TeamDamage),
		CPMA_FIELD("Run", FlagPickups),
		CPMA_FIELD("Held", FlagTime),
		CPMA_FIELD("Cap", Captures),
		CPMA_FIELD("Ast", Assists),
		CPMA_FIELD("Def", Defends),
		CPMA_FIELD("Rtn", Returns),
		CPMA_FIELD("FC", Captures),
		CPMA_FIELD("FR", FlagPickups), // "Flag Runs" seems more probable than "Flag Returns" in CTFS.
		CPMA_FIELD("SG", ShotgunPickups),
		CPMA_FIELD("GL", GrenadeLauncherPickups),
		CPMA_FIELD("RL", RocketLauncherPickups),
		CPMA_FIELD("LG", LightningGunPickups),
		CPMA_FIELD("RG", RailgunPickups),
		CPMA_FIELD("PG", PlasmaGunPickups),
		CPMA_FIELD("MH", MegaHealthPickups),
		CPMA_FIELD("RA", RedArmorPickups),
		CPMA_FIELD("YA", YellowArmorPickups),
		CPMA_FIELD("GA", GreenArmorPickups)
	};
#undef CPMA_FIELD
#undef CPMA_FIELD_CUSTOM
#undef CPMA_PLAYER_FIELD
#undef CPMA_TEAM_FIELD_EX

	idTokenizer* const tokenizer = _plugInTokenizer;
	tokenizer->Tokenize(message.GetPtr());

	const u32 tokenCount = tokenizer->GetArgCount();
	const u32 fieldCount = (u32)UDT_COUNT_OF(fields);
	u32& headerCount = _cpmaPrintStats.HeaderCount;
	headerCount = 0;
	bool previousLeftAligned = false;
	for(u32 i = 0; i < tokenCount; ++i)
	{
		const udtString token = tokenizer->GetArg(i);
		for(u32 j = 0; j < fieldCount; ++j)
		{
			if(udtString::Equals(token, fields[j].Name))
			{
				udtCPMAPrintStats::Header& header = _cpmaPrintStats.Headers[headerCount++];
				header.Field1 = teamStats ? fields[j].TeamField1 : fields[j].PlayerField1;
				header.Field2 = teamStats ? fields[j].TeamField2 : (s16)-666;

				const bool leftAligned = header.Field1 < 0;
				u16 startOffset = 0;
				u16 length = 0;
				if(i > 0)
				{
					startOffset = previousLeftAligned ? 
						(s16)tokenizer->GetArgOffset(i) :
						(u16)(tokenizer->GetArgOffset(i - 1) + tokenizer->GetArgLength(i - 1) + 1);
					const u16 endOffset = (leftAligned && i + 1 < tokenCount) ?
						(u16)tokenizer->GetArgOffset(i + 1) :
						(u16)(tokenizer->GetArgOffset(i) + tokenizer->GetArgLength(i));
					length = endOffset - startOffset;
				}
				else
				{
					length = (u16)tokenizer->GetArgOffset(i + 1);
				}

				header.StringStart = startOffset;
				header.StringLength = length;

				previousLeftAligned = leftAligned;

				break;
			}
		}
	}
}

void udtParserPlugInStats::ParseCPMAPrintStats(const udtString& message)
{
	if(_cpmaPrintStats.TeamStats)
	{
		ParseCPMAPrintStatsTeam(message);
	}
	else
	{
		ParseCPMAPrintStatsPlayer(message);
	}
}

void udtParserPlugInStats::ParseCPMAPrintStatsPlayer(const udtString& message)
{
	const u32 headerCount = _cpmaPrintStats.HeaderCount;
	if(headerCount < 2)
	{
		return;
	}

	udtVMScopedStackAllocator allocatorScope(*TempAllocator);

	udtString playerName;
	for(u32 i = 0; i < headerCount; ++i)
	{
		const udtCPMAPrintStats::Header& header = _cpmaPrintStats.Headers[i];
		if(header.Field1 == -1)
		{
			playerName = udtString::NewSubstringClone(*TempAllocator, message, header.StringStart, header.StringLength);
			udtString::CleanUp(playerName, _protocol);
			udtString::TrimTrailingCharacter(playerName, ' ');
			break;
		}
	}

	s32 clientNumber = -1;
	for(u32 i = 0; i < 64; ++i)
	{
		if(_playerStats[i].CleanName != UDT_U32_MAX &&
		   udtString::Equals(playerName, _stringAllocator.GetStringAt(_playerStats[i].CleanName)))
		{
			clientNumber = (s32)i;
			break;
		}
	}

	if(clientNumber == -1)
	{
		return;
	}

	for(u32 i = 0; i < headerCount; ++i)
	{
		const udtCPMAPrintStats::Header& header = _cpmaPrintStats.Headers[i];
		if(header.Field1 == -2)
		{
			udtString teamName = udtString::NewSubstringClone(*TempAllocator, message, header.StringStart, header.StringLength);
			udtString::CleanUp(playerName, _protocol);
			if(udtString::StartsWith(teamName, "RED"))
			{
				SetPlayerField(clientNumber, udtPlayerStatsField::TeamIndex, (s32)udtTeam::Red);
			}
			else if(udtString::StartsWith(teamName, "BLUE"))
			{
				SetPlayerField(clientNumber, udtPlayerStatsField::TeamIndex, (s32)udtTeam::Blue);
			}
			break;
		}
	}

	idTokenizer* const tokenizer = _plugInTokenizer;
	for(u32 i = 0; i < headerCount; ++i)
	{
		const udtCPMAPrintStats::Header& header = _cpmaPrintStats.Headers[i];
		if(header.Field1 < 0)
		{
			continue;
		}

		s32 value = 0;
		const udtString section = udtString::NewSubstringClone(*TempAllocator, message, header.StringStart, header.StringLength);
		tokenizer->Tokenize(section.GetPtr());
		if(tokenizer->GetArgCount() >= 1)
		{
			if(header.Field1 == udtPlayerStatsField::FlagTime)
			{
				StringParseSeconds(value, tokenizer->GetArgString(0));
			}
			else
			{
				StringParseInt(value, tokenizer->GetArgString(0));
			}
		}
		SetPlayerField(clientNumber, (udtPlayerStatsField::Id)header.Field1, value);
	}
}

void udtParserPlugInStats::ParseCPMAPrintStatsTeam(const udtString& message)
{
	const u32 headerCount = _cpmaPrintStats.HeaderCount;
	if(headerCount < 2)
	{
		return;
	}

	udtVMScopedStackAllocator allocatorScope(*TempAllocator);

	s32 teamIndex = -1;
	for(u32 i = 0; i < headerCount; ++i)
	{
		const udtCPMAPrintStats::Header& header = _cpmaPrintStats.Headers[i];
		if(header.Field1 == -2)
		{
			udtString teamName = udtString::NewSubstringClone(*TempAllocator, message, header.StringStart, header.StringLength);
			udtString::CleanUp(teamName, _protocol);
			if(udtString::StartsWith(teamName, "RED"))
			{
				teamIndex = 0;
			}
			else if(udtString::StartsWith(teamName, "BLUE"))
			{
				teamIndex = 1;
			}
			break;
		}
	}

	if(teamIndex == -1)
	{
		return;
	}

	idTokenizer* const tokenizer = _plugInTokenizer;
	for(u32 i = 0; i < headerCount; ++i)
	{
		const udtCPMAPrintStats::Header& header = _cpmaPrintStats.Headers[i];
		if(header.Field1 < 0)
		{
			continue;
		}

		s32 value = 0;
		const udtString section = udtString::NewSubstringClone(*TempAllocator, message, header.StringStart, header.StringLength);
		tokenizer->Tokenize(section.GetPtr());
		if(tokenizer->GetArgCount() >= 1)
		{
			StringParseInt(value, tokenizer->GetArgString(0));
		}
		SetTeamField(teamIndex, (udtTeamStatsField::Id)header.Field1, value);

		s32 duration = 0;
		if(tokenizer->GetArgCount() >= 2)
		{
			StringParseSeconds(duration, tokenizer->GetArgString(1));
		}
		if(header.Field2 >= 0)
		{
			SetTeamField(teamIndex, (udtTeamStatsField::Id)header.Field2, duration);
		}
	}
}

void udtParserPlugInStats::ResetCPMAPrintStats()
{
	memset(&_cpmaPrintStats, 0, sizeof(_cpmaPrintStats));
}

void udtParserPlugInStats::ParseFields(u8* destMask, s32* destFields, const udtStatsField* fields, s32 fieldCount, s32 tokenOffset)
{
	for(s32 i = 0; i < fieldCount; ++i)
	{
		const s32 fieldIndex = fields[i].Index;
		const s32 byteIndex = fieldIndex >> 3;
		const s32 bitIndex = fieldIndex & 7;
		if(_disableStatsOverrides &&
		   (destMask[byteIndex] & ((u8)1 << (u8)bitIndex)) != 0)
		{
			continue;
		}

		s32* const field = destFields + fieldIndex;
		*field = GetValue(fields[i].TokenIndex + tokenOffset);
		destMask[byteIndex] |= (u8)1 << (u8)bitIndex;
	}
}

void udtParserPlugInStats::SetFields(u8* destMask, s32* destFields, const udtStatsFieldValue* fields, s32 fieldCount)
{
	for(s32 i = 0; i < fieldCount; ++i)
	{
		const s32 fieldIndex = fields[i].Index;
		const s32 byteIndex = fieldIndex >> 3;
		const s32 bitIndex = fieldIndex & 7;
		if(_disableStatsOverrides &&
		   (destMask[byteIndex] & ((u8)1 << (u8)bitIndex)) != 0)
		{
			continue;
		}

		s32* const field = destFields + fieldIndex;
		*field = fields[i].Value;
		destMask[byteIndex] |= (u8)1 << (u8)bitIndex;
	}
}

s32 udtParserPlugInStats::GetValue(s32 index)
{
	s32 result = 0;
	const bool success = StringParseInt(result, _tokenizer->GetArgString((u32)index));

	return success ? result : UDT_S32_MIN;
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
	if(!AreStatsValid())
	{
		return;
	}

	if(_statsArray.GetSize() >= 1 &&
	   _lastMatchEndTime > _analyzer.MatchStartTime())
	{
		return;
	}

	//
	// Fix up the stats and save them.
	//

	const bool forfeited = _cpma150ValidDuelEndScores ? _cpma150Forfeit : _analyzer.Forfeited();
	
	_stats.MatchDurationMs = (u32)(_analyzer.MatchEndTime() - _analyzer.MatchStartTime() - _analyzer.TotalTimeOutDuration());

	if(_stats.GameType == udtGameType::Invalid &&
	   _analyzer.GameType() != udtGameType::Invalid)
	{
		_stats.GameType = _analyzer.GameType();
	}

	if(!forfeited &&
	   !IsRoundBasedMode((udtGameType::Id)_stats.GameType))
	{
		if(_analyzer.OvertimeCount() == 0 &&
		   _analyzer.GetTimeLimit() != 0)
		{
			// Match ended by time limit.
			_stats.MatchDurationMs = _analyzer.GetTimeLimit() * u32(60000);
		}
		else if(_analyzer.OvertimeCount() > 0 &&
				_analyzer.OvertimeType() == udtOvertimeType::Timed)
		{
			// Match ended by time limit in overtime, so round to nearest minute.
			_stats.MatchDurationMs = u32(60000) * ((_stats.MatchDurationMs + u32(59999)) / u32(60000));
		}
	}

	for(s32 i = 0; i < 64; ++i)
	{
		// Fix the weapon index.
		s32& bestWeapon = GetPlayerFields(i)[udtPlayerStatsField::BestWeapon];
		u32 udtWeaponId;
		if(GetUDTNumber(udtWeaponId, udtMagicNumberType::Weapon, bestWeapon, _protocol))
		{
			bestWeapon = (s32)udtWeaponId;
		}
		else
		{
			bestWeapon = (s32)udtWeapon::Gauntlet;
		}
		
		// Fix the team index when needed and possible.
		if(!IsBitSet(GetPlayerFlags(i), (u32)udtPlayerStatsField::TeamIndex) &&
		   _playerTeamIndices[i] != -1)
		{
			SetPlayerField(i, udtPlayerStatsField::TeamIndex, _playerTeamIndices[i]);
		}
		
		ComputePlayerAccuracies(i);
		ComputePlayerRocketSkill(i);
	}

	// Make sure damage given and damage received are defined for both players in a duel.
	if((udtGameType::Id)_stats.GameType == udtGameType::Duel)
	{
		s32 playerIndices[2] = { -1, -1 };
		s32 playersFound = 0;
		for(s32 i = 0; i < 64; ++i)
		{
			if(IsBitSet(GetPlayerFlags(i), (u32)udtPlayerStatsField::TeamIndex) &&
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

	if(_cpma150ValidDuelEndScores)
	{
		// only keep client numbers 0 and 1
		memset(&_playerFlags[2][0], 0, 62 * sizeof(_playerFlags[0]));
		memset(&_playerFields[2][0], 0, 62 * sizeof(_playerFields[0]));
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

	const u32 timeOutCount = _analyzer.TimeOutCount();
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

	_stats.FirstTeamFlagIndex = _teamFlagsArray.GetSize();
	_stats.FirstPlayerFlagIndex = _playerFlagsArray.GetSize();
	_stats.FirstTeamFieldIndex = _teamFieldsArray.GetSize();
	_stats.FirstPlayerFieldIndex = _playerFieldsArray.GetSize();
	_stats.FirstPlayerStatsIndex = _playerStatsArray.GetSize();
	_stats.FirstTimeOutRangeIndex = _timeOutTimes.GetSize() / 2;

	u8* teamFlags = teamCount == 0 ? NULL : _teamFlagsArray.ExtendAndMemset(teamCount * UDT_TEAM_STATS_MASK_BYTE_COUNT, 0);
	u8* playerFlags = playerCount == 0 ? NULL : _playerFlagsArray.ExtendAndMemset(playerCount * UDT_PLAYER_STATS_MASK_BYTE_COUNT, 0);
	s32* teamFields = teamFieldCount == 0 ? NULL : _teamFieldsArray.ExtendAndMemset(teamFieldCount, 0);
	s32* playerFields = playerFieldCount == 0 ? NULL : _playerFieldsArray.ExtendAndMemset(playerFieldCount, 0);
	udtPlayerStats* playerStats = playerCount == 0 ? NULL : _playerStatsArray.ExtendAndMemset(playerCount, 0);
	s32* const timeOutTimes = timeOutCount == 0 ? NULL : _timeOutTimes.ExtendAndMemset(timeOutCount * 2, 0);

	for(s32 i = 0; i < 2; ++i)
	{
		if((_stats.ValidTeams & ((u64)1 << (u64)i)) == 0)
		{
			continue;
		}

		memcpy(teamFlags, GetTeamFlags(i), UDT_TEAM_STATS_MASK_BYTE_COUNT);

		for(s32 j = 0; j < (s32)udtTeamStatsField::Count; ++j)
		{
			if(IsBitSet(teamFlags, (u32)j))
			{
				*teamFields++ = GetTeamFields(i)[j];
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
			if(IsBitSet(playerFlags, (u32)j))
			{
				*playerFields++ = GetPlayerFields(i)[j];
			}
		}

		playerFlags += UDT_PLAYER_STATS_MASK_BYTE_COUNT;
		*playerStats++ = _playerStats[i];
	}

	for(u32 i = 0; i < timeOutCount; ++i)
	{
		timeOutTimes[2 * i + 0] = _analyzer.GetTimeOutStartTime(i);
		timeOutTimes[2 * i + 1] = _analyzer.GetTimeOutEndTime(i);
	}

	if(IsTeamMode((udtGameType::Id)_stats.GameType))
	{
		s32 redScore = 0;
		s32 blueScore = 0;
		if(forfeited &&
		   _protocol >= udtProtocol::Dm73)
		{
			// Short-circuit the scores commands in case of a forfeit so we don't get the -999 scores.
			redScore = _firstPlaceScore;
			blueScore = _secondPlaceScore;
			_stats.FirstPlaceScore = udt_max(redScore, blueScore);
			_stats.SecondPlaceScore = udt_min(redScore, blueScore);
			WriteStringToApiStruct(_stats.FirstPlaceName, redScore > blueScore ? _redString : _blueString);
			WriteStringToApiStruct(_stats.SecondPlaceName, redScore > blueScore ? _blueString : _redString);
		}
		else if((_stats.ValidTeams & 3) == 3 &&
				IsBitSet(GetTeamFlags(0), (u32)udtTeamStatsField::Score) &&
				IsBitSet(GetTeamFlags(1), (u32)udtTeamStatsField::Score))
		{
			redScore = GetTeamFields(0)[udtTeamStatsField::Score];
			blueScore = GetTeamFields(1)[udtTeamStatsField::Score];
			_stats.FirstPlaceScore = udt_max(redScore, blueScore);
			_stats.SecondPlaceScore = udt_min(redScore, blueScore);
			WriteStringToApiStruct(_stats.FirstPlaceName, redScore > blueScore ? _redString : _blueString);
			WriteStringToApiStruct(_stats.SecondPlaceName, redScore > blueScore ? _blueString : _redString);
		}
		else if(_analyzer.Mod() == udtMod::CPMA)
		{
			const bool roundBased = IsRoundBasedMode(_analyzer.GameType());
			redScore = roundBased ? _cpmaScoreRed : _cpmaRoundScoreRed;
			blueScore = roundBased ? _cpmaScoreBlue : _cpmaRoundScoreBlue;
			_stats.FirstPlaceScore = udt_max(redScore, blueScore);
			_stats.SecondPlaceScore = udt_min(redScore, blueScore);
			WriteStringToApiStruct(_stats.FirstPlaceName, redScore > blueScore ? _redString : _blueString);
			WriteStringToApiStruct(_stats.SecondPlaceName, redScore > blueScore ? _blueString : _redString);
		}
		else
		{
			redScore = _firstPlaceScore;
			blueScore = _secondPlaceScore;
			_stats.FirstPlaceScore = udt_max(redScore, blueScore);
			_stats.SecondPlaceScore = udt_min(redScore, blueScore);
			WriteStringToApiStruct(_stats.FirstPlaceName, redScore > blueScore ? _redString : _blueString);
			WriteStringToApiStruct(_stats.SecondPlaceName, redScore > blueScore ? _blueString : _redString);
		}
	}
	else
	{
		s32 firstPlaceScore = UDT_S32_MIN;
		s32 secondPlaceScore = UDT_S32_MIN;
		s32 firstPlaceIndex = -1;
		s32 secondPlaceIndex = -1;
		for(s32 i = 0; i < 64; ++i)
		{
			if((_stats.ValidPlayers & ((u64)1 << (u64)i)) != 0 &&
			   IsBitSet(GetPlayerFlags(i), (u32)udtPlayerStatsField::Score))
			{
				if(IsBitSet(GetPlayerFlags(i), (u32)udtPlayerStatsField::TeamIndex) && 
				   GetPlayerFields(i)[udtPlayerStatsField::TeamIndex] != (s32)udtTeam::Spectators)
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

		if(_analyzer.Mod() == udtMod::CPMA &&
		   forfeited &&
		   !_cpma150Forfeit &&
		   _firstPlaceClientNumber >= 0 &&
		   _firstPlaceClientNumber < 64 &&
		   _secondPlaceClientNumber >= 0 &&
		   _secondPlaceClientNumber < 64)
		{
			_stats.FirstPlaceScore = udt_max(_cpmaRoundScoreRed, _cpmaRoundScoreBlue);
			_stats.SecondPlaceScore = udt_min(_cpmaRoundScoreRed, _cpmaRoundScoreBlue);
			_stats.FirstPlaceName = _playerStats[_firstPlaceClientNumber].CleanName;
			_stats.FirstPlaceNameLength = _playerStats[_firstPlaceClientNumber].CleanNameLength;
			_stats.SecondPlaceName = _playerStats[_secondPlaceClientNumber].CleanName;
			_stats.SecondPlaceNameLength = _playerStats[_secondPlaceClientNumber].CleanNameLength;
		}
		else if(_analyzer.Mod() == udtMod::CPMA &&
				_cpma150ValidDuelEndScores &&
				IsBitSet(GetPlayerFlags(0), (u32)udtPlayerStatsField::Score) &&
				IsBitSet(GetPlayerFlags(1), (u32)udtPlayerStatsField::Score))
		{
			_stats.FirstPlaceScore = GetPlayerFields(0)[udtPlayerStatsField::Score];
			_stats.SecondPlaceScore = GetPlayerFields(1)[udtPlayerStatsField::Score];
			_stats.FirstPlaceName = _playerStats[0].CleanName;
			_stats.FirstPlaceNameLength = _playerStats[0].CleanNameLength;
			_stats.SecondPlaceName = _playerStats[1].CleanName;
			_stats.SecondPlaceNameLength = _playerStats[1].CleanNameLength;
		}
		else if(firstPlaceScore != UDT_S32_MIN &&
				secondPlaceScore != UDT_S32_MIN &&
				firstPlaceIndex != -1 &&
				secondPlaceIndex != -1)
		{
			_stats.FirstPlaceScore = firstPlaceScore;
			_stats.SecondPlaceScore = secondPlaceScore;
			_stats.FirstPlaceName = _playerStats[firstPlaceIndex].CleanName;
			_stats.FirstPlaceNameLength = _playerStats[firstPlaceIndex].CleanNameLength;
			_stats.SecondPlaceName = _playerStats[secondPlaceIndex].CleanName;
			_stats.SecondPlaceNameLength = _playerStats[secondPlaceIndex].CleanNameLength;
		}
		else if(_analyzer.Mod() != udtMod::CPMA &&
				_firstPlaceClientNumber >= 0 &&
				_firstPlaceClientNumber < 64 &&
				_secondPlaceClientNumber >= 0 &&
				_secondPlaceClientNumber < 64)
		{
			_stats.FirstPlaceScore = _firstPlaceScore;
			_stats.SecondPlaceScore = _secondPlaceScore;
			_stats.FirstPlaceName = _playerStats[_firstPlaceClientNumber].CleanName;
			_stats.FirstPlaceNameLength = _playerStats[_firstPlaceClientNumber].CleanNameLength;
			_stats.SecondPlaceName = _playerStats[_secondPlaceClientNumber].CleanName;
			_stats.SecondPlaceNameLength = _playerStats[_secondPlaceClientNumber].CleanNameLength;
		}
		else if(_analyzer.Mod() == udtMod::CPMA &&
				_firstPlaceClientNumber >= 0 &&
				_firstPlaceClientNumber < 64 &&
				_secondPlaceClientNumber >= 0 &&
				_secondPlaceClientNumber < 64)
		{
			_stats.FirstPlaceScore = udt_max(_cpmaScoreRed, _cpmaScoreBlue);
			_stats.SecondPlaceScore = udt_min(_cpmaScoreRed, _cpmaScoreBlue);
			_stats.FirstPlaceName = _playerStats[_firstPlaceClientNumber].CleanName;
			_stats.FirstPlaceNameLength = _playerStats[_firstPlaceClientNumber].CleanNameLength;
			_stats.SecondPlaceName = _playerStats[_secondPlaceClientNumber].CleanName;
			_stats.SecondPlaceNameLength = _playerStats[_secondPlaceClientNumber].CleanNameLength;
		}
		else
		{
			_stats.FirstPlaceScore = UDT_S32_MIN;
			_stats.SecondPlaceScore = UDT_S32_MIN;
			_stats.FirstPlaceName = UDT_U32_MAX;
			_stats.FirstPlaceNameLength = 0;
			_stats.SecondPlaceName = UDT_U32_MAX;
			_stats.SecondPlaceNameLength = 0;
		}
	}

	u32 scoreLimit = _analyzer.GetScoreLimit();
	u32 fragLimit = _analyzer.GetFragLimit();
	u32 captureLimit = _analyzer.GetCaptureLimit();
	u32 roundLimit = _analyzer.GetRoundLimit();

	const u8* gameTypeFlags = NULL;
	u32 gameTypeCount = 0;
	if(udtGetByteArray(udtByteArray::GameTypeFlags, &gameTypeFlags, &gameTypeCount) == udtErrorCode::None &&
	   (u32)_stats.GameType < gameTypeCount)
	{
		const u8 flags = gameTypeFlags[_stats.GameType];
		if((flags & (u8)udtGameTypeMask::HasScoreLimit) == 0)
		{
			scoreLimit = 0;
		}
		if((flags & (u8)udtGameTypeMask::HasFragLimit) == 0)
		{
			fragLimit = 0;
		}
		if((flags & (u8)udtGameTypeMask::HasCaptureLimit) == 0)
		{
			captureLimit = 0;
		}
		if((flags & (u8)udtGameTypeMask::HasRoundLimit) == 0)
		{
			roundLimit = 0;
		}
	}

	const s32 countDownStartTime = _analyzer.CountDownStartTime();
	const s32 intermissionEndTime = _analyzer.IntermissionEndTime();

	_stats.StartDateEpoch = _analyzer.GetMatchStartDateEpoch();
	_stats.GamePlay = (u32)_analyzer.GamePlay();
	WriteStringToApiStruct(_stats.MapName, udtString::NewCloneFromRef(_stringAllocator, _analyzer.MapName()));
	_stats.OverTimeCount = _analyzer.OvertimeCount();
	_stats.OverTimeType = (u32)_analyzer.OvertimeType();
	_stats.Mod = _analyzer.Mod();
	WriteStringToApiStruct(_stats.ModVersion, udtString::NewCloneFromRef(_stringAllocator, _analyzer.ModVersion()));
	_stats.Forfeited = forfeited ? 1 : 0;
	_stats.TimeOutCount = timeOutCount;
	_stats.TotalTimeOutDurationMs = _analyzer.TotalTimeOutDuration();
	_stats.MercyLimited = _analyzer.MercyLimited();
	_stats.TeamMode = IsTeamMode((udtGameType::Id)_stats.GameType) ? 1 : 0;
	_stats.TimeLimit = _analyzer.GetTimeLimit();
	_stats.ScoreLimit = scoreLimit;
	_stats.FragLimit = fragLimit;
	_stats.CaptureLimit = captureLimit;
	_stats.RoundLimit = roundLimit;
	_stats.StartTimeMs = _analyzer.MatchStartTime();
	_stats.EndTimeMs = _analyzer.MatchEndTime();
	_stats.GameStateIndex = (u32)_analyzer.GameStateIndex();
	_stats.CountDownStartTimeMs = countDownStartTime != UDT_S32_MIN ? countDownStartTime : _stats.StartTimeMs;
	_stats.IntermissionEndTimeMs = intermissionEndTime != UDT_S32_MIN ? intermissionEndTime : _stats.EndTimeMs;
	_statsArray.Add(_stats);

	_lastMatchEndTime = _analyzer.MatchEndTime();

	// Clear the stats for the next match.
	ClearStats();

	// We just cloned its strings into our local _stringAllocator, so it's safe.
	_analyzer.ClearStringAllocator();
}

void udtParserPlugInStats::ClearStats(bool newGameState)
{
	memset(_playerFields, 0, sizeof(_playerFields));
	memset(_playerFlags, 0, sizeof(_playerFlags));
	memset(_teamFields, 0, sizeof(_teamFields));
	memset(_teamFlags, 0, sizeof(_teamFlags));
	ResetCPMAPrintStats();
	_cpma150DuelEndScoresState = 0;
	_cpma150ValidDuelEndScores = false;
	_cpma150Forfeit = false;

	if(newGameState)
	{
		memset(&_stats, 0, sizeof(_stats));
		memset(_playerStats, 0, sizeof(_playerStats));
		_stats.GameType = (u32)udtGameType::Invalid;
		_stats.CustomBlueName = UDT_U32_MAX;
		_stats.CustomRedName = UDT_U32_MAX;
		_stats.FirstPlaceName = UDT_U32_MAX;
		_stats.MapName = UDT_U32_MAX;
		_stats.ModVersion = UDT_U32_MAX;
		_stats.SecondPlaceName = UDT_U32_MAX;
		for(s32 i = 0; i < 64; ++i)
		{
			_playerTeamIndices[i] = -1;
		}
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
	if(IsBitSet(flags, (u32)acc) ||
	   !IsBitSet(flags, (u32)hits) ||
	   !IsBitSet(flags, (u32)shots))
	{
		return;
	}

	s32* const fields = GetPlayerFields(clientNumber);
	const s32 hitCount = fields[hits];
	const s32 shotCount = fields[shots];
	fields[acc] = shotCount == 0 ? 0 : (((100 * hitCount) + (shotCount / 2)) / shotCount);
	SetBit(flags, acc);
}

void udtParserPlugInStats::ComputePlayerRocketSkill(s32 clientNumber)
{
	const u32 skillIdx = (u32)udtPlayerStatsField::RocketLauncherSkill;
	const u32 dmgIdx = (u32)udtPlayerStatsField::RocketLauncherDamage;
	const u32 hitsIdx = (u32)udtPlayerStatsField::RocketLauncherHits;

	u8* const flags = GetPlayerFlags(clientNumber);
	if(IsBitSet(flags, skillIdx) ||
	   !IsBitSet(flags, dmgIdx) ||
	   !IsBitSet(flags, hitsIdx))
	{
		return;
	}
	
	s32* const fields = GetPlayerFields(clientNumber);
	const s32 damage = fields[dmgIdx];
	const s32 hits = fields[hitsIdx];
	fields[skillIdx] = hits == 0 ? 0 : ((damage + (hits / 2)) / hits);
	SetBit(flags, skillIdx);
}
