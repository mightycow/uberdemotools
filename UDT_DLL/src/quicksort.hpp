#pragma once


#include "types.hpp"


typedef int (*QuickSortCompareCb)(void*, const void*, const void*);

extern void QuickSort(void* base, u32 elementCount, u32 elementSize, QuickSortCompareCb comparator, void* context);
