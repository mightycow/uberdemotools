#pragma once


#include "array.hpp"


struct udtCutSection
{
	s32 GameStateIndex;
	s32 StartTimeMs;
	s32 EndTimeMs;
};

extern void MergeRanges(udtVMArray<udtCutSection>& result, const udtVMArray<udtCutSection>& ranges);
