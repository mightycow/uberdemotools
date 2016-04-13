#include "read_only_sequ_file_stream.hpp"
#include "memory.hpp"


#if defined(UDT_WINDOWS)


#include "string.hpp"
#include "scoped_stack_allocator.hpp"
#include "thread_local_allocators.hpp"
#include "assert_or_fatal.hpp"
#include "utils.hpp"

#include <Windows.h>


#define BLOCK_SIZE  (128*1024)
#define BLOCK_COUNT (2)


static bool GetDeviceSectorSize(u32& sectorSize, const WCHAR* deviceName)
{
	const HANDLE file = CreateFileW(deviceName, STANDARD_RIGHTS_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	STORAGE_PROPERTY_QUERY query;
	ZeroMemory(&query, sizeof(query));
	query.QueryType = PropertyStandardQuery;
	query.PropertyId = StorageAccessAlignmentProperty;

	STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR alignment;
	ZeroMemory(&alignment, sizeof(alignment));

	DWORD bytes = 0;
	const bool success = DeviceIoControl(file, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &alignment, sizeof(alignment), &bytes, NULL) != FALSE;
	CloseHandle(file);
	if(success)
	{
		sectorSize = (u32)alignment.BytesPerPhysicalSector;
	}

	return success;
}

static u32 GetLargestDeviceSectorSize()
{
	u32 largestSectorSize = UDT_MEMORY_PAGE_SIZE;
	WCHAR path[7] = L"\\\\.\\C:";

	const DWORD devices = GetLogicalDrives();
	for(DWORD i = 2; i < 32; ++i)
	{
		if(((devices >> i) & 1) == 0)
		{
			continue;
		}

		path[4] = L'A' + (WCHAR)i;
		u32 sectorSize = 0;
		if(GetDeviceSectorSize(sectorSize, path) && sectorSize > largestSectorSize)
		{
			largestSectorSize = sectorSize;
		}
	}

	return largestSectorSize;
}

static u32 IsPowerOfTwo(u32 x)
{
	return x != 0 && (x & (x - 1)) == 0;
}

static u8* AlignTop(u8* address, u32 alignment)
{
	const uptr alignmentM1 = uptr(alignment - 1);
	const uptr aligned = uptr(address + alignmentM1) & (~alignmentM1);

	return (u8*)aligned;
}


struct BlockInfo
{
	OVERLAPPED Overlapped;
	HANDLE Event; // If invalid: NULL.
	u32 BlockIndex;
	bool RequestPending;
	bool Ready;
};

struct udtReadOnlySequentialFileStreamImpl
{
	BlockInfo _blocks[BLOCK_COUNT];
	u8* _buffer;     // Aligned buffer used for the reads.
	void* _realBuffer; // The buffer we need to free.
	HANDLE _file;    // If invalid: INVALID_HANDLE_VALUE.
	u32 _blockSize;
	u32 _fileByteCount;
	u32 _fileOffset;
};

udtReadOnlySequentialFileStream::udtReadOnlySequentialFileStream()
{
	_data = (udtReadOnlySequentialFileStreamImpl*)udt_malloc(sizeof(udtReadOnlySequentialFileStreamImpl));
	_data->_buffer = NULL;
	_data->_file = INVALID_HANDLE_VALUE;
	_data->_blockSize = 0;
	_data->_fileByteCount = 0;
	_data->_fileOffset = 0;
	for(u32 i = 0; i < BLOCK_COUNT; ++i)
	{
		_data->_blocks[i].Event = NULL;
		_data->_blocks[i].BlockIndex = i;
		_data->_blocks[i].RequestPending = false;
		_data->_blocks[i].Ready = false;
	}
}

udtReadOnlySequentialFileStream::~udtReadOnlySequentialFileStream()
{
	Destroy(); 
}

bool udtReadOnlySequentialFileStream::Init()
{
	for(int i = 0; i < BLOCK_COUNT; ++i)
	{
		const HANDLE event = CreateEvent(NULL, TRUE, FALSE, NULL);
		if(event == NULL)
		{
			return false;
		}
		_data->_blocks[i].Event = event;
	}

	// If the largest sector size found on the system is higher than a page size,
	// we use that as the alignment. Otherwise, we use the page size.
	// This is for fast/correct DMA with unbuffered reads.
	u32 alignment = 1; // In number of pages.
	u32 bytesRequired = BLOCK_SIZE * BLOCK_COUNT;
	const u32 sectorSize = GetLargestDeviceSectorSize();
	if(sectorSize > 4096 && IsPowerOfTwo(alignment))
	{
		alignment = sectorSize / UDT_MEMORY_PAGE_SIZE;
		bytesRequired += (alignment - 1) * UDT_MEMORY_PAGE_SIZE;
	}

	void* const buffer = VirtualAlloc(NULL, (SIZE_T)bytesRequired, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if(buffer == NULL)
	{
		return false;
	}

	_data->_buffer = AlignTop((u8*)buffer, alignment * UDT_MEMORY_PAGE_SIZE);
	_data->_realBuffer = buffer;
	_data->_blockSize = BLOCK_SIZE;

	return true;
}

bool udtReadOnlySequentialFileStream::Open(const char* filePath, u32 offset)
{
	Close();
	_data->_fileOffset = 0;
	for(u32 i = 0; i < BLOCK_COUNT; ++i)
	{
		_data->_blocks[i].BlockIndex = i;
		_data->_blocks[i].RequestPending = false;
		_data->_blocks[i].Ready = false;
	}

	udtVMLinearAllocator& allocator = udtThreadLocalAllocators::GetTempAllocator();
	udtVMScopedStackAllocator allocatorScope(allocator);
	wchar_t* const wideFilePath = udtString::ConvertToUTF16(allocator, udtString::NewConstRef(filePath));
	const DWORD fileAttribs = FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED;
	const HANDLE file = CreateFileW(wideFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, fileAttribs, NULL);
	if(file == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	LARGE_INTEGER size;
	GetFileSizeEx(file, &size);
	const u32 blockSize = _data->_blockSize;
	_data->_file = file;
	_data->_fileByteCount = (u32)size.QuadPart;
	_data->_fileOffset = offset;

	const u32 blockCount = (_data->_fileByteCount + blockSize - 1) / blockSize;
	const u32 requestCount = udt_min(blockCount, (u32)BLOCK_COUNT - 1);
	const u32 firstBlockIndex = offset / BLOCK_SIZE;
	for(u32 i = 0; i < requestCount; ++i)
	{
		RequestBlock(firstBlockIndex + i);
	}

	return true;
}

void udtReadOnlySequentialFileStream::RequestBlock(u32 blockIndex)
{
	const u32 blockSize = _data->_blockSize;
	const u32 blockId = blockIndex % BLOCK_COUNT;
	BlockInfo& block = _data->_blocks[blockId];
	if(block.BlockIndex == blockIndex && (block.RequestPending || block.Ready))
	{
		return;
	}

	LARGE_INTEGER offset;
	offset.QuadPart = (LONGLONG)blockIndex * blockSize;

	ZeroMemory(&block.Overlapped, sizeof(block.Overlapped));
	block.Overlapped.Offset = offset.LowPart;
	block.Overlapped.OffsetHigh = offset.HighPart;
	block.Overlapped.hEvent = block.Event;
	const bool success = ReadFile(_data->_file, _data->_buffer + blockId * blockSize, (DWORD)blockSize, NULL, &block.Overlapped) != FALSE;
	block.BlockIndex = blockIndex;
	block.RequestPending = !success && GetLastError() == ERROR_IO_PENDING;
	block.Ready = success;
}

void udtReadOnlySequentialFileStream::WaitForBlock(u32 blockIndex)
{
	const u32 blockId = blockIndex % BLOCK_COUNT;
	BlockInfo& block = _data->_blocks[blockId];
	if(block.BlockIndex == blockIndex && block.RequestPending)
	{
		DWORD bytesRead = 0;
		GetOverlappedResult(_data->_file, &block.Overlapped, &bytesRead, TRUE);
		block.RequestPending = false;
		block.Ready = true;
	}
}

u32 udtReadOnlySequentialFileStream::Read(void* dstBuff, u32 elementSize, u32 count)
{
	u32 byteCount = elementSize * count;
	const u32 blockSize = _data->_blockSize;
	UDT_ASSERT_OR_FATAL(byteCount <= blockSize);

	const u32 fileSize = _data->_fileByteCount;
	const u32 fileOffset = _data->_fileOffset;
	if(byteCount == 0 || fileOffset == fileSize)
	{
		return 0;
	}

	if(fileOffset + byteCount > fileSize)
	{
		byteCount = fileSize - fileOffset;
	}

	const u32 blockIndex = fileOffset / blockSize;
	WaitForBlock(blockIndex);

	const u32 nextBlockIndex = blockIndex + BLOCK_COUNT - 1;
	const u32 blockCount = (fileSize + blockSize - 1) / blockSize;
	if(nextBlockIndex < blockCount)
	{
		RequestBlock(nextBlockIndex);
	}

	if((fileOffset % blockSize) + byteCount > blockSize)
	{
		WaitForBlock(blockIndex + 1);
	}

	const u32 blockId = blockIndex % BLOCK_COUNT;
	const u8* const buffer = _data->_buffer;
	const u8* const readData = buffer + (blockId * blockSize) + (fileOffset % blockSize);
	const u8* const bufferEnd = buffer + (u32)BLOCK_COUNT * blockSize;
	if(readData + byteCount > bufferEnd)
	{
		u8* const writeData = (u8*)dstBuff;
		const u32 byteCount1 = (u32)(bufferEnd - readData);
		const u32 byteCount2 = byteCount - byteCount1;
		memcpy(writeData, readData, (size_t)byteCount1);
		memcpy(writeData + byteCount1, buffer, (size_t)byteCount2);
	}
	else
	{
		memcpy(dstBuff, readData, (size_t)byteCount);
	}
	_data->_fileOffset += byteCount;

	return byteCount / elementSize;
}

u32 udtReadOnlySequentialFileStream::Write(const void* /*srcBuff*/, u32 /*elementSize*/, u32 /*count*/)
{
	UDT_ASSERT_OR_FATAL_ALWAYS("Calling Write on a udtReadOnlySequentialFileStream is invalid!");
	return 0;
}

s32	udtReadOnlySequentialFileStream::Seek(s32 /*offset*/, udtSeekOrigin::Id /*origin*/)
{
	UDT_ASSERT_OR_FATAL_ALWAYS("Calling Seek on a udtReadOnlySequentialFileStream is invalid!");
	return 0;
}

s32 udtReadOnlySequentialFileStream::Offset()
{
	return (s32)_data->_fileOffset;
}

u64 udtReadOnlySequentialFileStream::Length()
{
	return (u64)_data->_fileByteCount;
}

s32 udtReadOnlySequentialFileStream::Close()
{
	if(_data->_file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(_data->_file);
		_data->_file = INVALID_HANDLE_VALUE;
	}

	return 0;
}

void udtReadOnlySequentialFileStream::Destroy()
{
	Close();

	for(int i = 0; i < BLOCK_COUNT; ++i)
	{
		const HANDLE event = _data->_blocks[i].Event;
		if(event != NULL)
		{
			CloseHandle(event);
		}
	}

	if(_data->_realBuffer != NULL)
	{
		VirtualFree(_data->_realBuffer, 0, MEM_RELEASE);
	}
	
	free(_data);
}


#endif

