#include "converter_entity_timer_shifter.hpp"
#include "utils.hpp"


udtdEntityTimeShifterPlugIn::udtdEntityTimeShifterPlugIn()
{
}

udtdEntityTimeShifterPlugIn::~udtdEntityTimeShifterPlugIn()
{
}

void udtdEntityTimeShifterPlugIn::ResetForNextDemo(const udtTimeShiftArg& timeShiftArg)
{
	memset(_backupSnaps, 0, sizeof(_backupSnaps));
	_backupSnapIndex = 0;
	_parsedSnapIndex = 0;
	_snapshotDuration = 1000 / 30; // CPMA default: 30 Hz.
	_delaySnapshotCount = udt_clamp(timeShiftArg.SnapshotCount, 1, (s32)MaxSnapshotCount);
	_protocol = udtProtocol::Invalid;
}

void udtdEntityTimeShifterPlugIn::InitPlugIn(udtProtocol::Id protocol)
{
	_protocol = protocol;
	if(protocol != udtProtocol::Dm68)
	{
		_snapshotDuration = 1000 / 40; // Quake Live runs at 40 Hz.
	}
}

void udtdEntityTimeShifterPlugIn::ModifySnapshot(udtdSnapshotData& curSnap, udtdSnapshotData& oldSnap)
{
	const s32 backupSnapshotCount = _delaySnapshotCount + 1;
	if(_parsedSnapIndex < backupSnapshotCount)
	{
		CopySnapshot(_backupSnaps[_parsedSnapIndex], curSnap);
	}
	else
	{
		CopySnapshot(_newOldSnap, _backupSnaps[_backupSnapIndex]);
		CopySnapshot(_newCurSnap, _backupSnaps[(_backupSnapIndex + 1) % backupSnapshotCount]);
		CopySnapshot(_backupSnaps[_backupSnapIndex], curSnap);
		FixSnapshot(curSnap, _newCurSnap);
		FixSnapshot(oldSnap, _newOldSnap);
		_backupSnapIndex = (_backupSnapIndex + 1) % backupSnapshotCount;
	}

	++_parsedSnapIndex;
}

void udtdEntityTimeShifterPlugIn::AnalyzeConfigString(s32 index, const char* configString, u32 /*stringLength*/)
{
	if(_protocol != udtProtocol::Dm68)
	{
		return;
	}

	if(index == CS_SERVERINFO)
	{
		s32 snaps = 0;
		if(ParseConfigStringValueInt(snaps, _tempAllocator, "sv_fps", configString) && 
		   snaps > 0)
		{
			_snapshotDuration = 1000 / snaps;
		}

		_tempAllocator.Clear();
	}
}

void udtdEntityTimeShifterPlugIn::CopySnapshot(udtdSnapshotData& dest, const udtdSnapshotData& source)
{
	memcpy(&dest, &source, sizeof(udtdSnapshotData));
}

void udtdEntityTimeShifterPlugIn::FixSnapshot(udtdSnapshotData& dest, const udtdSnapshotData& source)
{
	const s32 entityTypePlayerId = GetIdNumber(udtMagicNumberType::EntityType, udtEntityType::Player, _protocol);
	const s32 entityFlagDead = GetIdEntityStateFlagMask(udtEntityFlag::Dead, _protocol);
	const s32 entityFlagTeleportBit = GetIdEntityStateFlagMask(udtEntityFlag::TeleportBit, _protocol);
	for(s32 i = 0; i < MAX_GENTITIES; ++i)
	{
		const udtdClientEntity& sourceEntity = source.Entities[i];
		udtdClientEntity& destEntity = dest.Entities[i];
		if(sourceEntity.Valid &&
		   sourceEntity.EntityState.eType == entityTypePlayerId &&
		   (sourceEntity.EntityState.eFlags & entityFlagDead) == 0 &&
		   sourceEntity.EntityState.clientNum == destEntity.EntityState.clientNum)
		{
			destEntity.Valid = true;

			destEntity.EntityState.pos.trTime += _delaySnapshotCount * _snapshotDuration;
			for(s32 j = 0; j < 3; ++j)
			{
				destEntity.EntityState.pos.trBase[j] = sourceEntity.EntityState.pos.trBase[j];
				destEntity.EntityState.pos.trDelta[j] = sourceEntity.EntityState.pos.trDelta[j];
			}

			if(sourceEntity.EntityState.eFlags & entityFlagTeleportBit)
			{
				destEntity.EntityState.eFlags |= entityFlagTeleportBit;
			}
			else
			{
				destEntity.EntityState.eFlags &= ~entityFlagTeleportBit;
			}
		}
	}
}
