#include "analysis_cut_by_flag.hpp"
#include "utils.hpp"


/*
To detect other players taking a flag, we could use the "global item pickup" event.
- Value: EV_GLOBAL_ITEM_PICKUP is one of EV_GLOBAL_ITEM_PICKUP_68, EV_GLOBAL_ITEM_PICKUP_73p
- Encoded as: eType = ET_EVENTS + EV_GLOBAL_ITEM_PICKUP
- The "eventParm" entity field is the model index of the flag
It seems that there is no robust way of knowing who took the flag with it.
Therefore, it's better to use the powerups entity field to find pickup times.

For captures, we don't have the persistant field in entities. Maybe the EF_AWARD_CAP can help?
*/


static idEntityStateBase* FindPlayerEntity(const udtSnapshotCallbackArg& arg, s32 trackedPlayerIdx)
{
	for(u32 i = 0, count = arg.EntityCount; i < count; ++i)
	{
		idEntityStateBase* const entity = arg.Entities[i].Entity;
		if(entity != NULL && entity->clientNum == trackedPlayerIdx)
		{
			return entity;
		}
	}

	return NULL;
}


udtCutByFlagCaptureAnalyzer::udtCutByFlagCaptureAnalyzer()
{
	StartAnalysis();
}

udtCutByFlagCaptureAnalyzer::~udtCutByFlagCaptureAnalyzer()
{
}

void udtCutByFlagCaptureAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg&, udtBaseParser&)
{
	++_gameStateIndex;
	_pickupTimeMs = S32_MIN;
	_previousCaptureCount = 0;
	_previousCapped = false;
}

void udtCutByFlagCaptureAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	const s32 trackedPlayerIdx = PlugIn->GetTrackedPlayerIndex();
	idPlayerStateBase* const ps = GetPlayerState(arg.Snapshot, parser._inProtocol);

	bool hasFlag = false;
	bool capped = _previousCapped;
	s32 captureCount = _previousCaptureCount;
	bool justCapped = false;

	if(ps->clientNum == trackedPlayerIdx)
	{
		hasFlag =
			ps->powerups[PW_REDFLAG] == S32_MAX ||
			ps->powerups[PW_BLUEFLAG] == S32_MAX ||
			ps->powerups[PW_NEUTRALFLAG] == S32_MAX;
		const s32 captureCountPersIdx = (parser._inProtocol) == udtProtocol::Dm68 ? (s32)PERS_CAPTURES_68 : (s32)PERS_CAPTURES_73p;
		captureCount = ps->persistant[captureCountPersIdx];
		justCapped = captureCount > _previousCaptureCount;
	}
	else
	{
		idEntityStateBase* const es = FindPlayerEntity(arg, trackedPlayerIdx);
		if(es == NULL)
		{
			return;
		}

		hasFlag =
			(es->powerups & (1 << PW_REDFLAG)) != 0 ||
			(es->powerups & (1 << PW_BLUEFLAG)) != 0 ||
			(es->powerups & (1 << PW_NEUTRALFLAG)) != 0;
		capped = (es->eFlags & EF_AWARD_CAP) != 0;
		justCapped = !_previousCapped && capped;
	}

	if(_pickupTimeMs == S32_MIN && hasFlag)
	{
		_pickupTimeMs = arg.ServerTime;
	}

	const u32 carryTimeMs = (u32)(arg.ServerTime - _pickupTimeMs);
	const udtCutByFlagCaptureArg& extraInfo = GetExtraInfo<udtCutByFlagCaptureArg>();
	if(_pickupTimeMs != S32_MIN &&
	   justCapped &&
	   carryTimeMs >= extraInfo.MinCarryTimeMs &&
	   carryTimeMs <= extraInfo.MaxCarryTimeMs)
	{
		const udtCutByPatternArg& info = PlugIn->GetInfo();

		udtCutSection cut;
		cut.VeryShortDesc = "flag";
		cut.GameStateIndex = _gameStateIndex;
		cut.StartTimeMs = _pickupTimeMs - info.StartOffsetSec * 1000;
		cut.EndTimeMs = arg.ServerTime + info.EndOffsetSec * 1000;
		CutSections.Add(cut);

		_pickupTimeMs = S32_MIN;
	}

	_previousCapped = capped;
	_previousCaptureCount = captureCount;

	if(!hasFlag)
	{
		_pickupTimeMs = S32_MIN;
	}
}

void udtCutByFlagCaptureAnalyzer::StartAnalysis()
{
	_gameStateIndex = -1;
	_pickupTimeMs = S32_MIN;
	_previousCaptureCount = 0;
	_previousCapped = false;
}
