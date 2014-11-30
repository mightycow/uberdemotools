#pragma once


#include "api.h"
#include "array.hpp"


struct udtParsingSharedData
{
	const char** FilePaths;
	const udtParseArg* ParseInfo;
	const udtMultiParseArg* MultiParseInfo;
	const void* JobTypeSpecificInfo; // udtCutByChatArg etc
};

struct udtParsingThreadData
{
	u64 TotalByteCount;
	udtParsingSharedData* Shared;
	udtParserContext* Context;
	u32 FirstFileIndex;
	u32 FileCount;
	f32 Progress;
};

struct udtDemoThreadAllocator
{
	// Returns true if more than 1 thread should be launched.
	bool Process(const char** filePaths, u32 fileCount, u32 maxThreadCount);

	udtVMArray<const char*> FilePaths;
	//udtVMArray<u64> FileSizes;
	udtVMArray<udtParsingThreadData> Threads;
};

struct udtMultiThreadedParsing
{
	bool Process(udtParserContext* contexts, 
                 udtDemoThreadAllocator& threadInfo, 
				 const udtParseArg* parseInfo, 
				 const udtMultiParseArg* multiParseInfo,
				 const void* jobTypeSpecificInfo);
};
