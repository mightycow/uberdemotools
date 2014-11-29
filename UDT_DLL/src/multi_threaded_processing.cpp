#include "multi_threaded_processing.hpp"
#include "file_stream.hpp"
#include "utils.hpp"

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

	// @TODO: Get the processor's core count.

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
	const u32 finalThreadCount = udt_min(maxThreadCount, (u32)(totalByteCount / MIN_BYTE_SIZE_PER_THREAD));
	Threads.Resize(finalThreadCount);
	for(u32 i = 0; i < finalThreadCount; ++i)
	{
		Threads[i].FileCount = 0;
		Threads[i].FirstFileIndex = 0;
		Threads[i].TotalByteCount = 0;
	}

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
	assert(files[0].ThreadIdx == 0);

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

	return true;
}
