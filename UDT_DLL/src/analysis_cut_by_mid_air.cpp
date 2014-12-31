#include "analysis_cut_by_mid_air.hpp"
#include "utils.hpp"
#include "math.hpp"

#include <string.h>
#include <math.h>


static s32 GetEntityStateGravity(const idEntityStateBase* ent, udtProtocol::Id protocol)
{
	// @NOTE: the original Quake 3 id function BG_EvaluateTrajectory
	// didn't use the local gravity field but used the 800 constant instead.

	if(protocol == udtProtocol::Dm73)
	{
		return ((const idEntityState73*)ent)->pos_gravity;
	}

	if(protocol == udtProtocol::Dm90)
	{
		return ((const idEntityState90*)ent)->pos_gravity;
	}

	return 800;
}

static s32 GetEntityStateGravitySafe(const idEntityStateBase* ent, udtProtocol::Id protocol)
{
	const s32 gravity = GetEntityStateGravity(ent, protocol);

	return gravity > 0 ? gravity : 800;
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

static bool IsAllowedMeanOfDeath(s32 idMOD, udtProtocol::Id procotol)
{
	const s32 bit = GetUDTPlayerMODBitFromIdMod(idMOD, procotol);

	return
		bit == (s32)udtPlayerMeansOfDeathBits::Grenade ||
		bit == (s32)udtPlayerMeansOfDeathBits::Rocket ||
		bit == (s32)udtPlayerMeansOfDeathBits::BFG;
}

static bool IsAllowedWeapon(s32 idWeapon, udtProtocol::Id procotol)
{
	const s32 weapon = GetUDTWeaponFromIdWeapon(idWeapon, procotol);

	return
		weapon == (s32)udtWeapon::RocketLauncher ||
		weapon == (s32)udtWeapon::GrenadeLauncher ||
		weapon == (s32)udtWeapon::BFG;
}

static void GetEntityStateEventFromPlayerState(s32& event, s32& eventParm, const idPlayerStateBase* ps)
{
	const s32 seq = (ps->eventSequence & 1) ^ 1;
	event = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
	eventParm = ps->eventParms[seq];
}

static s32 RoundToNearest(s32 x, s32 n)
{
	return ((x + (n / 2)) / n) * n;
}


static bool IsAllowedWeapon(u32 udtWeapon, u32 allowedWeapons)
{
	const u32 result = allowedWeapons & (1 << udtWeapon);

	return result != 0;
}


udtCutByMidAirAnalyzer::udtCutByMidAirAnalyzer()
{
	_protocol = udtProtocol::Invalid;
	_lastEventSequence = S32_MIN;
	_gameStateIndex = -1;
	_rocketSpeed = -1.0f;
	_bfgSpeed = -1.0f;
	memset(_projectiles, 0, sizeof(_projectiles));
	memset(_players, 0, sizeof(_players));
}

udtCutByMidAirAnalyzer::~udtCutByMidAirAnalyzer()
{
}

void udtCutByMidAirAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& parser)
{
	_protocol = parser._protocol;

	++_gameStateIndex;

	for(u32 i = 0; i < (u32)UDT_COUNT_OF(_players); ++i)
	{
		_players[i].LastGroundContactTime = S32_MIN;
		_players[i].LastZDirChangeTime = S32_MIN;
		_players[i].ZDir = 0;
		_players[i].Position[0] = 0.0f;
		_players[i].Position[1] = 0.0f;
		_players[i].Position[2] = 0.0f;
	}
}

void udtCutByMidAirAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/)
{
}

void udtCutByMidAirAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	const s32 trackedPlayerIndex = PlugIn->GetTrackedPlayerIndex();

	// Store projectiles fired by the player in first-person view if needed.
	idPlayerStateBase* const ps = GetPlayerState(arg.Snapshot, _protocol);
	if(ps != NULL && ps->clientNum == trackedPlayerIndex)
	{
		s32 event = 0;
		s32 eventParm = 0;
		GetEntityStateEventFromPlayerState(event, eventParm, ps);
		const s32 eventType = event & (~EV_EVENT_BITS);
		const s32 fireWeaponEventId = (_protocol == udtProtocol::Dm68) ? (s32)EV_FIRE_WEAPON : (s32)EV_FIRE_WEAPON_73p;
		if(ps->eventSequence != _lastEventSequence &&
		   eventType == fireWeaponEventId &&
		   IsAllowedWeapon(ps->weapon, _protocol))
		{
			AddProjectile(ps->weapon, ps->origin, arg.ServerTime);
		}
		_lastEventSequence = ps->eventSequence;

		// Update this player's data.
		Float3::Copy(_players[ps->clientNum].Position, ps->origin);
		if(ps->groundEntityNum != ENTITYNUM_NONE)
		{
			_players[ps->clientNum].LastGroundContactTime = arg.ServerTime;
		}
	}

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

	// Update the data of all the other players.
	for(u32 i = 0; i < arg.EntityCount; ++i)
	{
		const idEntityStateBase* const ent = arg.Entities[i].Entity;
		if(ent->eType != ET_PLAYER || ent->clientNum < 0 || ent->clientNum >= 64)
		{
			continue;
		}

		PlayerInfo& player = _players[ent->clientNum];

		const f32 ZDirDelta = 1.0f;
		const s32 oldZDir = player.ZDir;

		f32 newPosition[3];
		ComputeTrajectoryPosition(newPosition, ent, arg.ServerTime, _protocol);
		const f32 ZChange = newPosition[2] - player.Position[2];
		Float3::Copy(player.Position, newPosition);

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

		if(ent->groundEntityNum != ENTITYNUM_NONE)
		{
			player.LastGroundContactTime = arg.ServerTime;
		}
	}

	// Find a player getting mid-aired.
	const s32 obituaryEvtId = parser._protocol == udtProtocol::Dm68 ? (s32)EV_OBITUARY : (s32)EV_OBITUARY_73p;
	for(u32 i = 0; i < arg.EntityCount; ++i)
	{
		if(!arg.Entities[i].IsNewEvent)
		{
			continue;
		}

		const idEntityStateBase* const ent = arg.Entities[i].Entity;
		const s32 eventType = ent->eType & (~EV_EVENT_BITS);
		if(eventType != (s32)(ET_EVENTS + obituaryEvtId))
		{
			continue;
		}

		const s32 targetIdx = ent->otherEntityNum;
		if(targetIdx < 0 || targetIdx >= MAX_CLIENTS || targetIdx == trackedPlayerIndex)
		{
			continue;
		}

		const s32 attackerIdx = ent->otherEntityNum2;
		if(attackerIdx < 0 || attackerIdx >= MAX_CLIENTS || attackerIdx != trackedPlayerIndex)
		{
			continue;
		}

		const s32 meanOfDeath = ent->eventParm;
		if(!IsAllowedMeanOfDeath(meanOfDeath, parser._protocol))
		{
			continue;
		}

		if(_players[targetIdx].LastGroundContactTime == arg.ServerTime)
		{
			continue;
		}

		const udtCutByMidAirArg& extraInfo = GetExtraInfo<udtCutByMidAirArg>();
		const s32 udtWeapon = GetUDTWeaponFromIdMod(meanOfDeath, _protocol);
		if(udtWeapon != (u32)-1 && !IsAllowedWeapon(udtWeapon, extraInfo.AllowedWeapons))
		{
			continue;
		}

		ProjectileInfo* projectile = FindBestProjectileMatch(udtWeapon, _players[targetIdx].Position, arg.ServerTime);
		if(projectile == NULL)
		{
			continue;
		}
		projectile->UsedSlot = 0;

		const u32 victimAirTimeMs = (u32)udt_max<s32>(0, arg.ServerTime - _players[targetIdx].LastZDirChangeTime);
		if(extraInfo.MinAirTimeMs > 0 && victimAirTimeMs < extraInfo.MinAirTimeMs)
		{
			continue;
		}

		const u32 projectileDistance = (u32)Float3::Dist(projectile->CreationPosition, _players[targetIdx].Position);
		if(projectileDistance < extraInfo.MinDistance)
		{
			continue;
		}

		const udtCutByPatternArg& info = PlugIn->GetInfo();
		udtCutSection cut;
		cut.GameStateIndex = _gameStateIndex;
		cut.StartTimeMs = arg.ServerTime - info.StartOffsetSec * 1000;
		cut.EndTimeMs = arg.ServerTime + info.EndOffsetSec * 1000;
		CutSections.Add(cut);
	}
}

void udtCutByMidAirAnalyzer::AddProjectile(s32 weapon, const f32* position, s32 serverTimeMs)
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

udtCutByMidAirAnalyzer::ProjectileInfo* udtCutByMidAirAnalyzer::FindBestProjectileMatch(s32 udtWeapon, const f32* targetPosition, s32 serverTimeMs)
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

		const f32 computedDist = Float3::Dist(targetPosition, info.CreationPosition);
		const f32 computedDuration = (serverTimeMs - info.CreationTimeMs) * 0.001f;
		const f32 computedSpeed = computedDist / computedDuration;
		const f32 scale = computedSpeed >= targetTravelSpeed ? (computedSpeed / targetTravelSpeed) : (targetTravelSpeed / computedSpeed);
		if(scale < 1.1f && scale < smallestScale)
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
