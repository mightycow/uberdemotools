#include "analysis_cut_by_mid_air.hpp"
#include "utils.hpp"
#include "math.hpp"

#include <string.h>
#include <math.h>


// The player's bounding box is approx. 32x32x56 units so if we sum up the 2 imprecisions 
// 1. missile start position (fired) vs attacker position at shooting time
// 2. missile end position (hit) vs victim position at hit time
// We'll get something close to 32, if not bigger when the missile angle is pretty vertical.
#define    UDT_MAX_PROJ_SPEED_RATIO_WHEN_FASTER    2.0f
#define    UDT_MAX_PROJ_SPEED_RATIO_WHEN_SLOWER    1.2f
#define    UDT_MAX_PLAYER_DISAPPEARANCE_TIME_MS    100
#define    UDT_AVG_PLAYER_TO_PROJECTILE_DELTA      32.0f
#define    ID_MISSILE_PRESTEP_TIME_MS              50 // From Quake 3's g_missile.c


static s32 GetEntityStateGravity(const idEntityStateBase* ent, udtProtocol::Id protocol)
{
	// @NOTE: the original Quake 3 id function BG_EvaluateTrajectory
	// didn't use the local gravity field but used the DEFAULT_GRAVITY constant instead.

	if(protocol == udtProtocol::Dm73)
	{
		return ((const idEntityState73*)ent)->pos_gravity;
	}

	if(protocol == udtProtocol::Dm90)
	{
		return ((const idEntityState90*)ent)->pos_gravity;
	}

	return DEFAULT_GRAVITY;
}

static s32 GetEntityStateGravitySafe(const idEntityStateBase* ent, udtProtocol::Id protocol)
{
	const s32 gravity = GetEntityStateGravity(ent, protocol);

	return gravity > 0 ? gravity : DEFAULT_GRAVITY;
}

static void ComputeTrajectoryPosition(f32* result, const idEntityStateBase* ent, s32 atTime, udtProtocol::Id protocol)
{
	const idTrajectoryBase& tr = ent->pos;
	switch(tr.trType)
	{
		case TR_LINEAR:
			Float3::Mad(result, tr.trBase, tr.trDelta, (atTime - tr.trTime) * 0.001f);
			break;

		case TR_SINE:
			{
				const f32 deltaTime = (atTime - tr.trTime) / (f32)tr.trDuration;
				const f32 phase = sinf(deltaTime * UDT_PI * 2.0f);
				Float3::Mad(result, tr.trBase, tr.trDelta, phase);
			}
			break;

		case TR_LINEAR_STOP:
			{
				if(atTime > tr.trTime + tr.trDuration)
				{
					atTime = tr.trTime + tr.trDuration;
				}

				f32 deltaTime = (atTime - tr.trTime) * 0.001f;
				if(deltaTime < 0.0f)
				{
					deltaTime = 0.0f;
				}

				Float3::Mad(result, tr.trBase, tr.trDelta, deltaTime);
			}
			break;

		case TR_GRAVITY:
			{
				const f32 gravity = (f32)GetEntityStateGravitySafe(ent, protocol);
				const f32 deltaTime = (atTime - tr.trTime) * 0.001f;
				Float3::Mad(result, tr.trBase, tr.trDelta, deltaTime);
				result[2] -= 0.5f * gravity * deltaTime * deltaTime;
			}
			break;

		case TR_STATIONARY:
		case TR_INTERPOLATE:
		default:
			Float3::Copy(result, tr.trBase);
			break;
	}
}

static bool IsAllowedIdMeanOfDeath(s32 idMOD, udtProtocol::Id procotol)
{
	const s32 bit = GetUDTPlayerMODBitFromIdMod(idMOD, procotol);

	return
		bit == (s32)udtPlayerMeansOfDeathBits::Grenade ||
		bit == (s32)udtPlayerMeansOfDeathBits::Rocket ||
		bit == (s32)udtPlayerMeansOfDeathBits::BFG;
}

static bool IsAllowedIdWeapon(s32 idWeapon, udtProtocol::Id procotol)
{
	const s32 weapon = GetUDTWeaponFromIdWeapon(idWeapon, procotol);

	return
		weapon == (s32)udtWeapon::RocketLauncher ||
		weapon == (s32)udtWeapon::GrenadeLauncher ||
		weapon == (s32)udtWeapon::BFG;
}

static s32 RoundToNearest(s32 x, s32 n)
{
	return ((x + (n / 2)) / n) * n;
}

static bool IsAllowedUDTWeapon(u32 udtWeapon, u32 allowedWeapons)
{
	const u32 result = allowedWeapons & (1 << udtWeapon);

	return result != 0;
}

static void PlayerStateToEntityState(idEntityStateBase& es, s32& lastEventSequence, const idPlayerStateBase& ps, bool extrapolate, s32 serverTimeMs, udtProtocol::Id protocol)
{
	const u32 healthStatIdx = (protocol == udtProtocol::Dm68) ? (u32)STAT_HEALTH_68 : (u32)STAT_HEALTH_73p;
	if(ps.pm_type == PM_INTERMISSION || ps.pm_type == PM_SPECTATOR)
	{
		es.eType = ET_INVISIBLE;
	}
	else if(ps.stats[healthStatIdx] <= GIB_HEALTH)
	{
		es.eType = ET_INVISIBLE;
	}
	else
	{
		es.eType = ET_PLAYER;
	}

	es.number = ps.clientNum;

	Float3::Copy(es.pos.trBase, ps.origin);
	Float3::Copy(es.pos.trDelta, ps.velocity); // set the trDelta for flag direction
	if(extrapolate)
	{
		es.pos.trType = TR_LINEAR_STOP;
		es.pos.trTime = serverTimeMs; // set the time for linear prediction
		es.pos.trDuration = 50; // set maximum extrapolation time: 1000 / sv_fps (default = 20)
	}
	else
	{
		es.pos.trType = TR_INTERPOLATE;
	}

	es.apos.trType = TR_INTERPOLATE;
	Float3::Copy(es.apos.trBase, ps.viewangles);

	es.angles2[YAW] = (f32)ps.movementDir;
	es.legsAnim = ps.legsAnim;
	es.torsoAnim = ps.torsoAnim;
	es.clientNum = ps.clientNum;
	// ET_PLAYER looks here instead of at number
	// so corpses can also reference the proper config
	es.eFlags = ps.eFlags;
	if(ps.stats[healthStatIdx] <= 0)
	{
		es.eFlags |= EF_DEAD;
	}
	else
	{
		es.eFlags &= ~EF_DEAD;
	}

	if(ps.eventSequence != lastEventSequence)
	{
		const s32 seq = (ps.eventSequence & 1) ^ 1;
		es.event = ps.events[seq];
		es.eventParm = ps.eventParms[seq];
		lastEventSequence = ps.eventSequence;
	}
	else
	{
		es.event = 0;
		es.eventParm = 0;
	}

#if 0
	// Original id code for reference.
	if(ps.externalEvent)
	{
		es.event = ps.externalEvent;
		es.eventParm = ps.externalEventParm;
	}
	else if(ps.entityEventSequence < ps.eventSequence)
	{
		int		seq;

		if(ps.entityEventSequence < ps.eventSequence - MAX_PS_EVENTS)
		{
			ps.entityEventSequence = ps.eventSequence - MAX_PS_EVENTS;
		}
		seq = ps.entityEventSequence & (MAX_PS_EVENTS - 1);
		es.event = ps.events[seq] | ((ps.entityEventSequence & 3) << 8);
		es.eventParm = ps.eventParms[seq];
		ps.entityEventSequence++;
	}
#endif

	es.weapon = ps.weapon;
	es.groundEntityNum = ps.groundEntityNum;

	es.powerups = 0;
	for(s32 i = 0; i < ID_MAX_PS_POWERUPS; i++)
	{
		if(ps.powerups[i])
		{
			es.powerups |= 1 << i;
		}
	}

	es.loopSound = ps.loopSound;
	es.generic1 = ps.generic1;
}


struct PlayerEntities
{
	PlayerEntities()
	{
		Players.Init(1 << 16, "PlayerEntities::PlayersArray");
	}

	idLargestEntityState TempEntityState;
	udtVMArray<idEntityStateBase*> Players;
};

static void GetPlayerEntities(PlayerEntities& info, s32& lastEventSequence, const udtSnapshotCallbackArg& arg, udtProtocol::Id protocol)
{
	info.Players.Clear();

	idPlayerStateBase* const ps = GetPlayerState(arg.Snapshot, protocol);
	if(ps != NULL)
	{
		PlayerStateToEntityState(info.TempEntityState, lastEventSequence, *ps, false, 0, protocol);
		info.Players.Add(&info.TempEntityState);
	}

	for(u32 i = 0; i < arg.EntityCount; ++i)
	{
		idEntityStateBase* const es = arg.Entities[i].Entity;
		if(es->eType == ET_PLAYER && es->clientNum >= 0 && es->clientNum < ID_MAX_CLIENTS)
		{
			info.Players.Add(es);
		}
	}
}


udtMidAirPatternAnalyzer::udtMidAirPatternAnalyzer()
{
}

udtMidAirPatternAnalyzer::~udtMidAirPatternAnalyzer()
{
}

void udtMidAirPatternAnalyzer::StartAnalysis()
{
	_protocol = udtProtocol::Invalid;
	_lastEventSequence = S32_MIN;
	_gameStateIndex = -1;
	_rocketSpeed = -1.0f;
	_bfgSpeed = -1.0f;
	memset(_projectiles, 0, sizeof(_projectiles));
	memset(_players, 0, sizeof(_players));
}

void udtMidAirPatternAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& parser)
{
	_protocol = parser._inProtocol;

	++_gameStateIndex;

	for(u32 i = 0; i < (u32)UDT_COUNT_OF(_players); ++i)
	{
		_players[i].LastUpdateTime = S32_MIN;
		_players[i].LastGroundContactTime = S32_MIN;
		_players[i].LastZDirChangeTime = S32_MIN;
		_players[i].ZDir = 0;
		_players[i].Position[0] = 0.0f;
		_players[i].Position[1] = 0.0f;
		_players[i].Position[2] = 0.0f;
	}
}

void udtMidAirPatternAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	const s32 trackedPlayerIndex = PlugIn->GetTrackedPlayerIndex();

	// Update the rocket speed if needed.
	if(_rocketSpeed == -1.0f)
	{
		for(u32 i = 0; i < arg.EntityCount; ++i)
		{
			const idEntityStateBase* const ent = arg.Entities[i].Entity;
			if(ent->eType == ET_MISSILE && GetUDTWeaponFromIdWeapon(ent->weapon, _protocol) == udtWeapon::RocketLauncher)
			{
				const f32 speed = Float3::Length(ent->pos.trDelta);
				_rocketSpeed = (f32)RoundToNearest((s32)speed, 25);
			}
		}
	}

	// Update the BFG speed if needed.
	if(_bfgSpeed == -1.0f)
	{
		for(u32 i = 0; i < arg.EntityCount; ++i)
		{
			const idEntityStateBase* const ent = arg.Entities[i].Entity;
			if(ent->eType == ET_MISSILE && GetUDTWeaponFromIdWeapon(ent->weapon, _protocol) == udtWeapon::BFG)
			{
				const f32 speed = Float3::Length(ent->pos.trDelta);
				_bfgSpeed = (f32)RoundToNearest((s32)speed, 25);
			}
		}
	}

	// Update player information: position, Z-axis change, fire projectiles, etc.
	PlayerEntities playersInfo;
	GetPlayerEntities(playersInfo, _lastEventSequence, arg, parser._inProtocol);
	for(u32 i = 0, count = playersInfo.Players.GetSize(); i < count; ++i)
	{
		idEntityStateBase* const es = playersInfo.Players[i];
		f32 currentPosition[3];
		ComputeTrajectoryPosition(currentPosition, es, arg.ServerTime, _protocol);

		if(es->clientNum == trackedPlayerIndex)
		{
			// Store the projectile fired by the tracked player, if any.
			const s32 eventType = es->event & (~ID_ES_EVENT_BITS);
			const s32 fireWeaponEventId = idEntityEvent::WeaponFired(_protocol);
			if(eventType == fireWeaponEventId && IsAllowedIdWeapon(es->weapon, _protocol))
			{
				AddProjectile(es->weapon, currentPosition, arg.ServerTime);
			}
		}

		PlayerInfo& player = _players[es->clientNum];

		const f32 ZDirDelta = 1.0f;
		const s32 oldZDir = player.ZDir;
		const f32 ZChange = currentPosition[2] - player.Position[2];
		Float3::Copy(player.Position, currentPosition);

		s32 ZDir = 0;
		if(ZChange > ZDirDelta) ZDir = 1;
		else if(ZChange < -ZDirDelta) ZDir = -1;

		// Going up then down should be allowed.
		const bool anyChange = ZDir != oldZDir;
		const bool upThenNoChange = oldZDir == 1 && ZDir == 0;
		const bool upThenDown = oldZDir == 1 && ZDir == -1;
		const bool noChangeThenDown = oldZDir == 0 && ZDir == -1;
		const bool forbiddenChange = anyChange && !upThenNoChange && !upThenDown && !noChangeThenDown;

		player.ZDir = ZDir;
		if(ZDir == 0 || forbiddenChange)
		{
			player.LastZDirChangeTime = arg.ServerTime;
		}

		if(es->groundEntityNum != ENTITYNUM_NONE)
		{
			player.LastGroundContactTime = arg.ServerTime;
		}

		player.LastUpdateTime = arg.ServerTime;
	}

	// Find a player getting mid-aired.
	for(u32 i = 0; i < arg.EntityCount; ++i)
	{
		if(!arg.Entities[i].IsNewEvent)
		{
			continue;
		}

		udtObituaryEvent eventInfo;
		if(!IsObituaryEvent(eventInfo, *arg.Entities[i].Entity, parser._inProtocol))
		{
			continue;
		}

		const s32 targetIdx = eventInfo.TargetIndex;
		if(targetIdx == trackedPlayerIndex)
		{
			continue;
		}

		const s32 attackerIdx = eventInfo.AttackerIndex;
		if(attackerIdx == -1 || attackerIdx != trackedPlayerIndex)
		{
			continue;
		}

		const s32 idMeanOfDeath = arg.Entities[i].Entity->eventParm;
		if(!IsAllowedIdMeanOfDeath(idMeanOfDeath, parser._inProtocol))
		{
			continue;
		}

		if(_players[targetIdx].LastGroundContactTime == arg.ServerTime ||
		   arg.ServerTime - _players[targetIdx].LastUpdateTime > UDT_MAX_PLAYER_DISAPPEARANCE_TIME_MS)
		{
			continue;
		}

		const udtMidAirPatternArg& extraInfo = GetExtraInfo<udtMidAirPatternArg>();
		const s32 udtWeapon = GetUDTWeaponFromIdMod(idMeanOfDeath, _protocol);
		if(udtWeapon != -1 && !IsAllowedUDTWeapon(udtWeapon, extraInfo.AllowedWeapons))
		{
			continue;
		}

		ProjectileInfo* projectile = FindBestProjectileMatch(udtWeapon, _players[targetIdx].Position, arg.ServerTime);
		if(projectile == NULL)
		{
			continue;
		}
		projectile->UsedSlot = 0;

		const u32 victimAirTimeMs = (u32)udt_max<s32>(0, _players[targetIdx].LastUpdateTime - _players[targetIdx].LastZDirChangeTime);
		if(extraInfo.MinAirTimeMs > 0 && victimAirTimeMs < extraInfo.MinAirTimeMs)
		{
			continue;
		}

		const u32 projectileDistance = (u32)Float3::Dist(projectile->CreationPosition, _players[targetIdx].Position);
		if(projectileDistance < extraInfo.MinDistance)
		{
			continue;
		}

		const udtPatternSearchArg& info = PlugIn->GetInfo();
		udtCutSection cut;
		cut.VeryShortDesc = "midair";
		cut.GameStateIndex = _gameStateIndex;
		cut.StartTimeMs = arg.ServerTime - info.StartOffsetSec * 1000;
		cut.EndTimeMs = arg.ServerTime + info.EndOffsetSec * 1000;
		cut.PatternTypes = UDT_BIT((u32)udtPatternType::MidAirFrags);
		CutSections.Add(cut);
	}
}

void udtMidAirPatternAnalyzer::AddProjectile(s32 weapon, const f32* position, s32 serverTimeMs)
{
	ProjectileInfo* projectile = NULL;
	for(u32 i = 0; i < UDT_COUNT_OF(_projectiles); ++i)
	{
		if(_projectiles[i].UsedSlot == 0)
		{
			projectile = &_projectiles[i];
			break;
		}
	}

	if(projectile == NULL)
	{
		// We didn't find a free slot, find and pick the oldest one.
		s32 oldestTimeMs = S32_MAX;
		projectile = &_projectiles[0];
		for(u32 i = 0; i < UDT_COUNT_OF(_projectiles); ++i)
		{
			if(_projectiles[i].CreationTimeMs < oldestTimeMs)
			{
				oldestTimeMs = _projectiles[i].CreationTimeMs;
				projectile = &_projectiles[i];
			}
		}
	}

	projectile->UsedSlot = 1;
	Float3::Copy(projectile->CreationPosition, position);
	projectile->CreationTimeMs = serverTimeMs;
	projectile->IdWeapon = weapon;
}

udtMidAirPatternAnalyzer::ProjectileInfo* udtMidAirPatternAnalyzer::FindBestProjectileMatch(s32 udtWeapon, const f32* targetPosition, s32 serverTimeMs)
{
	f32 targetTravelSpeed = -1.0f;
	if(udtWeapon == (s32)udtWeapon::BFG)
	{
		targetTravelSpeed = _bfgSpeed == -1.0f ? 2000.0f : _bfgSpeed;
	}
	else
	{
		targetTravelSpeed = _rocketSpeed == -1.0f ? 1000.0f : _rocketSpeed;
	}

	f32 smallestScale = 9999.0f;
	s32 smallestScaleIndex = -1;
	for(u32 i = 0; i < UDT_COUNT_OF(_projectiles); ++i)
	{
		const ProjectileInfo info = _projectiles[i];
		if(info.UsedSlot == 0)
		{
			continue;
		}

		const s32 udtProjWeapon = GetUDTWeaponFromIdWeapon(info.IdWeapon, _protocol);
		if(udtProjWeapon != udtWeapon)
		{
			continue;
		}

		if(serverTimeMs == info.CreationTimeMs)
		{
			return &_projectiles[i];
		}

		const f32 computedDist = udt_max(0.0f, Float3::Dist(targetPosition, info.CreationPosition) - UDT_AVG_PLAYER_TO_PROJECTILE_DELTA);
		const f32 computedDuration = (serverTimeMs - info.CreationTimeMs + ID_MISSILE_PRESTEP_TIME_MS) * 0.001f;
		const f32 computedSpeed = computedDist / computedDuration;
		const f32 scale = computedSpeed >= targetTravelSpeed ? (computedSpeed / targetTravelSpeed) : (targetTravelSpeed / computedSpeed);
		const bool speedOkay = 
			(computedSpeed >  targetTravelSpeed && scale < UDT_MAX_PROJ_SPEED_RATIO_WHEN_FASTER) ||
			(computedSpeed <= targetTravelSpeed && scale < UDT_MAX_PROJ_SPEED_RATIO_WHEN_SLOWER);
		if(scale < smallestScale && speedOkay)
		{
			smallestScale = scale;
			smallestScaleIndex = i;
		}
	}

	if(smallestScaleIndex >= 0 && smallestScaleIndex < (s32)UDT_COUNT_OF(_projectiles))
	{
		return &_projectiles[smallestScaleIndex];
	}

	return NULL;
}
