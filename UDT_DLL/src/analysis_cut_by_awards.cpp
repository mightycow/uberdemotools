#include "analysis_cut_by_awards.hpp"
#include "utils.hpp"


static bool IsAwardMatch(u32 udtAwardFlags, s32 idAwardId, udtProtocol::Id protocol)
{
	const s32 bit = GetUDTAwardBitFromIdAward(idAwardId, protocol);

	return (udtAwardFlags & (u32)bit) != 0;
}


void udtCutByAwardAnalyzer::FindCutSections()
{
	for(u32 i = 0, count = _analyzer.AwardEvents.GetSize(); i < count; ++i)
	{
		const udtParseDataAward& data = _analyzer.AwardEvents[i];
		if(!IsAwardMatch(_info.AllowedAwards, data.AwardId, _protocol))
		{
			continue;
		}

		if(_awards.IsEmpty())
		{
			AddMatch(data);
		}
		else
		{
			const Award previousMatch = _awards[_awards.GetSize() - 1];
			if(data.GameStateIndex != previousMatch.GameStateIndex ||
			   data.ServerTimeMs > previousMatch.ServerTimeMs + (s32)(_info.EndOffsetSec * 1000))
			{
				AddCurrentSectionIfValid();
			}

			AddMatch(data);
		}
	}

	AddCurrentSectionIfValid();
}

void udtCutByAwardAnalyzer::AddCurrentSectionIfValid()
{
	const u32 matchCount = _awards.GetSize();
	if(matchCount == 0)
	{
		return;
	}

	CutSection cut;
	cut.GameStateIndex = _awards[0].GameStateIndex;
	cut.StartTimeMs = _awards[0].ServerTimeMs - (s32)(_info.StartOffsetSec * 1000);
	cut.EndTimeMs = _awards[matchCount - 1].ServerTimeMs + (s32)(_info.EndOffsetSec * 1000);
	CutSections.Add(cut);

	_awards.Clear();
}

void udtCutByAwardAnalyzer::AddMatch(const udtParseDataAward& data)
{
	Award match;
	match.ServerTimeMs = data.ServerTimeMs;
	match.GameStateIndex = data.GameStateIndex;
	_awards.Add(match);
}
