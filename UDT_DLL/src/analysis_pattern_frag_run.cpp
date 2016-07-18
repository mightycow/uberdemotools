#include "analysis_pattern_frag_run.hpp"
#include "plug_in_pattern_search.hpp"
#include "utils.hpp"


static bool AreTeammates(s32 team1, s32 team2)
{
	return
		(team1 == team2) &&
		(team1 >= 0) &&
		(team1 < (s32)udtTeam::Count) &&
		(team1 == (s32)udtTeam::Red || (s32)team1 == udtTeam::Blue);
}

static bool GetUDTPlayerModFromUDTMod(u32& playerMod, u32 mod)
{
	switch((udtMeanOfDeath::Id)mod)
	{
		case udtMeanOfDeath::Shotgun: playerMod = (u32)udtPlayerMeanOfDeath::Shotgun; break;
		case udtMeanOfDeath::Gauntlet: playerMod = (u32)udtPlayerMeanOfDeath::Gauntlet; break;
		case udtMeanOfDeath::MachineGun: playerMod = (u32)udtPlayerMeanOfDeath::MachineGun; break;
		case udtMeanOfDeath::Grenade: playerMod = (u32)udtPlayerMeanOfDeath::Grenade; break;
		case udtMeanOfDeath::GrenadeSplash: playerMod = (u32)udtPlayerMeanOfDeath::GrenadeSplash; break;
		case udtMeanOfDeath::Rocket: playerMod = (u32)udtPlayerMeanOfDeath::Rocket; break;
		case udtMeanOfDeath::RocketSplash: playerMod = (u32)udtPlayerMeanOfDeath::RocketSplash; break;
		case udtMeanOfDeath::Plasma: playerMod = (u32)udtPlayerMeanOfDeath::Plasma; break;
		case udtMeanOfDeath::PlasmaSplash: playerMod = (u32)udtPlayerMeanOfDeath::PlasmaSplash; break;
		case udtMeanOfDeath::Railgun: playerMod = (u32)udtPlayerMeanOfDeath::Railgun; break;
		case udtMeanOfDeath::Lightning: playerMod = (u32)udtPlayerMeanOfDeath::Lightning; break;
		case udtMeanOfDeath::BFG: playerMod = (u32)udtPlayerMeanOfDeath::BFG; break;
		case udtMeanOfDeath::BFGSplash: playerMod = (u32)udtPlayerMeanOfDeath::BFGSplash; break;
		case udtMeanOfDeath::TeleFrag: playerMod = (u32)udtPlayerMeanOfDeath::TeleFrag; break;
		case udtMeanOfDeath::NailGun: playerMod = (u32)udtPlayerMeanOfDeath::NailGun; break;
		case udtMeanOfDeath::ChainGun: playerMod = (u32)udtPlayerMeanOfDeath::ChainGun; break;
		case udtMeanOfDeath::ProximityMine: playerMod = (u32)udtPlayerMeanOfDeath::ProximityMine; break;
		case udtMeanOfDeath::Kamikaze: playerMod = (u32)udtPlayerMeanOfDeath::Kamikaze; break;
		case udtMeanOfDeath::Grapple: playerMod = (u32)udtPlayerMeanOfDeath::Grapple; break;
		case udtMeanOfDeath::Thaw: playerMod = (u32)udtPlayerMeanOfDeath::Thaw; break;
		case udtMeanOfDeath::HeavyMachineGun: playerMod = (u32)udtPlayerMeanOfDeath::HeavyMachineGun; break;
		default: return false;
	}

	return true;
}

static bool IsAllowedUDTMeanOfDeath(s32 udtMod, u32 udtPlayerMODFlags)
{
	u32 playerModBit;
	if(!GetUDTPlayerModFromUDTMod(playerModBit, udtMod))
	{
		return false;
	}

	const u32 playerModFlag = 1 << playerModBit;

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
