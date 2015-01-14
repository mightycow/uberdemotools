#include "analysis_cut_by_frag.hpp"
#include "utils.hpp"


static bool AreTeammates(s32 team1, s32 team2)
{
	return
		(team1 == team2) &&
		(team1 >= 0) &&
		(team1 < (s32)udtTeam::Count) &&
		(team1 == (s32)udtTeam::Red || (s32)team1 == udtTeam::Blue);
}

static bool IsAllowedMeanOfDeath(s32 idMOD, u32 udtPlayerMODFlags, udtProtocol::Id procotol)
{
	if(procotol <= (s32)udtProtocol::Invalid || procotol >= (s32)udtProtocol::Count)
	{
		return true;
	}

	const s32 bit = GetUDTPlayerMODBitFromIdMod(idMOD, procotol);

	return (udtPlayerMODFlags & (u32)bit) != 0;
}


udtCutByFragAnalyzer::udtCutByFragAnalyzer() 
	: _frags(1 << 16)
{
	_analyzer.SetNameAllocationEnabled(false);
}

udtCutByFragAnalyzer::~udtCutByFragAnalyzer()
{
}

void udtCutByFragAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessGamestateMessage(arg, parser);
}

void udtCutByFragAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessCommandMessage(arg, parser);
}

void udtCutByFragAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessSnapshotMessage(arg, parser);
	const u32 obituaryCount = _analyzer.Obituaries.GetSize();
	if(obituaryCount == 0)
	{
		return;
	}

	const udtCutByFragArg& extraInfo = GetExtraInfo<udtCutByFragArg>();
	const s32 maxIntervalMs = extraInfo.TimeBetweenFragsSec * 1000;
	const s32 playerIndex = PlugIn->GetTrackedPlayerIndex();
	const bool allowSelfKills = (extraInfo.Flags & (u32)udtCutByFragArgFlags::AllowSelfKills) != 0;
	const bool allowTeamKills = (extraInfo.Flags & (u32)udtCutByFragArgFlags::AllowTeamKills) != 0;
	const bool allowAnyDeath = (extraInfo.Flags & (u32)udtCutByFragArgFlags::AllowDeaths) != 0;

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
		if(!IsAllowedMeanOfDeath(data.MeanOfDeath, extraInfo.AllowedMeansOfDeaths, parser._protocol))
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

void udtCutByFragAnalyzer::InitAllocators(u32 demoCount)
{
	_analyzerFinalAllocator.Init((uptr)(1 << 16) * (uptr)demoCount);
	_analyzer.InitAllocators(demoCount, _analyzerFinalAllocator, PlugIn->GetTempAllocator());
}

void udtCutByFragAnalyzer::StartAnalysis()
{
	_frags.Clear();
}

void udtCutByFragAnalyzer::FinishAnalysis()
{
	AddCurrentSectionIfValid();
}

void udtCutByFragAnalyzer::AddCurrentSectionIfValid()
{
	const udtCutByPatternArg& info = PlugIn->GetInfo();
	const udtCutByFragArg& extraInfo = GetExtraInfo<udtCutByFragArg>();

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
	CutSections.Add(cut);

	_frags.Clear();
}

void udtCutByFragAnalyzer::AddMatch(const udtParseDataObituary& data)
{
	Frag match;
	match.ServerTimeMs = data.ServerTimeMs;
	match.GameStateIndex = data.GameStateIndex;
	_frags.Add(match);
}
