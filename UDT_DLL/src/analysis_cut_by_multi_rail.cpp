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
	for(u32 i = 0; i < arg.EntityCount; ++i)
	{
		if(!arg.Entities[i].IsNewEvent)
		{
			continue;
		}

		udtObituaryEvent eventInfo;
		if(!IsObituaryEvent(eventInfo, *arg.Entities[i].Entity, parser._inProtocol))
		{
			continue;
		}

		const s32 attackerIdx = eventInfo.AttackerIndex;
		if(attackerIdx < 0 || attackerIdx >= MAX_CLIENTS)
		{
			continue;
		}

		const s32 targetIdx = eventInfo.TargetIndex;
		if(attackerIdx != trackedPlayerIndex || targetIdx == trackedPlayerIndex)
		{
			continue;
		}
		
		// @NOTE: eventInfo.MeanOfDeath is of type udtMeanOfDeath::Id.
		const s32 idMeanOfDeath = arg.Entities[i].Entity->eventParm;
		const u32 udtWeapon = GetUDTWeaponFromIdMod(idMeanOfDeath, parser._inProtocol);
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
