#pragma once


#include "array.hpp"
#include "parser.hpp"


typedef udtVMArray<udtBaseParser::udtCutInfo> udtCutArray;
typedef udtCutArray udtCutArrayArray[64];

bool CutList_HasOverlappingCuts(const udtCutArray& cutList); // Sorted source list.
void CutList_GenerateNonOverlappingLists(udtCutArrayArray& dst, u32& dstSize, const udtCutArray& src); // Sorted source list.
