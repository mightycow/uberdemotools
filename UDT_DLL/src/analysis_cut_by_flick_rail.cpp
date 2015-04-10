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

		parser._context->LogInfo("t=0 angles: %f, %f, %f", snapNew.Angles[0], snapNew.Angles[1], snapNew.Angles[2]);
		parser._context->LogInfo("t=0 quat: %f, %f, %f", quatNew[0], quatNew[1], quatNew[2]);

		f32 totalAngleDiff = 0.0f;
		f32 largestAngleDiff = 0.0f;
		f32 fastestSpeed = 0.0f;
		f32 lastAngleDiff = 0.0;
		for(u32 j = 0; j < 2; ++j)
		{
			//if(arg.ServerTime == 146491) __debugbreak();
			//__debugbreak();

			SnapshotInfo& snapOld = player.GetMostRecentSnapshot(1 + j);
			const s32 timeDiff = snapNew.ServerTimeMs - snapOld.ServerTimeMs;
			if(timeDiff <= 0 || snapNew.ServerTimeMs == 0 || snapOld.ServerTimeMs == 0)
			{
				continue;
			}
	
			Quat::FromEulerAnglesDeg(temp, snapOld.Angles);
			Quat::Normalize(quatOld, temp);

			parser._context->LogInfo("t=-%u angles: %f, %f, %f", j, snapOld.Angles[0], snapOld.Angles[1], snapOld.Angles[2]);
			parser._context->LogInfo("t=-%u quat: %f, %f, %f", j, quatOld[0], quatOld[1], quatOld[2]);

			const f32 angle = Quat::Angle(quatNew, quatOld);
			const f32 angleDiff = (angle > 0.5f * UDT_PI) ? (UDT_PI - angle) : angle;

			parser._context->LogInfo("t=-%u angle: %f --- diff: %f", j, angle, angleDiff);

			const f32 speed = angleDiff / ((f32)timeDiff / 1000.0f);
			totalAngleDiff += angleDiff;
			largestAngleDiff = udt_max(largestAngleDiff, angleDiff);
			fastestSpeed = udt_max(fastestSpeed, speed);
			if(j == 0)
			{
				lastAngleDiff = angleDiff;
			}
		}
		
		//const f32 averageAngleDef = RadToDeg(totalAngleDiff / 2.0f);
		//const f32 topSpeedDegsPerSecond = RadToDeg(fastestSpeed);
		//const f32 lastAngleDiffDegs = RadToDeg(lastAngleDiff);
		/*
		parser._context->LogInfo("Total angle change: %.0f degrees", RadToDeg(totalAngleDiff));
		parser._context->LogInfo("Peak angle change: %.0f degrees", RadToDeg(largestAngleDiff));
		parser._context->LogInfo("Peak velocity: %.0f degrees/second", topSpeedDegsPerSecond);
		parser._context->LogInfo("Last angle change: %.0f degrees", lastAngleDiffDegs);
		*/

		parser._context->LogInfo("Total angle change in 2 snapshots: %.0f degrees", RadToDeg(totalAngleDiff));

		//if(topSpeedDegsPerSecond < 600.0f || lastAngleDiffDegs < 20.0f)
		//if(topSpeedDegsPerSecond < 600.0f || RadToDeg(totalAngleDiff) < 40.0f)
		//if(topSpeedDegsPerSecond < 400.0f || RadToDeg(totalAngleDiff) < 40.0f)
		//if(topSpeedDegsPerSecond < 400.0f || RadToDeg(totalAngleDiff) < 60.0f)
		if(RadToDeg(totalAngleDiff) < 45.0f)
		{
			continue;
		}
		
		/*
		PlayerInfo& player = _players[attackerIdx];
		SnapshotInfo& snapNew = player.GetMostRecentSnapshot();
		SnapshotInfo& snapOld = player.GetMostRecentSnapshot(3);
		const s32 timeDiff = snapNew.ServerTimeMs - snapOld.ServerTimeMs;
		if(timeDiff <= 0 || snapNew.ServerTimeMs == 0 || snapOld.ServerTimeMs == 0)
		{
			continue;
		}

		f32 temp[4];
		f32 q0[4];
		f32 q1[4];
		Quat::FromEulerAngles(temp, snapNew.Angles);
		Quat::Normalize(q0, temp);
		Quat::FromEulerAngles(temp, snapOld.Angles);
		Quat::Normalize(q1, temp);
		const f32 angle = Quat::Angle(q0, q1);
		const f32 angleDiff = (angle > UDT_PI) ? (2.0f * UDT_PI - angle) : angle;

		//__debugbreak();

		const f32 speed = angleDiff / ((f32)timeDiff / 1000.0f);
		parser._context->LogInfo("Angle change: %.0f degrees", RadToDeg(angleDiff));
		parser._context->LogInfo("Time diff: %d milli-seconds", timeDiff);
		parser._context->LogInfo("Angular velocity: %.0f degrees/second", RadToDeg(speed));

		//snap1.Angles

		// @TODO:
		//const f32 minSpeed = (f32)UDT_PI;
		//const f32 minAngle = (f32)UDT_PI;
		*/
		udtCutSection cut;
		cut.VeryShortDesc = "flickrail";
		cut.GameStateIndex = _gameStateIndex;
		cut.StartTimeMs = arg.ServerTime - info.StartOffsetSec * 1000;
		cut.EndTimeMs = arg.ServerTime + info.EndOffsetSec * 1000;
		CutSections.Add(cut);
	}
}
