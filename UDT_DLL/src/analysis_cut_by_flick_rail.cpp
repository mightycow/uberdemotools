#include "analysis_cut_by_flick_rail.hpp"
#include "utils.hpp"
#include "math.hpp"

#include <math.h>


udtCutByFlickRailAnalyzer::udtCutByFlickRailAnalyzer()
	: _gameStateIndex(-1)
{
}

udtCutByFlickRailAnalyzer::~udtCutByFlickRailAnalyzer()
{
}

void udtCutByFlickRailAnalyzer::StartAnalysis()
{
	_gameStateIndex = -1;
}

void udtCutByFlickRailAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/)
{
	++_gameStateIndex;
	memset(_players, 0, sizeof(_players));
}

void udtCutByFlickRailAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	//const udtCutByFlickRailArg& extraInfo = GetExtraInfo<udtCutByFlickRailArg>();
	const udtCutByPatternArg& info = PlugIn->GetInfo();
	const s32 trackedPlayerIndex = PlugIn->GetTrackedPlayerIndex();

	idPlayerStateBase* const ps = GetPlayerState(arg.Snapshot, parser._inProtocol);
	if(ps != NULL && ps->clientNum >= 0 && ps->clientNum < 64)
	{
		PlayerInfo& player = _players[ps->clientNum];
		SnapshotInfo& prevSnapshot = player.GetMostRecentSnapshot();
		if(arg.ServerTime > prevSnapshot.ServerTimeMs)
		{
			SnapshotInfo& snapshot = player.GetWriteSnapshot();
			snapshot.ServerTimeMs = arg.ServerTime;
			Float3::Copy(snapshot.Angles, ps->viewangles);
			player.IncrementIndex();
		}
	}

	const s32 obituaryEvtId = parser._inProtocol == udtProtocol::Dm68 ? (s32)EV_OBITUARY_68 : (s32)EV_OBITUARY_73p;
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
		const s32 attackerIdx = ent->otherEntityNum2;
		if(targetIdx < 0 || targetIdx >= MAX_CLIENTS || 
		   attackerIdx < 0 || attackerIdx >= MAX_CLIENTS)
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
		SnapshotInfo& snapNew = player.GetMostRecentSnapshot();

		f32 temp[4];
		f32 quatNew[4];
		f32 quatOld[4];
		Quat::FromEulerAnglesDeg(temp, snapNew.Angles);
		Quat::Normalize(quatNew, temp);

		f32 totalAngleDiff = 0.0f;
		f32 largestAngleDiff = 0.0f;
		f32 fastestSpeed = 0.0f;
		f32 lastAngleDiff = 0.0;
		const u32 SnapshotCount = 2;
		for(u32 j = 0; j < SnapshotCount; ++j)
		{
			SnapshotInfo& snapOld = player.GetMostRecentSnapshot(1 + j);
			const s32 timeDiff = snapNew.ServerTimeMs - snapOld.ServerTimeMs;
			if(timeDiff <= 0 || snapNew.ServerTimeMs == 0 || snapOld.ServerTimeMs == 0)
			{
				continue;
			}
	
			Quat::FromEulerAnglesDeg(temp, snapOld.Angles);
			Quat::Normalize(quatOld, temp);

			const f32 angle = Quat::Angle(quatNew, quatOld);
			const f32 angleDiff = fabsf((angle > UDT_PI) ? (2.0f * UDT_PI - angle) : angle);
			const f32 speed = angleDiff / ((f32)timeDiff / 1000.0f);
			totalAngleDiff += angleDiff;
			largestAngleDiff = udt_max(largestAngleDiff, angleDiff);
			fastestSpeed = udt_max(fastestSpeed, speed);
			if(j == 0)
			{
				lastAngleDiff = angleDiff;
			}
		}

		// @TODO: Implement usage of the udtCutByFlickRailArg parameters.
		if(RadToDeg(fastestSpeed) < 800.0f)
		{
			continue;
		}

		udtCutSection cut;
		cut.VeryShortDesc = "flickrail";
		cut.GameStateIndex = _gameStateIndex;
		cut.StartTimeMs = arg.ServerTime - info.StartOffsetSec * 1000;
		cut.EndTimeMs = arg.ServerTime + info.EndOffsetSec * 1000;
		CutSections.Add(cut);
	}
}
