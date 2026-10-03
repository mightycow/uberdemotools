#pragma once


#include "array.hpp"
#include "parser.hpp"


typedef udtBaseParser::udtCutInfo udtParserCut;
typedef udtVMArray<udtParserCut> udtCutArray;
typedef udtCutArray udtCutArrayArray[64];

struct udtCutBatcher
{
	udtCutBatcher();

	void CreateBatches();

	// Input. Can be unsorted, have overlaps and have invalid ranges.
	udtCutArray Cuts { "CutBatcher::CutsArray" };

	// Output.
	udtCutArrayArray Batches;
	u32 BatchCount;
};
