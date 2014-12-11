#pragma once


#include "api.h"
#include "array.hpp"


struct udtParsingJobType
{
	enum Id
	{
		General,
		CutByChat,
		CutByFrag,
		Count
	};
};

struct udtParsingSharedData
{
	const char** FilePaths;
	u64* FileSizes;
	const udtParseArg* ParseInfo;
	const udtMultiParseArg* MultiParseInfo;
	const void* JobTypeSpecificInfo; // udtCutByChatArg etc
	u32 JobType; // Of type udtParsingJobType.
};

struct udtParsingThreadData
{
	u64 TotalByteCount;
	udtParsingSharedData* Shared;
	udtParserContext* Context;
	u32 FirstFileIndex;
	u32 FileCount;
	f32 Progress;
	bool Finished;
	bool Stop;
	bool Result;
};

struct udtDemoThreadAllocator
{
	udtDemoThreadAllocator();

	// Returns true if more than 1 thread should be launched.
	bool Process(const char** filePaths, u32 fileCount, u32 maxThreadCount);

	udtVMArray<const char*> FilePaths;
	udtVMArray<u64> FileSizes;
	udtVMArray<u32> InputIndices;
	udtVMArray<udtParsingThreadData> Threads;
};

struct udtMultiThreadedParsing
{
	bool Process(udtParserContext* contexts, 
                 udtDemoThreadAllocator& threadInfo, 
				 const udtParseArg* parseInfo, 
				 const udtMultiParseArg* multiParseInfo,
				 udtParsingJobType::Id jobType,
				 const void* jobTypeSpecificInfo);
};
