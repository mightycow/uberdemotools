#include "analysis_pattern_multi_rail.hpp"
#include "plug_in_pattern_search.hpp"
#include "utils.hpp"


udtMultiRailPatternAnalyzer::udtMultiRailPatternAnalyzer() 
	: _gameStateIndex(-1)
{
}

udtMultiRailPatternAnalyzer::~udtMultiRailPatternAnalyzer()
{
}

void udtMultiRailPatternAnalyzer::StartAnalysis()
{
	_gameStateIndex = -1;
}

void udtMultiRailPatternAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/)
{
	++_gameStateIndex;
}

void udtMultiRailPatternAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	// @FIXME: It might happen that the obituary entity events don't all appear 
	// for the first time during the same snapshot.

	const udtMultiRailPatternArg& extraInfo = GetExtraInfo<udtMultiRailPatternArg>();
	const u32 minKillCount = extraInfo.MinKillCount;
	const s32 trackedPlayerIndex = PlugIn->GetTrackedPlayerIndex();

	u32 railKillCount = 0;
	for(u32 i = 0; i < arg.ChangedEntityCount; ++i)
	{
		if(!arg.ChangedEntities[i].IsNewEvent)
		{
			continue;
		}

		udtObituaryEvent eventInfo;
		if(!IsObituaryEvent(eventInfo, *arg.ChangedEntities[i].Entity, parser._inProtocol))
		{
			continue;
		}

		const s32 attackerIdx = eventInfo.AttackerIndex;
		if(attackerIdx < 0 || attackerIdx >= ID_MAX_CLIENTS)
		{
			continue;
		}

		const s32 targetIdx = eventInfo.TargetIndex;
		if(attackerIdx != trackedPlayerIndex || targetIdx == trackedPlayerIndex)
		{
			continue;
		}

		if(eventInfo.MeanOfDeath == (u32)udtMeanOfDeath::Railgun)
		{
			++railKillCount;
		}
	}

	if(railKillCount >= minKillCount)
	{
		const udtPatternSearchArg& info = PlugIn->GetInfo();

		udtCutSection cut;
		cut.VeryShortDesc = "rail";
		cut.GameStateIndex = _gameStateIndex;
		cut.StartTimeMs = arg.ServerTime - info.StartOffsetSec * 1000;
		cut.EndTimeMs = arg.ServerTime + info.EndOffsetSec * 1000;
		cut.PatternTypes = UDT_BIT((u32)udtPatternType::MultiFragRails);
		CutSections.Add(cut);
	}
}
