#include "cut_section.hpp"
#include "utils.hpp"


void MergeRanges(udtVMArray<udtCutSection>& result, const udtVMArray<udtCutSection>& ranges)
{
	if(ranges.IsEmpty())
	{
		return;
	}

	result.Clear();

	udtCutSection current = ranges[0];
	for(u32 i = 1, count = ranges.GetSize(); i < count; ++i)
	{
		const udtCutSection& it = ranges[i];
		if(current.EndTimeMs >= it.StartTimeMs && current.GameStateIndex == it.GameStateIndex)
		{
			current.EndTimeMs = udt_max(current.EndTimeMs, it.EndTimeMs);
			current.PatternTypes |= it.PatternTypes;
		}
		else
		{
			result.Add(current);
			current = it;
		}
	}

	result.Add(current);
}