#include "analysis_mid_air.hpp"
#include "utils.hpp"
#include "math.hpp"

#include <string.h>


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


udtMidAirAnalyzer::udtMidAirAnalyzer()
{
	_protocol = udtProtocol::Invalid;
	_gameStateIndex = -1;
	RecordingPlayerIndex = -1;
	memset(_projectiles, 0, sizeof(_projectiles));
	memset(_playerEntities, 0, sizeof(_playerEntities));
}

void udtMidAirAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{	
	// Update our client number to entity look-up table.
	for(u32 i = 0; i < arg.EntityCount; ++i)
	{
		const idEntityStateBase* const ent = arg.Entities[i].Entity;
		if(ent->eType == ET_PLAYER && ent->clientNum >= 0 && ent->clientNum < 64)
		{
			_playerEntities[ent->clientNum] = ent;
		}
	}
	
	// Add/change projectiles.
	for(u32 i = 0; i < arg.EntityCount; ++i)
	{
		const idEntityStateBase* const ent = arg.Entities[i].Entity;
		if(ent->eType == ET_MISSILE)
		{
			AddOrUpdateProjectile(ent, arg.ServerTime);
		}
	}

	// Find the projectile that exploded.
	s32 projectileEntityNumber = -1;
	for(u32 i = 0; i < arg.EntityCount; ++i)
	{
		const idEntityStateBase* const ent = arg.Entities[i].Entity;
		if(ent->eType == ET_GENERAL && IsAllowedWeapon(ent->weapon, parser._protocol))
		{
			projectileEntityNumber = ent->number;
			break;
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
		if(attackerIdx < 0 || attackerIdx >= MAX_CLIENTS)
		{
			continue;
		}

		const s32 meanOfDeath = ent->eventParm;
		if(!IsAllowedMeanOfDeath(meanOfDeath, parser._protocol))
		{
			continue;
		}

		const idEntityStateBase* const targetEnt = _playerEntities[targetIdx];
		if(targetEnt == NULL || targetEnt->eType != ET_PLAYER || targetEnt->groundEntityNum != ENTITYNUM_NONE)
		{
			continue;
		}

		ProjectileInfo* projectile = NULL;
		if(projectileEntityNumber != -1)
		{
			for(u32 i = 0; i < UDT_COUNT_OF(_projectiles); ++i)
			{
				if(_projectiles[i].UsedSlot && _projectiles[i].IdEntityNumber == projectileEntityNumber)
				{
					projectile = &_projectiles[i];
					projectile->UsedSlot = 0;
					break;
				}
			}
		}

		udtParseDataMidAir info = { 0 };
		info.GameStateIndex = _gameStateIndex;
		info.ServerTimeMs = arg.ServerTime;
		info.AttackerIdx = (u32)attackerIdx;
		info.Weapon = (u32)GetUDTWeaponFromIdMod(meanOfDeath, _protocol);
		info.TravelInfoAvailable = 0;
		if(projectile != NULL)
		{
			const s32 duration = arg.ServerTime - projectile->CreationTimeMs;
			info.TravelInfoAvailable = 1;
			info.TravelDistance = (u32)Float3::Dist(projectile->CreationPosition, targetEnt->pos.trBase);
			info.TravelDurationMs = duration > 0 ? (u32)duration : 0;
		}
		MidAirs.Add(info);
	}

	// Remove projectiles.
	for(u32 i = 0; i < arg.RemovedEntityCount; ++i)
	{
		const idEntityStateBase* const ent = arg.RemovedEntities[i];
		if(ent->eType == ET_MISSILE)
		{
			RemoveProjectile(ent->number);
		}
	}
}

void udtMidAirAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	_protocol = parser._protocol;

	RecordingPlayerIndex = arg.ClientNum;

	++_gameStateIndex;
}

void udtMidAirAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/)
{
}

void udtMidAirAnalyzer::AddOrUpdateProjectile(const idEntityStateBase* entity, s32 /*serverTimeMs*/)
{
	// Update if existing.
	for(u32 i = 0; i < UDT_COUNT_OF(_projectiles); ++i)
	{
		if(_projectiles[i].UsedSlot && _projectiles[i].IdEntityNumber == entity->number)
		{
			ProjectileInfo* const projectile = &_projectiles[i];
			projectile->CreationPosition[0] = entity->pos.trBase[0];
			projectile->CreationPosition[1] = entity->pos.trBase[1];
			projectile->CreationPosition[2] = entity->pos.trBase[2];
			projectile->CreationTimeMs = entity->pos.trTime;
			projectile->IdWeapon = entity->weapon;
			return;
		}
	}
	
	// Add if not existing.
	for(u32 i = 0; i < UDT_COUNT_OF(_projectiles); ++i)
	{
		if(!_projectiles[i].UsedSlot)
		{
			ProjectileInfo* const projectile = &_projectiles[i];
			projectile->UsedSlot = 1;
			projectile->IdEntityNumber = entity->number;
			projectile->CreationPosition[0] = entity->pos.trBase[0];
			projectile->CreationPosition[1] = entity->pos.trBase[1];
			projectile->CreationPosition[2] = entity->pos.trBase[2];
			projectile->CreationTimeMs = entity->pos.trTime;
			projectile->IdWeapon = entity->weapon;
			return;
		}
	}
}

void udtMidAirAnalyzer::RemoveProjectile(s32 entityNumber)
{
	for(u32 i = 0; i < UDT_COUNT_OF(_projectiles); ++i)
	{
		if(_projectiles[i].UsedSlot && _projectiles[i].IdEntityNumber == entityNumber)
		{
			_projectiles[i].UsedSlot = 0;
			return;
		}
	}
}
