#include "analysis_cut_by_frag.hpp"


static const s32 playerIndex = 0;
static const s32 maxTimeBetweenFragsMs = 5000;
static const u32 minFragCount = 3;


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
			   data.ServerTimeMs > previousMatch.ServerTimeMs + maxTimeBetweenFragsMs)
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
	if(fragCount < 2 || fragCount < minFragCount)
	{
		_frags.Clear();
		return;
	}

	CutSection cut;
	cut.GameStateIndex = _frags[0].GameStateIndex;
	cut.StartTimeMs = _frags[0].ServerTimeMs;
	cut.EndTimeMs = _frags[fragCount - 1].ServerTimeMs;
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
