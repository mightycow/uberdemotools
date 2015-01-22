#include "analysis_cut_by_multi_rail.hpp"
#include "utils.hpp"


udtCutByMultiRailAnalyzer::udtCutByMultiRailAnalyzer() 
	: _gameStateIndex(-1)
{
}

udtCutByMultiRailAnalyzer::~udtCutByMultiRailAnalyzer()
{
}

void udtCutByMultiRailAnalyzer::StartAnalysis()
{
	_gameStateIndex = -1;
}

void udtCutByMultiRailAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/)
{
	++_gameStateIndex;
}

void udtCutByMultiRailAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	// @FIXME: It might happen that the obituary entity events don't all appear 
	// for the first time during the same snapshot.

	const udtCutByMultiRailArg& extraInfo = GetExtraInfo<udtCutByMultiRailArg>();
	const u32 minKillCount = extraInfo.MinKillCount;
	const s32 trackedPlayerIndex = PlugIn->GetTrackedPlayerIndex();

	u32 railKillCount = 0;
	const s32 obituaryEvtId = parser._inProtocol == udtProtocol::Dm68 ? (s32)EV_OBITUARY : (s32)EV_OBITUARY_73p;
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
		if(udtWeapon == udtWeapon::Railgun)
		{
			++railKillCount;
		}
	}

	if(railKillCount >= minKillCount)
	{
		const udtCutByPatternArg& info = PlugIn->GetInfo();

		udtCutSection cut;
		cut.VeryShortDesc = "rail";
		cut.GameStateIndex = _gameStateIndex;
		cut.StartTimeMs = arg.ServerTime - info.StartOffsetSec * 1000;
		cut.EndTimeMs = arg.ServerTime + info.EndOffsetSec * 1000;
		CutSections.Add(cut);
	}
}
