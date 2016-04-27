#pragma once


#include "array.hpp"


struct udtCutSection
{
	const char* VeryShortDesc; // Used for output file name formatting. May be NULL.
	s32 GameStateIndex;
	s32 StartTimeMs;
	s32 EndTimeMs;
	u32 PatternTypes; // Bits indexed with udtPatternType::Id.
};

extern void MergeRanges(udtVMArray<udtCutSection>& result, const udtVMArray<udtCutSection>& ranges);
