#include "linear_allocator.hpp"
#include "virtual_memory.hpp"
#include "assert_or_fatal.hpp"
#include "allocator_tracking.hpp"
#include "utils.hpp"

#include <stddef.h> // For ptrdiff_t.

#if !defined(offsetof)
#	define offsetof(s, m) ((size_t)((ptrdiff_t)&reinterpret_cast<const volatile char&>((((s*)NULL)->m))))
#endif


#define    SAFE_NAME    (_name != NULL ? _name : "?")


static udtAllocatorTracker AllocatorTracker;

void udtVMLinearAllocator::GetThreadStats(Stats& stats)
{
	stats = Stats();

	udtIntrusiveList* allocators = NULL;
	AllocatorTracker.GetAllocatorList(allocators);
	if(allocators == NULL)
	{
		return;
	}

	udtIntrusiveListNode* node = allocators->Root.Next;
	while(node != &allocators->Root)
	{
		udtVMLinearAllocator* const allocator = (udtVMLinearAllocator*)((u8*)node - offsetof(udtVMLinearAllocator, _listNode));
		
		++stats.AllocatorCount;
		stats.CommittedByteCount += allocator->_committedByteCount;
		stats.ReservedByteCount += allocator->_reservedByteCount;
		stats.UsedByteCount += allocator->_peakUsedByteCount;
		stats.ResizeCount += allocator->_resizeCount;
		node = node->Next;
	}
}

void udtVMLinearAllocator::GetThreadAllocators(u32& allocatorCount, udtVMLinearAllocator** allocatorArray)
{
	udtIntrusiveList* allocatorList = NULL;
	AllocatorTracker.GetAllocatorList(allocatorList);
	if(allocatorList == NULL)
	{
		allocatorCount = 0;
		return;
	}

	udtIntrusiveListNode* node = allocatorList->Root.Next;
	u32 realAllocatorCount = 0;
	while(node != &allocatorList->Root && realAllocatorCount < allocatorCount)
	{
		udtVMLinearAllocator* const allocator = (udtVMLinearAllocator*)((u8*)node - offsetof(udtVMLinearAllocator, _listNode));
		*allocatorArray++ = allocator;
		++realAllocatorCount;
		node = node->Next;
	}

	allocatorCount = realAllocatorCount;
}


udtVMLinearAllocator::udtVMLinearAllocator(const char* name)
{
	_addressSpaceStart = NULL;
	_usedByteCount = 0;
	_reservedByteCount = 0;
	_commitByteCountGranularity = 0;
	_committedByteCount = 0;
	_peakUsedByteCount = 0;
	_name = name;
	_resizeCount = 0;
	_alignment = (u32)sizeof(void*);

	AllocatorTracker.RegisterAllocator(_listNode);
}

udtVMLinearAllocator::~udtVMLinearAllocator()
{
	AllocatorTracker.UnregisterAllocator(_listNode);

	Destroy();
}

void udtVMLinearAllocator::Init(uptr reservedByteCount)
{
	if(_addressSpaceStart != NULL)
	{
		return;
	}
	
	const uptr commitByteCountGranularity = UDT_MEMORY_PAGE_SIZE;

	// Ensure the reserve size is a multiple of the commit granularity.
	// If it is, leave it as is. If it's not, bump it up to the next multiple.
	reservedByteCount = (reservedByteCount + commitByteCountGranularity - 1) & (~(commitByteCountGranularity - 1));

	// Make sure we reserve at least 1 memory page.
	reservedByteCount = udt_max(reservedByteCount, (uptr)UDT_MEMORY_PAGE_SIZE);

	UDT_ASSERT_OR_FATAL(reservedByteCount >= (uptr)UDT_MEMORY_PAGE_SIZE);

	u8* const data = (u8*)VirtualMemoryReserve(reservedByteCount);
	if(data == NULL)
	{
		UDT_ASSERT_OR_FATAL_ALWAYS("VirtualMemoryReserve failed in allocator '%s'.", SAFE_NAME);
	}
	
	_addressSpaceStart = data;
	_usedByteCount = 0;
	_reservedByteCount = reservedByteCount;
	_committedByteCount = 0;
	_commitByteCountGranularity = commitByteCountGranularity;
}

uptr udtVMLinearAllocator::Allocate(uptr byteCount)
{
	const uptr alignmentMask = (uptr)_alignment - 1;
	byteCount = (byteCount + alignmentMask) & (~alignmentMask);

	if(_addressSpaceStart == NULL)
	{
		Init(udt_max(byteCount, (uptr)UDT_KB(64)));
	}

	// We didn't reserve enough address space?
	if(_usedByteCount + byteCount > _reservedByteCount)
	{
		return AllocateWithRelocation(byteCount);
	}

	if(_usedByteCount + byteCount > _committedByteCount)
	{
		// How many more commit chunks do we need?
		const uptr neededByteCount = _usedByteCount + byteCount - _committedByteCount;
		const uptr chunkCount = (neededByteCount + _commitByteCountGranularity - 1) / _commitByteCountGranularity;
		const uptr newByteCount = chunkCount * _commitByteCountGranularity;
		if(!VirtualMemoryCommit(_addressSpaceStart + _committedByteCount, newByteCount))
		{
			UDT_ASSERT_OR_FATAL_ALWAYS("VirtualMemoryCommit failed in allocator '%s'.", SAFE_NAME);
			return UDT_U32_MAX;
		}
		_committedByteCount += newByteCount;
	}

	const uptr offset = _usedByteCount;
	_usedByteCount += byteCount;
	_peakUsedByteCount = udt_max(_peakUsedByteCount, _usedByteCount);

	return offset;
}

uptr udtVMLinearAllocator::AllocateWithRelocation(uptr byteCount)
{
	// Reserve new address space.
	const uptr commitByteCountGranularity = _commitByteCountGranularity;
	uptr newReservedByteCount = udt_max(_usedByteCount + byteCount, ComputeNewReservedByteCount());
	newReservedByteCount = (newReservedByteCount + commitByteCountGranularity - 1) & (~(commitByteCountGranularity - 1));
	UDT_ASSERT_OR_FATAL(newReservedByteCount >= (uptr)commitByteCountGranularity);
	u8* const data = (u8*)VirtualMemoryReserve(newReservedByteCount);
	if(data == NULL)
	{
		UDT_ASSERT_OR_FATAL_ALWAYS("VirtualMemoryReserve failed in allocator '%s'.", SAFE_NAME);
		return UDT_U32_MAX;
	}

	// Commit just enough for the new used size.
	const uptr neededByteCount = _usedByteCount + byteCount;
	const uptr chunkCount = (neededByteCount + commitByteCountGranularity - 1) / commitByteCountGranularity;
	const uptr newCommitByteCount = chunkCount * commitByteCountGranularity;
	if(!VirtualMemoryCommit(data, newCommitByteCount))
	{
		UDT_ASSERT_OR_FATAL_ALWAYS("VirtualMemoryCommit failed in allocator '%s'.", SAFE_NAME);
		return UDT_U32_MAX;
	}

	// Copy the old data to the new location.
	const uptr oldUsedByteCount = _usedByteCount;
	if(oldUsedByteCount > 0)
	{
		memcpy(data, _addressSpaceStart, (size_t)oldUsedByteCount);
	}
	
	// Return the old address space and pages to the system.
	VirtualMemoryDecommitAndRelease(_addressSpaceStart, _reservedByteCount);
	
	// Update the members.
	_addressSpaceStart = data;
	_reservedByteCount = newReservedByteCount;
	_committedByteCount = newCommitByteCount;
	_usedByteCount += byteCount;
	_peakUsedByteCount = udt_max(_peakUsedByteCount, _usedByteCount);
	++_resizeCount;

	return oldUsedByteCount;
}

uptr udtVMLinearAllocator::ComputeNewReservedByteCount()
{
	const uptr byteCount = _reservedByteCount;

	if(byteCount >= UDT_KB(512))
	{
		return byteCount * 2;
	}

	return byteCount * 4;
}

u8* udtVMLinearAllocator::AllocateAndGetAddress(uptr byteCount)
{
	const uptr offset = Allocate(byteCount);

	return GetStartAddress() + offset;
}

void udtVMLinearAllocator::Pop(uptr byteCount)
{
	if(byteCount > _usedByteCount)
	{
		return;
	}

	_usedByteCount -= byteCount;
}

void udtVMLinearAllocator::Clear()
{
	_usedByteCount = 0;
}

void udtVMLinearAllocator::Purge()
{
	if(_committedByteCount - _usedByteCount < UDT_MEMORY_PAGE_SIZE)
	{
		return;
	}

	const uptr pageSizeM1 = (uptr)(UDT_MEMORY_PAGE_SIZE - 1);
	u8* const memoryToDecommit = (u8*)((uptr)(_addressSpaceStart + _usedByteCount + pageSizeM1) & (~pageSizeM1));
	u8* const committedEnd = _addressSpaceStart + _committedByteCount;
	const uptr byteCount = (uptr)(committedEnd - memoryToDecommit);
	VirtualMemoryDecommit(memoryToDecommit, byteCount);
	_committedByteCount -= byteCount;
}

void udtVMLinearAllocator::SetCurrentByteCount(uptr byteCount)
{
	if(byteCount > _committedByteCount)
	{
		return;
	}

	_usedByteCount = byteCount;
}

uptr udtVMLinearAllocator::GetCurrentByteCount() const
{
	return _usedByteCount;
}

uptr udtVMLinearAllocator::GetCommittedByteCount() const
{
	return _committedByteCount;
}

uptr udtVMLinearAllocator::GetPeakUsedByteCount() const
{
	return _peakUsedByteCount;
}

uptr udtVMLinearAllocator::GetReservedByteCount() const
{
	return _reservedByteCount;
}

u8* udtVMLinearAllocator::GetStartAddress() const
{
	return _addressSpaceStart;
}

u8* udtVMLinearAllocator::GetCurrentAddress() const
{
	return _addressSpaceStart + _usedByteCount;
}

const char* udtVMLinearAllocator::GetName() const
{
	return _name;
}

u32 udtVMLinearAllocator::GetResizeCount() const
{
	return _resizeCount;
}

const char* udtVMLinearAllocator::GetStringAt(uptr offset) const
{
	assert(offset < _usedByteCount);

	return (const char*)_addressSpaceStart + offset;
}

char* udtVMLinearAllocator::GetWriteStringAt(uptr offset) const
{
	assert(offset < _usedByteCount);

	return (char*)_addressSpaceStart + offset;
}

u8* udtVMLinearAllocator::GetAddressAt(uptr offset) const
{
	return _addressSpaceStart + offset;
}

void udtVMLinearAllocator::SetAlignment(u32 alignment)
{
	_alignment = alignment;
}

void udtVMLinearAllocator::SetName(const char* name)
{
	_name = name;
}

void udtVMLinearAllocator::Destroy()
{
	if(_addressSpaceStart == NULL)
	{
		return;
	}

	VirtualMemoryDecommitAndRelease(_addressSpaceStart, _reservedByteCount);
	_addressSpaceStart = NULL;
	_usedByteCount = 0;
	_reservedByteCount = 0;
	_commitByteCountGranularity = 0;
	_committedByteCount = 0;
}
