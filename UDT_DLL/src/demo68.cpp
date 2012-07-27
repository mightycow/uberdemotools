#include "demo68.hpp"


// Offset from the start of the structure == absolute address when the struct is at address 0.
#define ESF(x) #x, (int)&((entityState_68_t*)0)->x

const netField_t Demo68::EntityStateFields[] =
{
	{ ESF(pos.trTime), 32 },
	{ ESF(pos.trBase[0]), 0 },
	{ ESF(pos.trBase[1]), 0 },
	{ ESF(pos.trDelta[0]), 0 },
	{ ESF(pos.trDelta[1]), 0 },
	{ ESF(pos.trBase[2]), 0 },
	{ ESF(apos.trBase[1]), 0 },
	{ ESF(pos.trDelta[2]), 0 },
	{ ESF(apos.trBase[0]), 0 },
	{ ESF(event), 10 },
	{ ESF(angles2[1]), 0 },
	{ ESF(eType), 8 },
	{ ESF(torsoAnim), 8 },
	{ ESF(eventParm), 8 },
	{ ESF(legsAnim), 8 },
	{ ESF(groundEntityNum), GENTITYNUM_BITS },
	{ ESF(pos.trType), 8 },
	{ ESF(eFlags), 19 },
	{ ESF(otherEntityNum), GENTITYNUM_BITS },
	{ ESF(weapon), 8 },
	{ ESF(clientNum), 8 },
	{ ESF(angles[1]), 0 },
	{ ESF(pos.trDuration), 32 },
	{ ESF(apos.trType), 8 },
	{ ESF(origin[0]), 0 },
	{ ESF(origin[1]), 0 },
	{ ESF(origin[2]), 0 },
	{ ESF(solid), 24 },
	{ ESF(powerups), MAX_POWERUPS },
	{ ESF(modelindex), 8 },
	{ ESF(otherEntityNum2), GENTITYNUM_BITS },
	{ ESF(loopSound), 8 },
	{ ESF(generic1), 8 },
	{ ESF(origin2[2]), 0 },
	{ ESF(origin2[0]), 0 },
	{ ESF(origin2[1]), 0 },
	{ ESF(modelindex2), 8 },
	{ ESF(angles[0]), 0 },
	{ ESF(time), 32 },
	{ ESF(apos.trTime), 32 },
	{ ESF(apos.trDuration), 32 },
	{ ESF(apos.trBase[2]), 0 },
	{ ESF(apos.trDelta[0]), 0 },
	{ ESF(apos.trDelta[1]), 0 },
	{ ESF(apos.trDelta[2]), 0 },
	{ ESF(time2), 32 },
	{ ESF(angles[2]), 0 },
	{ ESF(angles2[0]), 0 },
	{ ESF(angles2[2]), 0 },
	{ ESF(constantLight), 32 },
	{ ESF(frame), 16 }
};

#undef ESF

const int Demo68::EntityStateFieldCount = sizeof(Demo68::EntityStateFields) / sizeof(Demo68::EntityStateFields[0]);


Demo68::Demo68() : Demo()
{
	_inTw = -1;
	_inTs = 0;
	_inSb = -9999;
	_inSr = -9999;
	_inTl = 0;
	_writeStats = false;
}

Demo68::~Demo68()
{
}

void Demo68::ProtocolInit()
{
	_inParseEntities.resize(MAX_PARSE_ENTITIES);
	_inEntityBaselines.resize(MAX_PARSE_ENTITIES);
	memset(&_inEntityBaselines[0], 0, MAX_PARSE_ENTITIES * sizeof(entityState_68_t));
}

void Demo68::ProtocolParseBaseline(msg_t* msg, msg_t* msgOut)
{
	ParseBaselineT<Demo68, entityState_68_t>(msg, msgOut);
}

void Demo68::ProtocolParsePacketEntities(msg_t* msg, msg_t* msgOut, clSnapshot_t* oldframe, clSnapshot_t* newframe)
{
	ParsePacketEntitiesT<Demo68, entityState_68_t>(msg, msgOut, oldframe, newframe);

	//
	// Stats.
	//
	//const int tw = _inTw;
	//const int ts = _inTs;
	if(_writeStats)
	{
		const int tl = _inTl * 60 * 1000;
		const int endTime = _gameStartTime + _serverTimeOffset + tl;
		if(GetVirtualInputTime() >= endTime)
		{
			_writeStats = false;
			if(_matchStats.size() > 0)
			{
				_matchStats[_matchStats.size() - 1].EndTime = newframe->serverTime + _serverTimeOffset;
				LogInfo("Current match ends: %d", _matchStats[_matchStats.size() - 1].EndTime);
				LogInfo("%d - %d", GetVirtualInputTime(), endTime);

				const MatchStats& match = _matchStats[_matchStats.size() - 1];
				for(int i = 0; i < MAX_CLIENTS; ++i)
				{
					const PlayerStats& player = match.Players[i];
					for(int j = 0; j < MAX_WEAPONS; ++j)
					{
						if(player.WeaponAtts[j] > 0)
						{
							LogInfo("Player %d: %d atts with weapon %d", i, player.WeaponAtts[j], j);
						}
					}
				}
			}
		}
	}

	if(!_writeStats || _matchStats.size() == 0)
	{
		return;
	}

	MatchStats& stats = _matchStats[_matchStats.size() - 1];
	for(int i = 0; i < newframe->numEntities; ++i)
	{
		const entityState_68_t* entity = &_inParseEntities[(newframe->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES-1)];
		const int event = entity->event & (~EV_EVENT_BITS);
		PlayerStats& player = stats.Players[entity->clientNum & (MAX_CLIENTS - 1)];
		if(event == ET_EVENTS + EV_FIRE_WEAPON)
		{
			++player.WeaponAtts[entity->weapon];
		}
	}
}

void Demo68::ProtocolEmitPacketEntities(clSnapshot_t* from, clSnapshot_t* to)
{
	EmitPacketEntitiesT<Demo68, entityState_68_t>(from, to);
}

void Demo68::ProtocolAnalyzeConfigString(int csIndex, const std::string& input)
{
	if(csIndex == CS_CPMA_GAME_INFO)
	{
		AnalyzeConfigStringCpmaGameInfo(input);
	}
	else if(csIndex == CS_CPMA_ROUND_INFO)
	{
		AnalyzeConfigStringCpmaRoundInfo(input);
	}
	else if(csIndex >= CS_PLAYERS_68 && csIndex < CS_PLAYERS_68 + MAX_CLIENTS)
	{
		AnalyzePlayerInfo(csIndex - CS_PLAYERS_68, input);
	}
	else if(csIndex == CS_SCORES1)
	{
		ReadScore(input.c_str(), &_score1);
	}
	else if(csIndex == CS_SCORES2)
	{
		ReadScore(input.c_str(), &_score2);
	}
	else if(csIndex == CS_SERVERINFO)
	{
		int gameType = -1;
		if(GetVariable(input, "g_gametype", &gameType))
		{
			_gameType = gameType;
		}
	}
}

void Demo68::ProtocolFixConfigString(int csIndex, const std::string& input, std::string& output)
{
	output = input;

	if(csIndex == CS_CPMA_GAME_INFO || csIndex == CS_CPMA_ROUND_INFO)
	{
		ChangeVariable(output, "sb", _inSb, output);
		ChangeVariable(output, "sr", _inSr, output);
	}

	if(csIndex != CS_CPMA_GAME_INFO)
	{
		return;
	}

	const int tw = _inTw;
	const int ts = _inTs;

	// @NOTE: Some calls down there might seems redundant...
	// They're not. We may be re-parsing the game state.
	// So they're needed for correct output.

	// Are we in warm-up mode?
	if(tw == -1 && ts == 0)
	{
		ChangeVariable(output, "tw", "-1", output);
		ChangeVariable(output, "ts", "0", output);
	}
	// Are we in the middle of a countdown?
	else if(tw > 0 && ts > 0)
	{
		const int newTw = GetFixedTwTs(tw); // The time the game is supposed to start.
		const int newTs = GetFixedTwTs(ts); // The time the demo started.
		ChangeVariable(output, "tw", newTw, output);
		ChangeVariable(output, "ts", newTs, output);
	}
	// Are we playing a match?
	else if(tw == 0 && ts > 0)
	{
		const int newTs = GetFixedTwTs(ts); // First snapshot time after the game started.
		ChangeVariable(output, "tw", "0", output);
		ChangeVariable(output, "ts", newTs, output);
	}
	// No idea what's going but apply the offsets anyway.
	else
	{
		const int newTw = GetFixedTwTs(tw);
		const int newTs = GetFixedTwTs(ts);
		ChangeVariable(output, "tw", newTw, output);
		ChangeVariable(output, "ts", newTs, output);
	}
}

void Demo68::ProtocolAnalyzeSnapshot(const clSnapshot_t* oldSnap, const clSnapshot_t* newSnap)
{
	AnalyzeSnapshotT<Demo68, entityState_68_t>(oldSnap, newSnap);
}

void Demo68::AnalyzeConfigStringCpmaGameInfo(const std::string& input)
{
	// Are we re-parsing the game state?
	if(_writeFirstBlock)
	{
		// If so, we ignore the tw, ts, sb and sr values.
		return;
	}

	const int oldTw = _inTw;
	const int oldTs = _inTs;

	const int tw = GetCpmaConfigStringInt("tw", input.c_str());
	const int ts = GetCpmaConfigStringInt("ts", input.c_str());
	_inTw = tw;
	_inTs = ts;
	if(tw == 0 && ts > 0)
	{
		// First snapshot time after the game started.
		_gameStartTime = ts;
	}
	else if(tw > 0)
	{
		// The time the game is supposed to start.
		// Generally lags 1 update behind the actual time (approx. 33ms).
		_gameStartTime = tw; 
	}

	_inSb = GetCpmaConfigStringInt("sb", input.c_str());
	_inSr = GetCpmaConfigStringInt("sr", input.c_str());

	//
	// Stats.
	//
	_inTl = GetCpmaConfigStringInt("tl", input.c_str());
	if((tw == 0 && ts > 0) && !(oldTw == 0 && oldTs > 0))
	{
		// The MatchStats struct is too big to be allocated on the stack.
		MatchStats* stats = (MatchStats*)malloc(sizeof(MatchStats));
		if(stats != NULL)
		{
			memset(stats, 0, sizeof(MatchStats));
			stats->StartTime = _gameStartTime + _serverTimeOffset;
			_matchStats.push_back(*stats);
			_writeStats = true;
			LogInfo("New match starts: %d", stats->StartTime);
			free(stats);
		}
	}
}

void Demo68::AnalyzeConfigStringCpmaRoundInfo(const std::string& input)
{
	// Are we re-parsing the game state?
	if(_writeFirstBlock)
	{
		// If so, we ignore the sb and sr values.
		return;
	}

	_inSb = GetCpmaConfigStringInt("sb", input.c_str());
	_inSr = GetCpmaConfigStringInt("sr", input.c_str());
}

void Demo68::AnalyzePlayerInfo(int clientNum, const std::string& configString)
{
	if(clientNum < 0 || clientNum > MAX_CLIENTS)
	{
		return;
	}

	PlayerInfoPers* const player = &_players[clientNum];
	if(configString.find("\"\"") != std::string::npos || configString == "")
	{
		EventInfo info;
		info.Time = _serverTime;
		info.Event = std::string(_players[clientNum].Name) + " disconnected";
		_eventPlaybackInfos.push_back(info);

		Q_strncpyz(player->Name, "<empty_slot>", sizeof(player->Name));
		player->Valid = false;

		return;
	}

	const int previousTeam = player->Info.Team;

	std::string name;
	ExtractPlayerNameFromConfigString(name, configString);

	player->Valid = true;
	Q_strncpyz(player->Name, name.c_str(), sizeof(player->Name));
	TryGetVariable(&player->Info.Handicap, configString, "hc");
	TryGetVariable(&player->Info.Team, configString, "t");
	TryGetVariable(&player->Info.BotSkill, configString, "l"); // @TODO: Correct?

	PlayerNameInfo nameInfo;
	nameInfo.Time = _serverTime;
	Q_strncpyz(nameInfo.Name, name.c_str(), sizeof(nameInfo.Name));
	nameInfo.Clan[0] = '\0';
	nameInfo.Country[0] = '\0';
	_playerNamesPlaybackInfos[clientNum].push_back(nameInfo);

	if(player->Info.Team != previousTeam)
	{
		std::string teamName;
		GetTeamName(teamName, player->Info.Team);

		EventInfo info;
		info.Time = _serverTime;
		info.Event = std::string(player->Name) + " moved to " + teamName;
		_eventPlaybackInfos.push_back(info);
	}
}

void Demo68::ProtocolAnalyzeAndFixCommandString(const char* command, std::string& output)
{
	int csIndex = -1;
	std::string configString;
	if(ExtractConfigStringFromCommand(csIndex, configString, command))
	{
		_inConfigStrings[csIndex] = configString;

		// Warm-up. Scores.
		if(csIndex == CS_CPMA_GAME_INFO)
		{
			ProtocolAnalyzeConfigString(csIndex, command);
			ProtocolFixConfigString(csIndex, command, output);
		}
		// Scores.
		else if(csIndex == CS_CPMA_ROUND_INFO)
		{
			ProtocolAnalyzeConfigString(csIndex, command);
			output = command;
		}
		// Players.
		if(csIndex >= CS_PLAYERS_68 && csIndex < CS_PLAYERS_68 + MAX_CLIENTS)
		{
			ProtocolAnalyzeConfigString(csIndex, command);
			output = command;
		}
		// Scores.
		else if(csIndex == CS_SCORES1)
		{
			ReadScore(configString.c_str(), &_score1);
			output = command;
		}
		// Scores.
		else if(csIndex == CS_SCORES2)
		{
			ReadScore(configString.c_str(), &_score2);
			output = command;
		}
		else if(csIndex == CS_SERVERINFO)
		{
			ProtocolAnalyzeConfigString(csIndex, configString);
			output = command;
		}
	}
	// Chat messages.
	else if(strstr(command, "chat") == command)
	{
		_tokenizer.Tokenize(command);
		if(_tokenizer.argc() >= 2 && strcmp(_tokenizer.argv(0), "chat") == 0)
		{
			const int currentTime = GetVirtualInputTime();
			const char* const chatMsg = _tokenizer.argv(1);
			_chatMessages[currentTime] = chatMsg;
		}
		output = command;
	}
	else
	{
		output = command;
	}
}

void Demo68::ProtocolGetScores(int& score1, int& score2)
{
	score1 = _inSr;
	score2 = _inSb;
}