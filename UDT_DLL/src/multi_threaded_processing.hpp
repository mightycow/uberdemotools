#pragma once


#include "api.h"
#include "array.hpp"


struct udtDemoThreadAllocator
{
	// Returns true if more than 1 thread should be launched.
	bool Process(const char** filePaths, u32 fileCount, u32 maxThreadCount);

	struct ThreadData
	{
		u64 TotalByteCount;
		u32 FirstFileIndex;
		u32 FileCount;
	};

	udtVMArray<const char*> FilePaths;
	//udtVMArray<u64> FileSizes;
	udtVMArray<ThreadData> Threads;
};
