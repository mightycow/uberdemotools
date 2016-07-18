#pragma once


#include "array.hpp"


struct udtPatternSearchContext_s
{
	udtPatternSearchContext_s(const udtPatternSearchArg* patternInfo)
	{
		PatternInfo = patternInfo;
	}

	udtVMArray<udtPatternMatch> Matches { "PatternSearchContext::MatchesArray" };
	const udtPatternSearchArg* PatternInfo;
};
