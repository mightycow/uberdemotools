#pragma once


#include "allocator.hpp"
#include "intrusive_list.hpp"


#define    UDT_MEMORY_PAGE_SIZE    4096


//
// A virtual-memory based linear allocator.
// It pointer bumps like any linear allocator, 
// but can grow and never needs to move old allocations.
//
struct udtVMLinearAllocator// : udtAllocator
{
public:
	struct Stats
	{
		uptr ReservedByteCount;
		uptr CommittedByteCount;
		uptr UsedByteCount;
		u32 AllocatorCount;
	};
	
	static void GetThreadStats(Stats& stats);
	static void GetThreadAllocators(u32& allocatorCount, udtVMLinearAllocator** allocators);

public:
	udtVMLinearAllocator();
	~udtVMLinearAllocator();

	bool        Init(uptr reservedByteCount, const char* name);
	u8*         Allocate(uptr byteCount);
	void        Pop(uptr byteCount);
	void        Clear(); // Only resets the index to the first free byte.
	void        Purge(); // De-commit all unused memory pages.
	void        SetCurrentByteCount(uptr byteCount); // Has to be less or equal to the currently committed byte count.
	uptr        GetCurrentByteCount() const;
	uptr        GetCommittedByteCount() const;
	uptr        GetPeakUsedByteCount() const;
	uptr        GetReservedByteCount() const;
	u8*         GetStartAddress() const;
	u8*         GetCurrentAddress() const;
	const char* GetName() const;

private:
	UDT_NO_COPY_SEMANTICS(udtVMLinearAllocator);

	void    Destroy();

	udtIntrusiveListNode _listNode;
	u8* _addressSpaceStart;
	uptr _firstFreeByteIndex; // In other words: the used byte count.
	uptr _reservedByteCount;
	uptr _commitByteCountGranularity;
	uptr _committedByteCount;
	uptr _peakUsedByteCount;
	const char* _name;
};
