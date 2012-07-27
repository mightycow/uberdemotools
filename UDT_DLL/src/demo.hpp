#pragma once


#include "message.hpp"
#include "common.hpp"
#include "tokenizer.hpp"
#include "utils.hpp"
#include "math.hpp"
#include "api.h"

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <cmath>


struct DemoCut
{
	std::string FilePath;
	int StartTimeMs;
	int EndTimeMs;
};


static std::string GetTeamName(int team)
{
	switch(team)
	{
	case TEAM_FREE:
		return "game";
	case TEAM_RED:
		return "red Team";
	case TEAM_BLUE:
		return "blue Team";
	case TEAM_SPECTATOR:
		return "spectators";
	}

	return "unknown";
}

struct Demo
{
public:
	Demo();
	~Demo();

	bool Do();

	

protected:
	void NewGamestate();
	void CloseFiles();
	void DemoCompleted();
	void FinishParsing();
	void ParseServerMessage(msg_t* msg, msg_t* msgOut);
	void ParseCommandString(msg_t* msg, msg_t* msgOut);
	void ParseGamestate(msg_t* msg, msg_t* msgOut);
	void ParseSnapshot(msg_t* msg, msg_t* msgOut);
	void ParseDownload(msg_t* msg, msg_t* msgOut);
	void WriteFileBlock(msg_t* msgOut);
	void WriteFileStart();
	void WriteFileEnd();
	void SetServerTime(int serverTime);
	void SetGameTime(int serverTime);
	void AnalyzePlayerState(playerState_t* oldState, playerState_t* newState);
	void FixPlayerState(playerState_t* oldState, playerState_t* newState);
	void FixPlayerStatePowerUp(playerState_t* oldState, playerState_t* newState, int powerUpIdx);
	void FixLastGameStateLastSnapshotTime();
	void FixLastGameStateFirstSnapshotTime();
	void FixLastGameStateServerTimeOffset();
	void ExtractPlayerNameFromConfigString(std::string& playerName, const std::string& configString);
	bool ExtractConfigStringFromCommand(int& csIndex, std::string& configString, const char* command);
	bool ShouldWriteDemoMessages() const;
	int  GetRealInputTime();
	int  GetVirtualInputTime();
	int  GetFixedOutputTime(int time);
	int  GetFixedTwTs(int time);

	virtual void ProtocolInit() = 0;
	virtual void ProtocolParseBaseline(msg_t* msg, msg_t* msgOut) = 0;
	virtual void ProtocolParsePacketEntities(msg_t* msg, msg_t* msgOut, clSnapshot_t* oldframe, clSnapshot_t* newframe) = 0;
	virtual void ProtocolEmitPacketEntities(clSnapshot_t* from, clSnapshot_t* to) = 0;
	virtual void ProtocolAnalyzeConfigString(int csIndex, const std::string& input) = 0;
	virtual void ProtocolFixConfigString(int csIndex, const std::string& input, std::string& output) = 0;
	virtual void ProtocolAnalyzeAndFixCommandString(const char* command, std::string& output) = 0;
	virtual void ProtocolAnalyzeSnapshot(const clSnapshot_t* oldSnap, const clSnapshot_t* newSnap) = 0;
	virtual void ProtocolGetScores(int& score1, int& score2) = 0;

	template<class DemoT, typename EntityStateT>
	void ParseBaselineT(msg_t* msg, msg_t* msgOut);

	template<class DemoT, typename EntityStateT>
	void ParsePacketEntitiesT(msg_t* msg, msg_t* msgOut, clSnapshot_t* oldframe, clSnapshot_t* newframe);

	template<class DemoT, typename EntityStateT>
	void EmitPacketEntitiesT(clSnapshot_t* from, clSnapshot_t* to);

	template<class DemoT, typename EntityStateT>
	void DeltaEntityT(msg_t* msg, clSnapshot_t *frame, int newnum, EntityStateT* old, qbool unchanged);

	template<class DemoT, typename EntityStateT>
	void AnalyzeEntityT(EntityStateT* oldState, EntityStateT* newState, int newStateIndex);

	template<class DemoT, typename EntityStateT>
	void AnalyzeEntityDeathT(EntityStateT* oldState, EntityStateT* newState, int newStateIndex);

	template<class DemoT, typename EntityStateT>
	void AnalyzeEntityObituaryT(EntityStateT* oldState, EntityStateT* newState, int newStateIndex);

	template<class DemoT, typename EntityStateT>
	void AnalyzeSnapshotT(const clSnapshot_t* oldSnap, const clSnapshot_t* newSnap);

public:
	struct GameStateInfo 
	{ 
		int FileOffset; 
		int ServerTimeOffset; 
		int FirstSnapshotTime;
		int LastSnapshotTime;
	};

	struct ObituaryInfo
	{
		int VirtualServerTime;
		int MeanOfDeath; // meansOfDeath_t
		std::string AttackerName;
		std::string TargetName;
	};

	struct PuRunInfo
	{
		int VirtualServerTime;
		int Pu; // powerup_t
		int Duration; // [ms]
		int Kills;
		int TeamKills;
		int SelfKill; // Non-zero if the player kills himself with the PU :-)
		int Ended;
		int PredictedEndTime;
		int Player;
		std::string PlayerName;
	};

	struct EntityEventInfo
	{
		int PreviousRelAck;
		int PreviousEvent;
		int PreviousEventType;
	};

	struct EntityType
	{
		enum Id
		{
			Unknown,
			Player,
			Item,
			Projectile,		
			Generic
		};
	};

	struct BeamType
	{
		enum Id
		{
			None,
			LG,
			RailTrail
		};
	};

	struct GenericType
	{
		enum Id
		{
			NotGeneric,
			Bullet,
			Miss,
			Hit,
			RocketSplash,
			PlasmaSplash,
			LgSplash,
			GrenadeSplash
		};
	};

	struct ProjectileType
	{
		enum Id
		{
			NotAProjectile,
			Rocket,
			PlasmaBall,
			Grenade
		};
	};

	struct ItemType
	{
		enum Id
		{
			NotAnItem,
			Shard,
			YA,
			RA,
			HP_5,
			HP_25,
			HP_50,
			MegaHealth,
			Gauntlet,
			MG,
			SG,
			GL,
			RL,
			LG,
			RG,
			PG,
			MG_Ammo,
			SG_Ammo,
			GL_Ammo,
			RL_Ammo,
			LG_Ammo,
			RG_Ammo,
			PG_Ammo
		};
	};

	struct PlayerNameInfo
	{
		char Name[MAX_NAME_LENGTH];
		char Clan[MAX_NAME_LENGTH];
		char Country[MAX_NAME_LENGTH];
		int Time; // Absolute server time, [ms].
	};

	struct PlayerInfo
	{
		bool Valid;
		bool DemoTaker;
		bool Firing;
		float Position[3];
		float Angles[3];
		int Time; // Absolute server time, [ms].
		int Player; // [0-63].
		int Score;
		int	Health;
		int	Armor;
		int	CurrentWeapon;
		int CurrentAmmo;
		int	GameEnteredTime;
		int	GameLeftTime;
		int	Rank;
		int	Handicap;
		int	Powerups;
		int Flags; // EF_TALK, EF_CONNECTION, ...
		int Team;
		int BotSkill; // 0 = not a bot.
		int BeamType;
	};

	struct EntityInfo
	{
		bool inSync;
		int Time; // Absolute server time, [ms].
		float Position[3];
		float Angle;
		int Number;

		int	Type;
		int ItemType;
		int ProjectileType;
		int GenericType;

		int	Flags;
		int	Event;
		int	EventParam;
		int	Powerups;
		int	Weapon;
		int ClientNum;
		float delta;
		float Base[3];
	};
	
	struct BeamInfo
	{
		int ClientNum; // player shooting the beam
		int Time;
		float StartPosition[2];
		float EndPosition[2];
		int BeamType;
	};

	struct ScoreInfo
	{
		int Score1;
		int Score2;
		int Time;
	};

	struct EventInfo
	{
		int Time;
		std::string Event;
	};

	struct PlayerStats
	{
		int Kills;
		int TeamKills;
		int Suicides;
		int HealthTaken[4]; // Bubble, 25, 50, MH.
		int ArmorTaken[4]; // Shard, GA, YA, RA.
		int WeaponAtts[MAX_WEAPONS];
		int WeaponHits[MAX_WEAPONS];
		int WeaponTimes[MAX_WEAPONS];
		int LastWeaponTimes[MAX_WEAPONS];
	};

	struct MatchStats
	{
		int StartTime;
		int EndTime;
		PlayerStats Players[MAX_CLIENTS];
	};

	struct PlayerInfoPers
	{
		char Name[MAX_NAME_LENGTH];
		char Clan[MAX_NAME_LENGTH];
		char Country[MAX_NAME_LENGTH];
		PlayerInfo Info;
		bool Valid; // Valid right now?
	};

	struct EntityInfoPers
	{
		EntityInfo Info;
		int Index; // Index into the _inParseEntities ring buffer, unmasked.
		bool Valid; // Valid right now?
	};

	struct GameType
	{
		enum Id
		{
			FFA = 0,
			Duel = 1,
			SP = 2, // Single-player...
			// Team games:
			TDM = 3,
			CA = 4,
			CTF = 5,
			OneFlagCTF = 6,
			Obelisk = 7,
			Harvester = 8,
			FreezeTag = 9,
			Domination = 10,
			CTFS = 11,
			RedRover = 12,
			// CPMA:
			NTF = 13,
			TwoVsTwo = 14,
			HoonyMode = 15,
			Count,
			TeamGameStart = TDM,
			TeamGameEnd = HoonyMode,
			Unknown
		};

		static bool IsTeamMode(GameType::Id gt)
		{
			return gt >= TeamGameStart && gt <= TeamGameEnd;
		}

		static bool IsFlagMode(GameType::Id gt)
		{
			return gt == CTF || gt == OneFlagCTF || gt == CTFS || gt == NTF;
		}
	};

	typedef std::map<int, std::string> CommandMap;
	typedef std::map<int, std::string> ConfigStringMap;
	typedef std::vector<entityState_68_t> EntityVector68;
	typedef std::vector<entityState_73_t> EntityVector73;
	typedef std::vector<clSnapshot_t> SnapshotVector;
	typedef std::map<int, std::string> ChatMap; // The key is the absolute server time, in ms.
	typedef std::vector<DemoCut> CutVector;
	typedef std::vector<GameStateInfo> GameStateVector;
	typedef std::vector<ObituaryInfo> ObituaryVector;
	typedef std::vector<PuRunInfo> PuRunVector;
	typedef std::vector<EntityEventInfo> EntityEventVector;
	typedef std::vector<PlayerInfo> PlayerPlaybackInfoVector;
	typedef std::vector<PlayerNameInfo> PlayerNamePlaybackInfoVector;
	typedef std::vector<EntityInfo> EntityPlaybackInfoVector;
	typedef std::vector<ScoreInfo> ScorePlaybackVector;
	typedef std::vector<BeamInfo> BeamPlaybackInfoVector;
	typedef std::vector<MatchStats> MatchStatsVector;
	typedef std::vector<PlayerInfoPers> PlayerInfoVector;
	typedef std::vector<EntityInfoPers> EntityInfoVector;
	typedef std::vector<EventInfo> EventPlaybackVector;

	// General.
	std::string _mapName;
	CommandLineTokenizer _tokenizer;
	CutVector _cuts;
	GameStateVector _gameStates;
	ChatMap _chatMessages;
	ObituaryVector _obituaries;
	PuRunVector _puRuns;
	PlayerPlaybackInfoVector _playerPlaybackInfos;
	PlayerNamePlaybackInfoVector _playerNamesPlaybackInfos[MAX_CLIENTS];
	EntityPlaybackInfoVector _entityPlaybackInfos;
	BeamPlaybackInfoVector _beamPlaybackInfos;
	ScorePlaybackVector _scorePlaybackInfos;
	EventPlaybackVector _eventPlaybackInfos;
	PlayerInfoVector _players; // Fixed-size array of size MAX_CLIENTS.
	EntityInfoVector _entities; // Fixed-size array of size MAX_GENTITIES.
	MatchStatsVector _matchStats;
	std::string _inFilePath;
	std::string _outFilePath;
	Protocol::Id _protocol;
	int _gameStartTime; // Absolute, [ms].
	int _serverTime; // Absolute, [ms].
	int _demoRecordStartTime; // Absolute, [ms].
	int _demoRecordEndTime; // Absolute, [ms].
	int _demoRecordLastEndTime; // Absolute, [ms].
	int _demoFirstSnapTime; // Absolute, [ms].
	int _demoLastSnapTime; // Absolute, [ms].
	int _serverTimeOffset;  // [ms].
	int _fileStartOffset; // [bytes].
	int _lastMessageFileOffset; // [bytes].
	int _score1;
	int _score2;
	int _gameType; // GameType::Id
	bool _writeDemo;
	bool _writeFirstBlock;

	// Input data.
	FILE* _inFile;
	msg_t _inMsg;
	byte _inMsgData[MAX_MSGLEN];
	msg_t _inGamestateMsg;
	byte _inGamestateMsgData[MAX_MSGLEN];
	bool _inGamestateMsgSaved;
	msg_t _outGamestateMsg;
	byte _outGamestateMsgData[MAX_MSGLEN];
	int _inServerMessageSequence; // Unreliable.
	int _inServerCommandSequence; // Reliable.
	int _inReliableSequenceAcknowledge;
	int _inRawSequenceAcknowledge;
	int _inClientNum;
	int _inChecksumFeed;
	int _inParseEntitiesNum;
	qbool _inNewSnapshots;
	CommandMap _inCommands;
	ConfigStringMap _inConfigStrings;
	EntityEventVector _inEntityEvents;  // Fixed-size array of size MAX_PARSE_ENTITIES. Must be zeroed initially.
	SnapshotVector _inSnapshots; // Fixed-size array of size PACKET_BACKUP.
	clSnapshot_t _inSnapshot;

	// Output data.
	FILE* _outFile;
	msg_t _outMsg;
	byte _outMsgData[MAX_MSGLEN];
	int _outServerMessageSequence; // Unreliable.
	int _outServerCommandSequence; // Reliable.
	int _outSnapshotsWritten;
};

template<class DemoT, typename EntityStateT>
void Demo::ParseBaselineT(msg_t* msg, msg_t* msgOut)
{
	int newIndex = MSG_ReadBits(msg, GENTITYNUM_BITS);
	if(newIndex < 0 || newIndex >= MAX_GENTITIES) 
	{
		CloseFiles();
		LogErrorAndCrash("ParseGamestate: Baseline number out of range: %i", newIndex);
	}

	EntityStateT nullState;
	EntityStateT* newState = &static_cast<DemoT*>(this)->_inEntityBaselines[newIndex];

	// We delta from the null state because we read a full entity.
	Com_Memset(&nullState, 0, sizeof(nullState));
	MSG_ReadDeltaEntity<DemoT, EntityStateT>(msg, &nullState, newState, newIndex);

	if(ShouldWriteDemoMessages())
	{
		// @NOTE: MSG_WriteBits is called in there with newState.number as an argument.
		MSG_WriteDeltaEntity<DemoT, EntityStateT>(msgOut, &nullState, newState, qtrue);
	}
}

template<class DemoT, typename EntityStateT>
void Demo::ParsePacketEntitiesT(msg_t* msg, msg_t* /*msgOut*/, clSnapshot_t* oldframe, clSnapshot_t* newframe)
{
	newframe->parseEntitiesNum = _inParseEntitiesNum;
	newframe->numEntities = 0;

	// delta from the entities present in oldframe
	int	oldnum;
	int	newnum;
	int oldindex = 0;
	EntityStateT* oldstate = NULL;
	if(!oldframe) 
	{
		oldnum = 99999;
	} 
	else 
	{
		if(oldindex >= oldframe->numEntities) 
		{
			oldnum = 99999;
		} 
		else 
		{
			oldstate = &static_cast<DemoT*>(this)->_inParseEntities[(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}

	for(;;)
	{
		newnum = MSG_ReadBits(msg, GENTITYNUM_BITS);

		if(newnum == (MAX_GENTITIES - 1))
		{
			break;
		}

		if(msg->readcount > msg->cursize) 
		{
			CloseFiles();
			LogErrorAndCrash("ParsePacketEntities: read past the end of the current message");
		}

		while(oldnum < newnum) 
		{
			//
			// One or more entities from the old packet is unchanged.
			//

			DeltaEntityT<DemoT, EntityStateT>(msg, newframe, oldnum, oldstate, qtrue);
			oldindex++;

			if(oldindex >= oldframe->numEntities) 
			{
				oldnum = 99999;
			} 
			else 
			{
				oldstate = &static_cast<DemoT*>(this)->_inParseEntities[(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
		}

		if(oldnum == newnum)
		{
			//
			// Delta from previous state.
			//

			DeltaEntityT<DemoT, EntityStateT>(msg, newframe, newnum, oldstate, qfalse);
			oldindex++;

			if(oldindex >= oldframe->numEntities) 
			{
				oldnum = 99999;
			} 
			else
			{
				oldstate = &static_cast<DemoT*>(this)->_inParseEntities[(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if(oldnum > newnum) 
		{
			//
			// Delta from the baseline.
			//

			DeltaEntityT<DemoT, EntityStateT>(msg, newframe, newnum, &static_cast<DemoT*>(this)->_inEntityBaselines[newnum], qfalse);
			continue;
		}
	}

	// Any remaining entities in the old frame are copied over.
	while(oldnum != 99999) 
	{
		//
		// One or more entities from the old packet is unchanged.
		//

		DeltaEntityT<DemoT, EntityStateT>(msg, newframe, oldnum, oldstate, qtrue);
		oldindex++;

		if(oldindex >= oldframe->numEntities) 
		{
			oldnum = 99999;
		} 
		else 
		{
			oldstate = &static_cast<DemoT*>(this)->_inParseEntities[(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}
}

//
// Write a delta update of an entityState_t list to the output message.
//
template<class DemoT, typename EntityStateT>
void Demo::EmitPacketEntitiesT(clSnapshot_t* from, clSnapshot_t* to)
{
	msg_t* msgOut = &_outMsg;

	EntityStateT* oldent = 0;
	EntityStateT* newent = 0;
	int oldindex = 0;
	int newindex = 0;
	int oldnum;
	int newnum;

	int from_num_entities;
	if(!from) 
	{
		from_num_entities = 0;
	} 
	else 
	{
		from_num_entities = from->numEntities;
	}

	while(newindex < to->numEntities || oldindex < from_num_entities) 
	{
		if(newindex >= to->numEntities) 
		{
			newnum = 9999;
		} 
		else 
		{
			int entNum = (to->parseEntitiesNum + newindex) & (MAX_PARSE_ENTITIES - 1);
			newent = &static_cast<DemoT*>(this)->_inParseEntities[entNum];
			newnum = newent->number;
		}

		if(oldindex >= from_num_entities) 
		{
			oldnum = 9999;
		}
		else 
		{
			int entNum = (from->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES - 1);
			oldent = &static_cast<DemoT*>(this)->_inParseEntities[entNum];
			oldnum = oldent->number;
		}

		if(newnum == oldnum)
		{
			// Delta update from old position
			// because the force parameter is qfalse, this will not result
			// in any bytes being emitted if the entity has not changed at all.
			MSG_WriteDeltaEntity<DemoT, EntityStateT>(msgOut, oldent, newent, qfalse);
			oldindex++;
			newindex++;
			continue;
		}

		if(newnum < oldnum) 
		{
			// This is a new entity, send it from the baseline.
			EntityStateT* baseline = &static_cast<DemoT*>(this)->_inEntityBaselines[newnum];
			MSG_WriteDeltaEntity<DemoT, EntityStateT>(msgOut, baseline, newent, qtrue);
			newindex++;
			continue;
		}

		if(newnum > oldnum) 
		{
			// The old entity isn't present in the new message.
			MSG_WriteDeltaEntity<DemoT, EntityStateT>(msgOut, oldent, 0, qtrue);
			oldindex++;
			continue;
		}
	}

	MSG_WriteBits(msgOut, MAX_GENTITIES - 1, GENTITYNUM_BITS);
}

//
// Parses deltas from the given base and adds the resulting entity to the current frame.
//
template<class DemoT, typename EntityStateT>
void Demo::DeltaEntityT(msg_t* msg, clSnapshot_t *frame, int newnum, EntityStateT* old, qbool unchanged)
{
	// Save the parsed entity state into the big circular buffer so
	// it can be used as the source for a later delta.
	EntityStateT* state = &static_cast<DemoT*>(this)->_inParseEntities[_inParseEntitiesNum & (MAX_PARSE_ENTITIES-1)];
	const int oldnum = state->number;

	if(unchanged) 
	{
		*state = *old;
	} 
	else 
	{
		MSG_ReadDeltaEntity<DemoT, EntityStateT>(msg, old, state, newnum);

		state->pos.trTime = GetFixedOutputTime(state->pos.trTime);
		AnalyzeEntityT<DemoT, EntityStateT>(old, state, newnum);
	}

	// The entity was delta removed?
	if(state->number == (MAX_GENTITIES-1)) 
	{
		_entities[oldnum].Valid = false; // @FIXME: Valid?
		return;	
	}

	_inParseEntitiesNum++;
	frame->numEntities++;
}

template<class DemoT, typename EntityStateT>
void Demo::AnalyzeEntityT(EntityStateT* oldState, EntityStateT* newState, int newStateIndex)
{
	if(newStateIndex < 0 || newStateIndex >= MAX_PARSE_ENTITIES)
	{
		return;
	}

	EntityStateT nullState;
	if(!oldState)
	{
		oldState = &nullState;
		memset(&nullState, 0, sizeof(nullState));
	}

	// Always update the PU times to have precise measurements.
	const int puRunCount = (int)_puRuns.size();
	for(int i = 0; i < puRunCount; ++i)
	{
		if(_puRuns[i].Ended == 0 && _serverTime >= _puRuns[i].PredictedEndTime)
		{
			int estimatedDuration = GetVirtualInputTime() - _puRuns[i].VirtualServerTime;
			int predictedDuration = _puRuns[i].PredictedEndTime - (_puRuns[i].VirtualServerTime - _serverTimeOffset);
			_puRuns[i].Ended = 1;
			_puRuns[i].Duration = std::min(estimatedDuration, predictedDuration);
		}
	}

	if(oldState->powerups == 0 && newState->powerups != 0)
	{
		int newStartTime = _serverTime;

		bool old = false;
		for(int j = 0; j < puRunCount; ++j)
		{
			int oldStart = _puRuns[j].VirtualServerTime - _serverTimeOffset;
			int oldPredEnd = _puRuns[j].PredictedEndTime;
			if(newStartTime >= oldStart && newStartTime <= oldPredEnd)
			{
				old = true;
				break;
			}
		}

		if(!old && newState->clientNum >= 0 && newState->clientNum < MAX_CLIENTS)
		{
			PuRunInfo info;
			info.VirtualServerTime = GetVirtualInputTime();
			info.PlayerName = _players[newState->clientNum].Name;
			info.Player = newState->clientNum;
			info.Duration = 0;
			info.Ended = 0;
			info.Kills = 0;
			info.PredictedEndTime = _serverTime + 30*1000;
			info.Pu = ConvertPowerUpFlagsToValue(newState->powerups);
			info.SelfKill = 0;
			info.TeamKills = 0;
			_puRuns.push_back(info);
		}
	}

	// Did we process this message?
	EntityEventInfo* eventInfo = &_inEntityEvents[newStateIndex];
	if( _inRawSequenceAcknowledge == eventInfo->PreviousRelAck &&
		(newState->event & ~EV_EVENT_BITS) == eventInfo->PreviousEvent && 
		newState->eType == eventInfo->PreviousEventType)
	{
		return;
	}

	// Keep the settings for the next test.
	eventInfo->PreviousRelAck = _inRawSequenceAcknowledge;
	eventInfo->PreviousEvent = newState->event & ~EV_EVENT_BITS;
	eventInfo->PreviousEventType = newState->eType & ~EV_EVENT_BITS;

	const int obituary = (int)ET_EVENTS + (_protocol == Protocol::Dm68 ? (int)EV_OBITUARY : (int)EV_OBITUARY_73);
	const int death1 = (int)ET_EVENTS + (_protocol == Protocol::Dm68 ? (int)EV_DEATH1 : (int)EV_DEATH1_73);
	const int death2 = (int)ET_EVENTS + (_protocol == Protocol::Dm68 ? (int)EV_DEATH2 : (int)EV_DEATH2_73);
	const int death3 = (int)ET_EVENTS + (_protocol == Protocol::Dm68 ? (int)EV_DEATH3 : (int)EV_DEATH3_73);

	const int eventType = newState->eType & ~EV_EVENT_BITS;
	if(eventType == obituary) 
	{
		AnalyzeEntityObituaryT<DemoT, EntityStateT>(oldState, newState, newStateIndex);
	}
	else if(eventType == death1 || eventType == death2 || eventType == death3) 
	{
		AnalyzeEntityDeathT<DemoT, EntityStateT>(oldState, newState, newStateIndex);
	}
}

template<class DemoT, typename EntityStateT>
void Demo::AnalyzeEntityDeathT(EntityStateT* /*oldState*/, EntityStateT* /*newState*/, int newStateIndex)
{
	int puRunCount = (int)_puRuns.size();
	for(int i = 0; i < puRunCount; ++i)
	{
		if(_puRuns[i].Ended != 0)
		{
			continue;
		}

		if(_puRuns[i].Player != newStateIndex)
		{
			continue;
		}

		_puRuns[i].Ended = 1;
		_puRuns[i].Duration = GetVirtualInputTime() - _puRuns[i].VirtualServerTime;
	}
}

template<class DemoT, typename EntityStateT>
void Demo::AnalyzeEntityObituaryT(EntityStateT* /*oldState*/, EntityStateT* newState, int /*newStateIndex*/)
{
	int target = newState->otherEntityNum;
	if(target < 0 || target >= MAX_CLIENTS) 
	{
		return;
	}

	int attacker = newState->otherEntityNum2;
	if(attacker < 0 || attacker >= MAX_CLIENTS)
	{
		attacker = target;
	}

	ObituaryInfo info;
	info.VirtualServerTime = GetVirtualInputTime();
	info.AttackerName = _players[attacker].Name;
	info.TargetName = _players[target].Name;
	info.MeanOfDeath = newState->eventParm;
	_obituaries.push_back(info);

	int playerCsIdx = _protocol == Protocol::Dm68 ? CS_PLAYERS_68 : CS_PLAYERS_73;

	int puRunCount = (int)_puRuns.size();
	for(int i = 0; i < puRunCount; ++i)
	{
		if(_puRuns[i].Ended != 0)
		{
			continue;
		}

		if(attacker < 0 || attacker >= MAX_CLIENTS || 
			target < 0 || target >= MAX_CLIENTS)
		{
			continue;
		}

		// The player died and it was no suicide?
		if(_puRuns[i].Player == target && _puRuns[i].Player != attacker)
		{
			_puRuns[i].Ended = 1;
			_puRuns[i].Duration = GetVirtualInputTime() - _puRuns[i].VirtualServerTime;
			continue;
		}

		// Someone got killed and it wasn't by the PU guy?
		if(_puRuns[i].Player != attacker)
		{
			continue;
		}

		// The PU guy committed suicide?
		if(attacker == target)
		{
			_puRuns[i].Ended = 1;
			_puRuns[i].Duration = GetVirtualInputTime() - _puRuns[i].VirtualServerTime;
			_puRuns[i].SelfKill = 1;
		}
		else
		{
			int teamA = GetVariable(_inConfigStrings[playerCsIdx + attacker], "t");
			int teamB = GetVariable(_inConfigStrings[playerCsIdx + target], "t");
			int mod = newState->eventParm;
			if(mod == MOD_SUICIDE || (teamA == teamB && teamA != -1 && teamA != 0))
			{
				_puRuns[i].TeamKills++;
			}
			else
			{
				_puRuns[i].Kills++;
			}
		}
	}
}

template<class DemoT, typename EntityStateT>
void Demo::AnalyzeSnapshotT(const clSnapshot_t* /*oldSnap*/, const clSnapshot_t* newSnap)
{
	const int healthStatIdx = (_protocol == Protocol::Dm68) ? (int)STAT_HEALTH_68 : (int)STAT_HEALTH_73;
	const int armorStatIdx = (_protocol == Protocol::Dm68) ? (int)STAT_ARMOR_68 : (int)STAT_ARMOR_73;
	const float dotThreshold = 0.85f;
	const float minSplashDist = 1.5f;
	std::vector<int> lgSplashPosList;

	// Reset persistence.
	for(int i = 0; i < MAX_GENTITIES; ++i)
	{
		_entities[i].Valid = false;
	}
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		_players[i].Info.Valid = false;
	}

	// Update client-side entity data.
	for(int i = 0; i < newSnap->numEntities; ++i)
	{
		// Find the entity's position in the ring buffer.
		const int pos = newSnap->parseEntitiesNum + i;

		// Find the parsed entity.
		const EntityStateT* const entity = &static_cast<DemoT*>(this)->_inParseEntities[pos & (MAX_PARSE_ENTITIES-1)];

		// Skip players (processed later)
		if(entity->eType == ET_PLAYER)
		{
			continue;
		}

		// Find the client entity and update it.
		EntityInfoPers* const info = &_entities[entity->number];
		info->Index = pos; // Point back to the correct parsed entity position.
		info->Valid = true;
		
		info->Info.Number = entity->number;
		info->Info.ClientNum = entity->clientNum;
		info->Info.Time = newSnap->serverTime;
		info->Info.inSync = true;

		info->Info.Type = EntityType::Unknown;
		info->Info.ItemType = ItemType::NotAnItem;
		info->Info.ProjectileType = ProjectileType::NotAProjectile;
		info->Info.GenericType = GenericType::NotGeneric;

		info->Info.Position[0] = entity->pos.trBase[0];
		info->Info.Position[1] = entity->pos.trBase[1];
		info->Info.Position[2] = entity->pos.trBase[2];
		
		info->Info.Flags = entity->eFlags;
		info->Info.Event = entity->event;
		info->Info.EventParam = entity->eventParm;
		info->Info.Powerups = entity->powerups;
		info->Info.Weapon = entity->weapon;

		switch(entity->eType)
		{
		case ET_ITEM: 
			{
				info->Info.Type = EntityType::Item;
				info->Info.ItemType = entity->modelindex;
				break;
			}
		case ET_EVENTS + EV_MISSILE_MISS:
			{
				info->Info.Type = EntityType::Generic;
				info->Info.GenericType = GenericType::Miss;
				lgSplashPosList.push_back(entity->number);
				break;
			}
		case ET_EVENTS + EV_MISSILE_HIT : 
			{
				info->Info.Type = EntityType::Generic;
				info->Info.GenericType = GenericType::Hit;
				lgSplashPosList.push_back(entity->number);
				break;
			}
		case ET_EVENTS + EV_BULLET_HIT_WALL:
			{
				info->Info.Type = EntityType::Generic;
				info->Info.GenericType = GenericType::LgSplash;
				lgSplashPosList.push_back(entity->number);
				break;
			}
		case ET_EVENTS + EV_BULLET_HIT_FLESH : 
			{
				info->Info.Type = EntityType::Generic;
				info->Info.GenericType = GenericType::LgSplash;
				lgSplashPosList.push_back(entity->number);
				break;
			}
		case ET_GENERAL	: 
			{
				info->Info.Type = EntityType::Generic;
				if(entity->weapon == WP_ROCKET_LAUNCHER)
					info->Info.GenericType = GenericType::RocketSplash;
				else if (entity->weapon == WP_PLASMAGUN)
					info->Info.GenericType = GenericType::PlasmaSplash;
				else if (entity->weapon == WP_GRENADE_LAUNCHER)
					info->Info.GenericType = GenericType::GrenadeSplash;
				break;
			}
		default:
			break;
		}

		// Compute projectiles and items interpolated position and 2D angle.
		if(entity->eType == ET_MISSILE || entity->eType == ET_ITEM)
		{
			float deltaTime = 1.0f + 0.001f * (float)(newSnap->serverTime - entity->pos.trTime);

			// Fix Grenades bouncing
			if(deltaTime < 0.0 && deltaTime > -0.001)
				deltaTime = 0;

			while(deltaTime < -0.001f)
				deltaTime += 1.0f;


			info->Info.Position[0] = entity->pos.trBase[0] + deltaTime * entity->pos.trDelta[0];
			info->Info.Position[1] = entity->pos.trBase[1] + deltaTime * entity->pos.trDelta[1];
			info->Info.Position[2] = entity->pos.trBase[2] + deltaTime * entity->pos.trDelta[2];

			info->Info.delta = deltaTime;

			info->Info.Base[0] = entity->pos.trBase[0];
			info->Info.Base[1] = entity->pos.trBase[1];
			info->Info.Base[2] = entity->pos.trBase[2];

			if(entity->pos.trDelta[0] != 0 || entity->pos.trDelta[1] != 0)
			{
				float angle = atan2(entity->pos.trDelta[0], entity->pos.trDelta[1]);
				if(angle < 0) angle += 2.0f * UDT_PI;

				info->Info.Angle = angle;
			}

			switch(entity->weapon)
			{
			case WP_ROCKET_LAUNCHER: 
				{
					info->Info.Type = EntityType::Projectile;
					info->Info.ProjectileType = ProjectileType::Rocket;
					break;
				}
			case WP_PLASMAGUN:
				{
					info->Info.Type = EntityType::Projectile;
					info->Info.ProjectileType = ProjectileType::PlasmaBall;
					break;
				}
			case WP_GRENADE_LAUNCHER:
				{
					info->Info.Type = EntityType::Projectile;
					info->Info.ProjectileType = ProjectileType::Grenade;
				}
			}
		}
	}

	// Save entities playback data
	for(int i = 0; i < MAX_GENTITIES; ++i)
	{
		if(_entities[i].Valid)
		{
			_entities[i].Info.Time = newSnap->serverTime;
			_entityPlaybackInfos.push_back(_entities[i].Info);
		}
	}

	// Process other players in the demo
	for(int i = 0; i < newSnap->numEntities; ++i)
	{
		const EntityStateT* playerEntity = &static_cast<DemoT*>(this)->_inParseEntities[(newSnap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES-1)];
		const int clientNum = playerEntity->clientNum;
		if(playerEntity->eType == ET_PLAYER && clientNum >= 0 && clientNum < MAX_CLIENTS)
		{
			PlayerInfo& info = _players[clientNum].Info;
			info.Time = newSnap->serverTime;
			info.Player = clientNum;
			info.Valid = true;
			info.DemoTaker = false;
			info.Firing = false;
			info.Position[0] = playerEntity->pos.trBase[0];
			info.Position[1] = playerEntity->pos.trBase[1];
			info.Position[2] = playerEntity->pos.trBase[2];
			info.Angles[0] = playerEntity->apos.trBase[0];
			info.Angles[1] = playerEntity->apos.trBase[1];
			info.Angles[2] = playerEntity->apos.trBase[2];
			info.Armor = -1;
			info.Flags = playerEntity->eFlags;
			info.Health = -1;
			info.Powerups = playerEntity->powerups;
			info.Rank = -1;
			info.Score = -1;
			info.BeamType = BeamType::None;
			info.CurrentAmmo = -1;
			switch(playerEntity->weapon)
			{
			case WP_NONE:
				info.CurrentWeapon = 0;
				break;
			case WP_GAUNTLET:
				info.CurrentWeapon = WEAPON_GAUNTLET;
				break;
			case WP_MACHINEGUN:
				info.CurrentWeapon = WEAPON_MACHINEGUN;
				break;
			case WP_SHOTGUN:
				info.CurrentWeapon = WEAPON_SHOTGUN;
				break;
			case WP_GRENADE_LAUNCHER:
				info.CurrentWeapon = WEAPON_GRENADELAUNCHER;
				break;
			case WP_ROCKET_LAUNCHER:
				info.CurrentWeapon = WEAPON_ROCKETLAUNCHER;
				break;
			case WP_LIGHTNING:
				info.CurrentWeapon = WEAPON_LIGHTNING;
				break;
			case WP_RAILGUN:
				info.CurrentWeapon = WEAPON_RAILGUN;
				break;
			case WP_PLASMAGUN:
				info.CurrentWeapon = WEAPON_PLASMAGUN;
				break;
			}

			if(playerEntity->eFlags & EF_FIRING)
			{
				info.Firing = true;
				switch(playerEntity->weapon)
				{
				case WP_LIGHTNING:
					{
						info.BeamType = BeamType::LG;

						// Find the best lg splash candidate by checking if it lies along the player view
						int bestLgSplashIndex = -1;
						float bestDot = dotThreshold; // lower threshold to consider a candidate

						// View vector
						const float angleRad1 = DegToRad(info.Angles[1]);
						float view[2];
						view[0] = cos(angleRad1);
						view[1] = sin(angleRad1);

						// Normalize the view vector
						float len = view[0]*view[0] + view[1]*view[1];
						if(len < 1e-10)
							break;

						len = sqrt(len);
						view[0] /= len; view[1] /= len;

						for(size_t j = 0; j < lgSplashPosList.size(); j++)
						{
							EntityInfoPers* const lgSplash = &_entities[lgSplashPosList[j]];

							// vector from the player to the lg splash
							float beam[2]; 
							beam[0] = lgSplash->Info.Position[0] - info.Position[0];
							beam[1] = lgSplash->Info.Position[1] - info.Position[1];

							// Normalize the beam vector
							float len = beam[0]*beam[0] + beam[1]*beam[1];

							// During lg fights, the opponent lg splash can be very close to the player							
							if(len < minSplashDist*minSplashDist) // min value 1e-10 for numerical errors with sqrt
								continue;

							len = sqrt(len); 
							beam[0] /= len; beam[1] /= len;

							// Compute dot product
							float dot = beam[0]*view[0] + beam[1]*view[1];

							if(dot > bestDot)
							{
								bestDot = dot;
								bestLgSplashIndex = j;
							}
						}

						// Create a beam using the best candidate
						if(bestLgSplashIndex >= 0)
						{
							EntityInfoPers* const lgSplash = &_entities[lgSplashPosList[bestLgSplashIndex]];

							// It seems there are less LG Splash than snapshots while firing
							// So keep the length of the shaft beam using the last LG Splash, but change the angle
							float v[2]; 
							v[0] = lgSplash->Info.Position[0] - info.Position[0];
							v[1] = lgSplash->Info.Position[1] - info.Position[1];

							float len = v[0]*v[0] + v[1]*v[1];
							if(len < 1e-8)
								break;
							len = sqrt(len);


							BeamInfo beam;
							beam.ClientNum = info.Player;
							beam.Time = newSnap->serverTime;
							beam.BeamType = BeamType::LG;
							beam.StartPosition[0] = info.Position[0];
							beam.StartPosition[1] = info.Position[1];
							beam.EndPosition[0] = info.Position[0] + view[0] * len;
							beam.EndPosition[1] = info.Position[1] + view[1] * len;

							_beamPlaybackInfos.push_back(beam);
						}
						else
						{
							BeamInfo beam;
							beam.ClientNum = info.Player;
							beam.Time = newSnap->serverTime;
							beam.BeamType = BeamType::LG;
							beam.StartPosition[0] = info.Position[0];
							beam.StartPosition[1] = info.Position[1];
							beam.EndPosition[0] = info.Position[0] + cos(angleRad1) * 1000.0f;
							beam.EndPosition[1] = info.Position[1] + sin(angleRad1) * 1000.0f;

							_beamPlaybackInfos.push_back(beam);
						}

						break;
					}
				case WP_RAILGUN:
					{
						info.BeamType = BeamType::RailTrail;
						break;
					}
				}
			}
			_playerPlaybackInfos.push_back(info);
		}
	}

	// Process the player taking the demo
	const playerState_t* playerEntity = &newSnap->ps;
	const int clientNum = playerEntity->clientNum;
	if(clientNum >= 0 && clientNum < MAX_CLIENTS)
	{
		PlayerInfo& info = _players[clientNum].Info;
		info.Time = newSnap->serverTime;
		info.Player = clientNum;
		info.Valid = true;
		info.DemoTaker = true;
		info.Firing = false;
		info.Position[0] = playerEntity->origin[0];
		info.Position[1] = playerEntity->origin[1];
		info.Position[2] = playerEntity->origin[2];
		info.Angles[0] = playerEntity->viewangles[0];
		info.Angles[1] = playerEntity->viewangles[1];
		info.Angles[2] = playerEntity->viewangles[2];
		info.Armor = playerEntity->stats[armorStatIdx];
		info.Flags = playerEntity->eFlags;
		info.Health = playerEntity->stats[healthStatIdx];
		//info.Powerups = _protocol == Protocol::Dm68 ? 0 : playerEntity->stats[STAT_PERSISTANT_POWERUP_73]; // @TODO:
		info.Powerups = 0;
		for (int i = 0 ; i < MAX_POWERUPS ; i++ ) 
		{
			int t = playerEntity->powerups[i];
			if (t > 0) 
			{
				info.Powerups |= 1 << i;
			}
		}
		info.Rank = playerEntity->persistant[PERS_RANK];
		info.Score = playerEntity->persistant[PERS_SCORE];
		info.BeamType = BeamType::None;
		info.CurrentAmmo = playerEntity->ammo[playerEntity->weapon];


		//info.CurrentWeapon = playerEntity->weapon;
		switch(playerEntity->weapon)
		{
		case WP_NONE:
			info.CurrentWeapon = 0;
			break;
		case WP_GAUNTLET:
			info.CurrentWeapon = WEAPON_GAUNTLET;
			break;
		case WP_MACHINEGUN:
			info.CurrentWeapon = WEAPON_MACHINEGUN;
			break;
		case WP_SHOTGUN:
			info.CurrentWeapon = WEAPON_SHOTGUN;
			break;
		case WP_GRENADE_LAUNCHER:
			info.CurrentWeapon = WEAPON_GRENADELAUNCHER;
			break;
		case WP_ROCKET_LAUNCHER:
			info.CurrentWeapon = WEAPON_ROCKETLAUNCHER;
			break;
		case WP_LIGHTNING:
			info.CurrentWeapon = WEAPON_LIGHTNING;
			break;
		case WP_RAILGUN:
			info.CurrentWeapon = WEAPON_RAILGUN;
			break;
		case WP_PLASMAGUN:
			info.CurrentWeapon = WEAPON_PLASMAGUN;
			break;
		}

		if(playerEntity->eFlags & EF_FIRING)
		{
			info.Firing = true;
			switch(playerEntity->weapon)
			{
			case WP_LIGHTNING:
				{
					info.BeamType = BeamType::LG;

					// Find the best lg splash candidate by checking if it lies along the player view
					int bestLgSplashIndex = -1;
					float bestDot = dotThreshold; // lower threshold to consider a candidate

					const float angleRad1 = DegToRad(info.Angles[1]);

					// View vector
					float view[2];
					view[0] = cos(angleRad1);
					view[1] = sin(angleRad1);

					// Normalize the view vector
					float len = view[0]*view[0] + view[1]*view[1];

					if(len < 1e-10)
						break;

					len = sqrt(len);
					view[0] /= len; view[1] /= len;

					for(size_t j = 0; j < lgSplashPosList.size(); j++)
					{
						EntityInfoPers* const lgSplash = &_entities[lgSplashPosList[j]];

						// vector from the player to the lg splash
						float beam[2]; 
						beam[0] = lgSplash->Info.Position[0] - info.Position[0];
						beam[1] = lgSplash->Info.Position[1] - info.Position[1];

						// Normalize the beam vector
						float len = beam[0]*beam[0] + beam[1]*beam[1];

						// During lg fights, the opponent lg splash can be very close to the player							
						if(len < minSplashDist*minSplashDist) // min value 1e-10 for numerical errors with sqrt
							continue;

						len = sqrt(len); 
						beam[0] /= len; beam[1] /= len;

						// Compute dot product
						float dot = beam[0]*view[0] + beam[1]*view[1];

						if(dot > bestDot)
						{
							bestDot = dot;
							bestLgSplashIndex = j;
						}
					}

					// Create a beam using the best candidate
					if(bestLgSplashIndex >= 0)
					{
						EntityInfoPers* const lgSplash = &_entities[lgSplashPosList[bestLgSplashIndex]];

						// It seems there are less LG Splash than snapshots while firing
						// So keep the length of the shaft beam using the last LG Splash, but change the angle
						float v[2]; 
						v[0] = lgSplash->Info.Position[0] - info.Position[0];
						v[1] = lgSplash->Info.Position[1] - info.Position[1];

						float len = v[0]*v[0] + v[1]*v[1];
						if(len < 1e-8)
							break;
						len = sqrt(len);


						BeamInfo beam;
						beam.ClientNum = info.Player;
						beam.Time = newSnap->serverTime;
						beam.BeamType = BeamType::LG;
						beam.StartPosition[0] = info.Position[0];
						beam.StartPosition[1] = info.Position[1];
						beam.EndPosition[0] = info.Position[0] + view[0] * len;
						beam.EndPosition[1] = info.Position[1] + view[1] * len;

						_beamPlaybackInfos.push_back(beam);
					}
					else
					{
						BeamInfo beam;
						beam.ClientNum = info.Player;
						beam.Time = newSnap->serverTime;
						beam.BeamType = BeamType::LG;
						beam.StartPosition[0] = info.Position[0];
						beam.StartPosition[1] = info.Position[1];
						beam.EndPosition[0] = info.Position[0] + cos(angleRad1) * 1000.0f;
						beam.EndPosition[1] = info.Position[1] + sin(angleRad1) * 1000.0f;

						_beamPlaybackInfos.push_back(beam);
					}
					break;
				}
			case WP_RAILGUN:
				{
					info.BeamType = BeamType::RailTrail;
					break;
				}
			}
		}
		_playerPlaybackInfos.push_back(info);
	}

	// Save score playback data.
	int score1 = -9999;
	int score2 = -9999;
	ProtocolGetScores(score1, score2);
	ScoreInfo scoreInfo;
	scoreInfo.Score1 = score1;
	scoreInfo.Score2 = score2;
	scoreInfo.Time = newSnap->serverTime;
	_scorePlaybackInfos.push_back(scoreInfo);
}
