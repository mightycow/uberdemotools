#include "analysis_pattern_flick_rail.hpp"
#include "plug_in_pattern_search.hpp"
#include "utils.hpp"
#include "math.hpp"

#include <math.h>


udtFlickRailPatternAnalyzer::udtFlickRailPatternAnalyzer()
	: _gameStateIndex(-1)
{
}

udtFlickRailPatternAnalyzer::~udtFlickRailPatternAnalyzer()
{
}

void udtFlickRailPatternAnalyzer::StartAnalysis()
{
	_gameStateIndex = -1;
}

void udtFlickRailPatternAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/)
{
	++_gameStateIndex;
	memset(_players, 0, sizeof(_players));
}

void udtFlickRailPatternAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	const udtFlickRailPatternArg& extraInfo = GetExtraInfo<udtFlickRailPatternArg>();
	const udtPatternSearchArg& info = PlugIn->GetInfo();
	const s32 trackedPlayerIndex = PlugIn->GetTrackedPlayerIndex();

	bool trackedPlayerFound = false;
	idPlayerStateBase* const ps = GetPlayerState(arg.Snapshot, parser._inProtocol);
	if(ps != NULL && ps->clientNum == trackedPlayerIndex)
	{
		trackedPlayerFound = true;
		PlayerInfo& player = _players[ps->clientNum];
		const SnapshotInfo& prevSnapshot = player.GetMostRecentSnapshot();
		if(arg.ServerTime > prevSnapshot.ServerTimeMs)
		{
			SnapshotInfo& snapshot = player.GetWriteSnapshot();
			snapshot.ServerTimeMs = arg.ServerTime;
			snapshot.TelePortBit = ps->eFlags & EF_TELEPORT_BIT;
			Float3::Copy(snapshot.Angles, ps->viewangles);
			player.IncrementIndex();
		}
	}
	else
	{
		for(u32 i = 0; i < arg.EntityCount; ++i)
		{
			idEntityStateBase* const es = arg.Entities[i].Entity;
			if(es->eType != ET_PLAYER || es->clientNum != trackedPlayerIndex)
			{
				continue;
			}

			trackedPlayerFound = true;
			PlayerInfo& player = _players[es->clientNum];
			const SnapshotInfo& prevSnapshot = player.GetMostRecentSnapshot();
			if(arg.ServerTime > prevSnapshot.ServerTimeMs)
			{
				// @NOTE: It seems that for players, the interpolation mode of the "apos" field is always set to interpolate.
				// However, the delta seems to always be 0, so I'm only using apos.trBase.
				SnapshotInfo& snapshot = player.GetWriteSnapshot();
				snapshot.ServerTimeMs = arg.ServerTime;
				snapshot.TelePortBit = es->eFlags & EF_TELEPORT_BIT;
				Float3::Copy(snapshot.Angles, es->apos.trBase);
				player.IncrementIndex();
			}

			break;
		}
	}

	if(!trackedPlayerFound)
	{
		return;
	}

	const s32 obituaryEvtId = idEntityEvent::Obituary(parser._inProtocol);
	for(u32 i = 0; i < arg.EntityCount; ++i)
	{
		if(!arg.Entities[i].IsNewEvent)
		{
			continue;
		}

		const idEntityStateBase* const ent = arg.Entities[i].Entity;
		const s32 eventType = ent->eType & (~ID_ES_EVENT_BITS);
		if(eventType != (s32)(ET_EVENTS + obituaryEvtId))
		{
			continue;
		}

		const s32 targetIdx = ent->otherEntityNum;
		const s32 attackerIdx = ent->otherEntityNum2;
		if(targetIdx < 0 || targetIdx >= ID_MAX_CLIENTS || 
		   attackerIdx < 0 || attackerIdx >= ID_MAX_CLIENTS)
		{
			continue;
		}

		if(attackerIdx != trackedPlayerIndex || targetIdx == trackedPlayerIndex)
		{
			continue;
		}

		const s32 meanOfDeath = ent->eventParm;
		const u32 udtWeapon = GetUDTWeaponFromIdMod(meanOfDeath, parser._inProtocol);
		if(udtWeapon != udtWeapon::Railgun)
		{
			continue;
		}

		//
		// We have a rail kill from the tracked player.
		// Now let's see if it was flick or not.
		//

		PlayerInfo& player = _players[attackerIdx];
		if(!player.IsValid())
		{
			return;
		}

		const SnapshotInfo& snapNew = player.GetMostRecentSnapshot();

		f32 temp[4];
		f32 quatNew[4];
		f32 quatOld[4];
		Quat::FromEulerAnglesDeg(temp, snapNew.Angles);
		Quat::Normalize(quatNew, temp);

		f32 fastestSpeed = 0.0f;
		for(u32 j = 0; j < extraInfo.MinSpeedSnapshotCount; ++j)
		{
			const SnapshotInfo& snapOld = player.GetMostRecentSnapshot(1 + j);
			if(snapOld.TelePortBit != snapNew.TelePortBit)
			{
				break;
			}

			const s32 timeDiff = snapNew.ServerTimeMs - snapOld.ServerTimeMs;
			Quat::FromEulerAnglesDeg(temp, snapOld.Angles);
			Quat::Normalize(quatOld, temp);
			const f32 angleDiff = Quat::AngleDiff(quatNew, quatOld);
			const f32 speed = angleDiff / ((f32)timeDiff / 1000.0f);
			fastestSpeed = udt_max(fastestSpeed, speed);
		}

		if(fastestSpeed < extraInfo.MinSpeed)
		{
			return;
		}

		if(extraInfo.MinAngleDelta > 0)
		{
			// This doesn't account for the view changing direction.
			// Seems highly unlikely, but can be done by summing the deltas between consecutive snapshots.
			const SnapshotInfo& snapOld = player.GetMostRecentSnapshot(extraInfo.MinAngleDeltaSnapshotCount);
			if(snapOld.TelePortBit != snapNew.TelePortBit)
			{
				return;
			}

			Quat::FromEulerAnglesDeg(temp, snapOld.Angles);
			Quat::Normalize(quatOld, temp);
			const f32 totalAngleDiff = Quat::AngleDiff(quatNew, quatOld);
			if(totalAngleDiff < extraInfo.MinAngleDelta)
			{
				return;
			}
		}

		udtCutSection cut;
		cut.VeryShortDesc = "flickrail";
		cut.GameStateIndex = _gameStateIndex;
		cut.StartTimeMs = arg.ServerTime - info.StartOffsetSec * 1000;
		cut.EndTimeMs = arg.ServerTime + info.EndOffsetSec * 1000;
		cut.PatternTypes = UDT_BIT((u32)udtPatternType::FlickRailFrags);
		CutSections.Add(cut);
	}
}
