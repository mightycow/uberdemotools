#include "analysis_mid_air.hpp"
#include "utils.hpp"
#include "math.hpp"

#include <string.h>
#include <math.h>


static void ComputeTrajectoryPosition(f32* result, const idTrajectoryBase& tr, s32 atTime)
{
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
				const f32 deltaTime = (atTime - tr.trTime) * 0.001f;
				Float3::Mad(result, tr.trBase, tr.trDelta, deltaTime);
				result[2] -= 0.5f * 800.0f * deltaTime * deltaTime;
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


udtMidAirAnalyzer::udtMidAirAnalyzer()
{
	_protocol = udtProtocol::Invalid;
	_lastEventSequence = S32_MIN;
	_gameStateIndex = -1;
	RecordingPlayerIndex = -1;
	memset(_projectiles, 0, sizeof(_projectiles));
	memset(_players, 0, sizeof(_players));
}

void udtMidAirAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{	   
	const s32 TrackedPlayerIndex = RecordingPlayerIndex;

	// Store projectiles fired by the tracked player.
	idPlayerStateBase* const ps = GetPlayerState(arg.Snapshot, _protocol);
	if(ps != NULL)
	{
		s32 event = 0;
		s32 eventParm = 0;
		GetEntityStateEventFromPlayerState(event, eventParm, ps);
		const s32 eventType = event & (~EV_EVENT_BITS);
		if(ps->eventSequence != _lastEventSequence &&
		   eventType == EV_FIRE_WEAPON &&
		   ps->clientNum == TrackedPlayerIndex &&
		   IsAllowedWeapon(ps->weapon, _protocol))
		{
			AddProjectile(ps->weapon, ps->origin, arg.ServerTime);
		}
	}

	// Update the data of the player recording the demo.
	if(ps != NULL)
	{
		Float3::Copy(_players[ps->clientNum].Position, ps->origin);
		if(ps->groundEntityNum != ENTITYNUM_NONE)
		{
			_players[ps->clientNum].LastGroundContactTime = arg.ServerTime;
		}
	}

	// Update the data of all the other player.
	for(u32 i = 0; i < arg.EntityCount; ++i)
	{
		const idEntityStateBase* const ent = arg.Entities[i].Entity;
		if(ent->eType == ET_PLAYER && ent->clientNum >= 0 && ent->clientNum < 64)
		{
			PlayerInfo& player = _players[ent->clientNum];

			const f32 ZDirDelta = 1.0f;
			const s32 oldZDir = player.ZDir;

			f32 newPosition[3];
			ComputeTrajectoryPosition(newPosition, ent->pos, arg.ServerTime);
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
	}

	// Find a player getting mid-aired.
	const s32 obituaryEvtId = parser._protocol == udtProtocol::Dm68 ? (s32)EV_OBITUARY : (s32)EV_OBITUARY_73;
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
		if(targetIdx < 0 || targetIdx >= MAX_CLIENTS)
		{
			continue;
		}

		const s32 attackerIdx = ent->otherEntityNum2;
		if(attackerIdx < 0 || attackerIdx >= MAX_CLIENTS || attackerIdx != TrackedPlayerIndex)
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

		const s32 udtWeapon = GetUDTWeaponFromIdMod(meanOfDeath, _protocol);
		ProjectileInfo* projectile = FindBestProjectileMatch(udtWeapon, _players[targetIdx].Position, arg.ServerTime);
		if(projectile != NULL)
		{
			projectile->UsedSlot = 0;
		}

		udtParseDataMidAir info = { 0 };
		info.GameStateIndex = _gameStateIndex;
		info.ServerTimeMs = arg.ServerTime;
		info.AttackerIdx = (u32)attackerIdx;
		info.Weapon = (u32)udtWeapon;
		info.TravelInfoAvailable = 0;
		info.VictimAirTimeMs = (u32)udt_max<s32>(0, arg.ServerTime - _players[targetIdx].LastZDirChangeTime);

		if(projectile != NULL)
		{
			const s32 duration = arg.ServerTime - projectile->CreationTimeMs;
			info.TravelInfoAvailable = 1;
			info.TravelDistance = (u32)Float3::Dist(projectile->CreationPosition, _players[targetIdx].Position);
			info.TravelDurationMs = duration > 0 ? (u32)duration : 0;
		}

		MidAirs.Add(info);
	}

	if(ps != NULL)
	{
		_lastEventSequence = ps->eventSequence;
	}
}

void udtMidAirAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	_protocol = parser._protocol;

	RecordingPlayerIndex = arg.ClientNum;

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

void udtMidAirAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/)
{
}

void udtMidAirAnalyzer::AddProjectile(s32 weapon, const f32* position, s32 serverTimeMs)
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

udtMidAirAnalyzer::ProjectileInfo* udtMidAirAnalyzer::FindBestProjectileMatch(s32 udtWeapon, const f32* targetPosition, s32 serverTimeMs)
{
	// CPMA: plasma/bfg 2000 rockets 1000 ups
	// VQ3: plasma/bfg 2000 rockets 900 ups
	// VQL: plasma/bfg ??? rockets ??? ups
	// PQL: plasma/bfg ??? rockets ??? ups

	// @FIXME: CPMA values.
	f32 TargetTravelSpeed = 1000.0f;
	if(udtWeapon == (s32)udtWeapon::BFG)
	{
		TargetTravelSpeed = 2000.0f;
	}

	f32 smallestDiff = 9999.0f;
	s32 smallestDiffIndex = -1;
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
		const f32 diff = fabsf(computedSpeed - TargetTravelSpeed);
		if(diff < smallestDiff && computedSpeed > 0.9f * TargetTravelSpeed)
		{
			smallestDiff = diff;
			smallestDiffIndex = i;
		}
	}

	if(smallestDiffIndex >= 0 && smallestDiffIndex < (s32)UDT_COUNT_OF(_projectiles))
	{
		return &_projectiles[smallestDiffIndex];
	}

	return NULL;
}
