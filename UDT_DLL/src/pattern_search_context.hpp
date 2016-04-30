#pragma once


#include "array.hpp"


struct udtPatternSearchContext_s
{
	udtPatternSearchContext_s(const udtPatternSearchArg* patternInfo)
	{
		Matches.Init(UDT_MEMORY_PAGE_SIZE, "PatternSearchContext::MatchesArray");
		PatternInfo = patternInfo;
	}

	udtVMArray<udtPatternMatch> Matches;
	const udtPatternSearchArg* PatternInfo;
};
