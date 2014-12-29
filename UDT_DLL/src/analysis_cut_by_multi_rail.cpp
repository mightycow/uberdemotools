#include "analysis_cut_by_multi_rail.hpp"
#include "utils.hpp"

#include <assert.h>


udtCutByMultiRailAnalyzer::udtCutByMultiRailAnalyzer()
{
	_gameStateIndex = -1;
	_recordingPlayerIndex = -1;
	_protocol = udtProtocol::Invalid;
}

void udtCutByMultiRailAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	++_gameStateIndex;
	_recordingPlayerIndex = arg.ClientNum;
	_protocol = parser._protocol;
}

void udtCutByMultiRailAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	// @FIXME: It might happen that the obituary entity events don't all appear 
	// for the first time during the same snapshot.

	assert(_info != NULL);
	assert(_extraInfo != NULL);

	udtCutByMultiRailArg* extraInfo = (udtCutByMultiRailArg*)_extraInfo;
	const u32 minKillCount = extraInfo->MinKillCount;

	// @TODO: Pass as an option?
	const s32 trackedPlayerIndex = _recordingPlayerIndex;
	// The clientNum of the player state is the index of the player we're currently spectating.
	//const s32 trackedPlayerIndex = GetPlayerState(arg.Snapshot, _protocol)->clientNum;

	u32 railKillCount = 0;
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
		const u32 udtWeapon = GetUDTWeaponFromIdMod(meanOfDeath, _protocol);
		if(udtWeapon == udtWeapon::Railgun)
		{
			++railKillCount;
		}
	}

	if(railKillCount >= minKillCount)
	{
		udtCutSection info;
		info.GameStateIndex = _gameStateIndex;
		info.StartTimeMs = arg.ServerTime - _info->StartOffsetSec * 1000;
		info.EndTimeMs = arg.ServerTime + _info->EndOffsetSec * 1000;
		CutSections.Add(info);
	}
}
