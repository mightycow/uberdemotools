#include "analysis_cut_by_frag.hpp"


// @TODO:
static const s32 playerIndex = 0;


void udtCutByFragAnalyzer::FindCutSections()
{
	for(u32 i = 0, count = _analyzer.Obituaries.GetSize(); i < count; ++i)
	{
		const udtParseDataObituary& data = _analyzer.Obituaries[i];
		if(data.AttackerIdx != playerIndex || data.TargetIdx == playerIndex)
		{
			continue;
		}

		if(_frags.IsEmpty())
		{
			AddMatch(data);
		}
		else
		{
			const Frag previousMatch = _frags[_frags.GetSize() - 1];
			if(data.GameStateIndex != previousMatch.GameStateIndex ||
			   data.ServerTimeMs > previousMatch.ServerTimeMs + (s32)(_info.TimeBetweenFragsSec * 1000))
			{
				AddCurrentSectionIfValid();
			}

			AddMatch(data);
		}
	}

	AddCurrentSectionIfValid();
}

void udtCutByFragAnalyzer::AddCurrentSectionIfValid()
{
	const u32 fragCount = _frags.GetSize();
	if(fragCount < 2 || fragCount < _info.MinFragCount)
	{
		_frags.Clear();
		return;
	}

	CutSection cut;
	cut.GameStateIndex = _frags[0].GameStateIndex;
	cut.StartTimeMs = _frags[0].ServerTimeMs - (s32)(_info.StartOffsetSec * 1000);
	cut.EndTimeMs = _frags[fragCount - 1].ServerTimeMs + (s32)(_info.EndOffsetSec * 1000);
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
