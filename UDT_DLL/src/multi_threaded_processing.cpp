#include "multi_threaded_processing.hpp"
#include "file_stream.hpp"
#include "quicksort.hpp"
#include "utils.hpp"


#define MIN_BYTE_SIZE_PER_THREAD (u64)(16 * (1<<20))


static int SortByFileSizesDescending(void* ctxPtr, const void* aPtr, const void* bPtr)
{
	const udtVMArray<u64>& fileSizes = ((udtDemoThreadAllocator*)ctxPtr)->FileSizes;

	return fileSizes[*(u32*)aPtr] - fileSizes[*(u32*)bPtr];
}

void udtDemoThreadAllocator::Process(const char** filePaths, u32 fileCount, u32 maxThreadCount)
{
	if(maxThreadCount <= 1 || fileCount <= 1)
	{
		return;
	}

	//
	// Get file sizes and make sure we have enough data to process
	// to consider launching new threads.
	//
	udtVMArray<u64> FileSizes;
	u64 totalByteCount = 0;
	FileSizes.Resize(fileCount);
	for(u32 i = 0; i < fileCount; ++i)
	{
		const u64 byteCount = udtFileStream::GetFileLength(filePaths[i]);
		FileSizes[i] = byteCount;
		totalByteCount += byteCount;
	}

	if(totalByteCount < 2 * MIN_BYTE_SIZE_PER_THREAD)
	{
		return;
	}

	maxThreadCount = udt_min(maxThreadCount, (u32)4);
	const u32 finalThreadCount = udt_min(maxThreadCount, (u32)(totalByteCount / MIN_BYTE_SIZE_PER_THREAD));
	Threads.Resize(finalThreadCount);
	for(u32 i = 0; i < finalThreadCount; ++i)
	{
		Threads[i].FileCount = 0;
		Threads[i].FirstFileIndex = 0;
		Threads[i].TotalByteCount = 0;
	}

	//
	// Sort files by byte sizes.
	//
	udtVMArray<u32> DescendingSizeFileIndices;
	DescendingSizeFileIndices.Resize(fileCount);
	for(u32 i = 0; i < fileCount; ++i)
	{
		DescendingSizeFileIndices[i] = i;
	}
	QuickSort(DescendingSizeFileIndices.GetStartAddress(), fileCount, (u32)sizeof(u32), &SortByFileSizesDescending, this);

	//
	// Assign files to threads.
	//
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

		Threads[threadIdx].TotalByteCount += FileSizes[DescendingSizeFileIndices[i]];

		// @TODO: Assign file index DescendingSizeFileIndices[i] to thread Threads[threadIdx]
	}
}
