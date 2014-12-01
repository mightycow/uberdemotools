#include "linear_allocator.hpp"
#include "virtual_memory.hpp"

#include <assert.h>


// We don't use large pages (which are 1 MB large instead of 4 KB large).
#define UDT_MEMORY_PAGE_SIZE 4096


udtVMLinearAllocator::udtVMLinearAllocator()
{
	_addressSpaceStart = NULL;
	_firstFreeByteIndex = 0;
	_reservedByteCount = 0;
	_commitByteCountGranularity = 0;
	_committedByteCount = 0;
}

udtVMLinearAllocator::~udtVMLinearAllocator()
{
	Destroy();
}

bool udtVMLinearAllocator::Init(u32 reservedByteCount, u32 commitByteCountGranularity, bool commitFirstBlock)
{
	if(_addressSpaceStart != NULL)
	{
		return false;
	}

	// Let's reserve at least 2 pages and commit whole pages.
	assert(reservedByteCount >= UDT_MEMORY_PAGE_SIZE * 2);
	assert((commitByteCountGranularity % UDT_MEMORY_PAGE_SIZE) == 0);

	u8* const data = (u8*)VirtualMemoryReserve(reservedByteCount);
	if(data == NULL)
	{
		return false;
	}

	if(commitFirstBlock)
	{
		if(!VirtualMemoryCommit(data, commitByteCountGranularity))
		{
			VirtualMemoryDecommitAndRelease(data, reservedByteCount);
			return false;
		}
	}
	
	_addressSpaceStart = data;
	_firstFreeByteIndex = 0;
	_reservedByteCount = reservedByteCount;
	_committedByteCount = commitFirstBlock ? commitByteCountGranularity : 0;
	_commitByteCountGranularity = commitByteCountGranularity;

	return true;
}

u8* udtVMLinearAllocator::Allocate(u32 byteCount)
{
	assert(_addressSpaceStart != NULL);

	byteCount = (byteCount + 3) & (~3); // Will make sure the next alignment is 4-byte aligned, just like this one.
	assert(_firstFreeByteIndex + byteCount <= _reservedByteCount); // We didn't reserve enough address space!

	if(_firstFreeByteIndex + byteCount > _committedByteCount)
	{
		// How many more commit chunks do we need?
		const u32 neededByteCount = _firstFreeByteIndex + byteCount - _committedByteCount;
		const u32 chunkCount = (neededByteCount + _commitByteCountGranularity - 1) / _commitByteCountGranularity;
		const u32 newByteCount = chunkCount * _commitByteCountGranularity;
		if(!VirtualMemoryCommit(_addressSpaceStart + _committedByteCount, newByteCount))
		{
			return NULL;
		}
		_committedByteCount += newByteCount;
	}

	u8* const data = _addressSpaceStart + _firstFreeByteIndex;
	_firstFreeByteIndex += byteCount;

	return data;
}

void udtVMLinearAllocator::Pop(u32 byteCount)
{
	if(byteCount > _firstFreeByteIndex)
	{
		return;
	}

	_firstFreeByteIndex -= byteCount;
}

void udtVMLinearAllocator::Clear()
{
	_firstFreeByteIndex = 0;
}

void udtVMLinearAllocator::Purge()
{
	if(_committedByteCount - _firstFreeByteIndex < UDT_MEMORY_PAGE_SIZE)
	{
		return;
	}

	const u32 pageSizeM1 = UDT_MEMORY_PAGE_SIZE - 1;
	u8* const memoryToDecommit = (u8*)((u32)(_addressSpaceStart + _firstFreeByteIndex + pageSizeM1) & (~pageSizeM1));
	u8* const committedEnd = _addressSpaceStart + _committedByteCount;
	const u32 byteCount = (u32)(committedEnd - memoryToDecommit);
	VirtualMemoryDecommit(memoryToDecommit, byteCount);
	_committedByteCount -= byteCount;
}

void udtVMLinearAllocator::SetCurrentByteCount(u32 byteCount)
{
	if(byteCount > _committedByteCount)
	{
		return;
	}

	_firstFreeByteIndex = byteCount;
}

u32	udtVMLinearAllocator::GetCurrentByteCount() const
{
	return _firstFreeByteIndex;
}

u32 udtVMLinearAllocator::GetCommittedByteCount() const
{
	return _committedByteCount;
}

u8* udtVMLinearAllocator::GetStartAddress() const
{
	return _addressSpaceStart;
}

u8* udtVMLinearAllocator::GetCurrentAddress() const
{
	return _addressSpaceStart + _firstFreeByteIndex;
}

void udtVMLinearAllocator::Destroy()
{
	if(_addressSpaceStart == NULL)
	{
		return;
	}

	VirtualMemoryDecommitAndRelease(_addressSpaceStart, _reservedByteCount);
	_addressSpaceStart = NULL;
	_firstFreeByteIndex = 0;
	_reservedByteCount = 0;
	_commitByteCountGranularity = 0;
	_committedByteCount = 0;
}
