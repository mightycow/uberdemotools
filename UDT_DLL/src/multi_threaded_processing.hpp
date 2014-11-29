#pragma once


#include "api.h"
#include "array.hpp"


struct udtDemoThreadAllocator
{
	void Process(const char** filePaths, u32 fileCount, u32 maxThreadCount);

	struct ThreadData
	{
		u64 TotalByteCount;
		u32 FirstFileIndex;
		u32 FileCount;
	};

	udtVMArray<u32> FinalFileIndices;
	udtVMArray<ThreadData> Threads;
};
