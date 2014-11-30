#include "multi_threaded_processing.hpp"
#include "file_stream.hpp"
#include "utils.hpp"
#include "threads.hpp"
#include "parser_context.hpp"
#include "system.hpp"

#include <stdlib.h>
#include <assert.h>


#define MIN_BYTE_SIZE_PER_THREAD (u64)(6 * (1<<20))


struct FileInfo
{
	u64 ByteCount;
	const char* FilePath;
	u32 ThreadIdx;
};

static int SortByFileSizesDescending(const void* aPtr, const void* bPtr)
{
	const u64 a = ((FileInfo*)aPtr)->ByteCount;
	const u64 b = ((FileInfo*)bPtr)->ByteCount;

	return (int)(b - a);
}

static int SortByThreadIndexAscending(const void* aPtr, const void* bPtr)
{
	const u32 a = ((FileInfo*)aPtr)->ThreadIdx;
	const u32 b = ((FileInfo*)bPtr)->ThreadIdx;

	return (int)(a - b);
}

bool udtDemoThreadAllocator::Process(const char** filePaths, u32 fileCount, u32 maxThreadCount)
{
	if(maxThreadCount <= 1 || fileCount <= 1)
	{
		return false;
	}

	u32 processorCoreCount = 1;
	GetProcessorCoreCount(processorCoreCount);
	if(processorCoreCount == 1)
	{
		return false;
	}

	// Get file sizes and make sure we have enough data to process
	// to even consider launching new threads.
	udtVMArray<FileInfo> files;
	u64 totalByteCount = 0;
	files.Resize(fileCount);
	for(u32 i = 0; i < fileCount; ++i)
	{
		const u64 byteCount = udtFileStream::GetFileLength(filePaths[i]);
		files[i].FilePath = filePaths[i];
		files[i].ByteCount = byteCount;
		files[i].ThreadIdx = (u32)-1;
		totalByteCount += byteCount;
	}

	if(totalByteCount < 2 * MIN_BYTE_SIZE_PER_THREAD)
	{
		return false;
	}

	// Prepare the final thread array.
	maxThreadCount = udt_min(maxThreadCount, (u32)4);
	maxThreadCount = udt_min(maxThreadCount, processorCoreCount);
	const u32 finalThreadCount = udt_min(maxThreadCount, (u32)(totalByteCount / MIN_BYTE_SIZE_PER_THREAD));
	Threads.Resize(finalThreadCount);
	memset(Threads.GetStartAddress(), 0, (size_t)Threads.GetSize() * sizeof(udtParsingThreadData));

	// Sort files by size.
	qsort(files.GetStartAddress(), (size_t)fileCount, sizeof(FileInfo), &SortByFileSizesDescending);

	// Assign files to threads.
	for(u32 i = 0; i < fileCount; ++i)
	{
		u32 threadIdx = 0;
		u64 smallestThreadByteCount = (u64)-1;
		for(u32 j = 0; j < finalThreadCount; ++j)
		{
			if(Threads[j].TotalByteCount < smallestThreadByteCount)
			{
				smallestThreadByteCount = Threads[j].TotalByteCount;
				threadIdx = j;
			}
		}

		Threads[threadIdx].TotalByteCount += files[i].ByteCount;
		files[i].ThreadIdx = threadIdx;
	}

	// Sort files by thread index.
	qsort(files.GetStartAddress(), (size_t)fileCount, sizeof(FileInfo), &SortByThreadIndexAscending);
	assert(files[0].ThreadIdx == 0); // Ascending order means thread 0 is always first.

	// Build and finalize the arrays.
	u32 threadIdx = 0;
	u32 firstFileIdx = 0;
	FilePaths.Resize(fileCount);
	for(u32 i = 0; i < fileCount; ++i)
	{
		if(files[i].ThreadIdx != threadIdx)
		{
			assert(threadIdx < Threads.GetSize());
			Threads[threadIdx].FirstFileIndex = firstFileIdx;
			Threads[threadIdx].FileCount = i - firstFileIdx;
			firstFileIdx = i;
			++threadIdx;
		}

		FilePaths[i] = files[i].FilePath;
	}
	Threads[threadIdx].FirstFileIndex = firstFileIdx;
	Threads[threadIdx].FileCount = fileCount - firstFileIdx;

#if defined(UDT_DEBUG)
	// The following is to catch errors in the file distribution across threads.
	for(u32 i = 0; i < finalThreadCount; ++i)
	{
		const u32 first = Threads[i].FirstFileIndex;

		u64 totalByteCount = 0;
		for(u32 j = 0; j < Threads[i].FileCount; ++i)
		{
			totalByteCount += udtFileStream::GetFileLength(FilePaths[first + j]);
		}

		assert(Threads[i].TotalByteCount == totalByteCount);
	}
#endif

	return true;
}

// @TODO: Move this.
extern bool CutByChat(udtParserContext* context, const udtParseArg* info, const udtCutByChatArg* chatInfo, const char* demoFilePath);

static void ThreadFunction(void* userData)
{
	udtParsingThreadData* const data = (udtParsingThreadData*)userData;
	if(data == NULL)
	{
		return;
	}

	udtParsingSharedData* const shared = data->Shared;
	const udtCutByChatArg* const chatInfo = (const udtCutByChatArg*)shared->JobTypeSpecificInfo;
	const udtParseArg* const info = shared->ParseInfo;
	const u32 startIdx = data->FirstFileIndex;
	const u32 endIdx = startIdx + data->FileCount;
	for(u32 i = startIdx; i < endIdx; ++i)
	{
		CutByChat(data->Context, info, chatInfo, shared->FilePaths[i]);
	}
}

bool udtMultiThreadedParsing::Process(udtParserContext* contexts, 
									  udtDemoThreadAllocator& threadInfo, 
									  const udtParseArg* parseInfo,
									  const udtMultiParseArg* multiParseInfo,
									  const void* jobTypeSpecificInfo)
{
	const u32 threadCount = threadInfo.Threads.GetSize();

	udtParsingSharedData sharedData;
	memset(&sharedData, 0, sizeof(sharedData));
	sharedData.JobTypeSpecificInfo = jobTypeSpecificInfo;
	sharedData.MultiParseInfo = multiParseInfo;
	sharedData.ParseInfo = parseInfo;
	sharedData.FilePaths = threadInfo.FilePaths.GetStartAddress();

	bool success = true;
	udtVMArray<udtThread> threads;
	threads.Resize(threadCount);
	for(u32 i = 0; i < threadCount; ++i)
	{
		udtThread& thread = threads[i];
		new (&thread) udtThread;
		threadInfo.Threads[i].Context = contexts + i;
		threadInfo.Threads[i].Shared = &sharedData;
		if(!thread.CreateAndStart(&ThreadFunction, &threadInfo.Threads[i]))
		{
			success = false;
			goto thread_clean_up;
		}
	}

	for(u32 i = 0; i < threadCount; ++i)
	{
		threads[i].Join();
	}

thread_clean_up:
	for(u32 i = 0; i < threadCount; ++i)
	{
		threads[i].Release();
	}

	return success;
}
