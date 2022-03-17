#include "analysis_pattern_frag_run.hpp"
#include "plug_in_pattern_search.hpp"
#include "utils.hpp"


#define PlayerKill(Bit) Bit,
#define WorldKill(Bit)
#define UDT_PLAYER_MOD_ITEM(Enum, Desc, KillType, Bit) KillType(Bit)
static const u32 PlayerMeanOfDeathToMeanOfDeath[] =
{
	UDT_MEAN_OF_DEATH_LIST(UDT_PLAYER_MOD_ITEM)
	0
};
#undef UDT_PLAYER_MOD_ITEM
#undef WorldKill
#undef PlayerKill
static_assert(UDT_ARRAY_LENGTH(PlayerMeanOfDeathToMeanOfDeath) == udtPlayerMeanOfDeath::Count + 1, "Invalid array size");


static bool AreTeammates(s32 team1, s32 team2)
{
	return
		(team1 == team2) &&
		(team1 >= 0) &&
		(team1 < (s32)udtTeam::Count) &&
		(team1 == (s32)udtTeam::Red || (s32)team1 == udtTeam::Blue);
}

static bool IsAllowedUDTMeanOfDeath(s32 udtMod, u64 udtPlayerMODFlags)
{
	u64 playerModBit = udtPlayerMeanOfDeath::Count;
	for(u32 i = 0; i < udtPlayerMeanOfDeath::Count; ++i)
	{
		if(PlayerMeanOfDeathToMeanOfDeath[i] == (u32)udtMod)
		{
			playerModBit = i;
		}
	}
	if(playerModBit == udtPlayerMeanOfDeath::Count)
	{
		return false;
	}

	const u64 playerModFlag = (u64)1 << playerModBit;

	return (udtPlayerMODFlags & playerModFlag) != 0;
}


udtFragRunPatternAnalyzer::udtFragRunPatternAnalyzer()
{
	_analyzer.SetNameAllocationEnabled(false);
}

udtFragRunPatternAnalyzer::~udtFragRunPatternAnalyzer()
{
}

void udtFragRunPatternAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessGamestateMessage(arg, parser);
}

void udtFragRunPatternAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessCommandMessage(arg, parser);
}

void udtFragRunPatternAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessSnapshotMessage(arg, parser);
	const u32 obituaryCount = _analyzer.Obituaries.GetSize();
	if(obituaryCount == 0)
	{
		return;
	}

	const udtFragRunPatternArg& extraInfo = GetExtraInfo<udtFragRunPatternArg>();
	const s32 maxIntervalMs = extraInfo.TimeBetweenFragsSec * 1000;
	const s32 playerIndex = PlugIn->GetTrackedPlayerIndex();
	const bool allowSelfKills = (extraInfo.Flags & (u32)udtFragRunPatternArgMask::AllowSelfKills) != 0;
	const bool allowTeamKills = (extraInfo.Flags & (u32)udtFragRunPatternArgMask::AllowTeamKills) != 0;
	const bool allowAnyDeath = (extraInfo.Flags & (u32)udtFragRunPatternArgMask::AllowDeaths) != 0;

	for(u32 i = 0; i < obituaryCount; ++i)
	{
		const udtParseDataObituary& data = _analyzer.Obituaries[i];

		// Got killed?
		if(data.TargetIdx == playerIndex)
		{
			if(!allowAnyDeath || (!allowSelfKills && data.AttackerIdx == data.TargetIdx))
			{
				AddCurrentSectionIfValid();
			}
			continue;
		}

		// Someone else did the kill?
		if(data.AttackerIdx != playerIndex)
		{
			continue;
		}

		// We killed someone we shouldn't have?
		if(AreTeammates(data.TargetTeamIdx, data.AttackerTeamIdx))
		{
			if(!allowTeamKills)
			{
				AddCurrentSectionIfValid();
			}
			continue;
		}

		// Did we use a weapon that's not allowed?
		if(!IsAllowedUDTMeanOfDeath(data.MeanOfDeath, extraInfo.AllowedMeansOfDeaths))
		{
			AddCurrentSectionIfValid();
			continue;
		}

		//
		// We passed all the filters, so we got a match.
		//

		if(_frags.IsEmpty())
		{
			AddMatch(data);
		}
		else
		{
			const Frag previousMatch = _frags[_frags.GetSize() - 1];
			if(data.GameStateIndex != previousMatch.GameStateIndex ||
			   data.ServerTimeMs > previousMatch.ServerTimeMs + maxIntervalMs)
			{
				AddCurrentSectionIfValid();
			}

			AddMatch(data);
		}
	}

	_analyzer.Obituaries.Clear();
}

void udtFragRunPatternAnalyzer::InitAllocators(u32 demoCount)
{
	_analyzer.InitAllocators(demoCount, PlugIn->GetTempAllocator());
}

void udtFragRunPatternAnalyzer::StartAnalysis()
{
	_frags.Clear();
}

void udtFragRunPatternAnalyzer::FinishAnalysis()
{
	AddCurrentSectionIfValid();
}

void udtFragRunPatternAnalyzer::AddCurrentSectionIfValid()
{
	const udtPatternSearchArg& info = PlugIn->GetInfo();
	const udtFragRunPatternArg& extraInfo = GetExtraInfo<udtFragRunPatternArg>();

	const u32 fragCount = _frags.GetSize();
	if(fragCount < 2 || fragCount < extraInfo.MinFragCount)
	{
		_frags.Clear();
		return;
	}

	udtCutSection cut;
	cut.VeryShortDesc = "frag";
	cut.GameStateIndex = _frags[0].GameStateIndex;
	cut.StartTimeMs = _frags[0].ServerTimeMs - (s32)(info.StartOffsetSec * 1000);
	cut.EndTimeMs = _frags[fragCount - 1].ServerTimeMs + (s32)(info.EndOffsetSec * 1000);
	cut.PatternTypes = UDT_BIT((u32)udtPatternType::FragSequences);
	CutSections.Add(cut);

	_frags.Clear();
}

void udtFragRunPatternAnalyzer::AddMatch(const udtParseDataObituary& data)
{
	Frag match;
	match.ServerTimeMs = data.ServerTimeMs;
	match.GameStateIndex = data.GameStateIndex;
	_frags.Add(match);
}
