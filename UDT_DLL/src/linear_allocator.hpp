#pragma once


#include "allocator.hpp"


#define    UDT_MEMORY_PAGE_SIZE    4096


//
// A virtual-memory based linear allocator.
// It pointer bumps like any linear allocator, 
// but can grow and never needs to move old allocations.
//
struct udtVMLinearAllocator// : udtAllocator
{
public:
	udtVMLinearAllocator();
	~udtVMLinearAllocator();

	bool	Init(u32 reservedByteCount, u32 commitByteCountGranularity, bool commitFirstBlock = false);
	u8*		Allocate(u32 byteCount);
	void    Pop(u32 byteCount);
	void	Clear(); // Only resets the index to the first free byte.
	void    Purge(); // De-commit all unused memory pages.
	void	SetCurrentByteCount(u32 byteCount); // Has to be less or equal to the currently committed byte count.
	u32		GetCurrentByteCount() const;
	u32     GetCommittedByteCount() const;
	u8*     GetStartAddress() const;
	u8*     GetCurrentAddress() const;

private:
	UDT_NO_COPY_SEMANTICS(udtVMLinearAllocator);

	void	Destroy();

	u8* _addressSpaceStart;
	u32 _firstFreeByteIndex; // In other words: the used byte count.
	u32 _reservedByteCount;
	u32 _commitByteCountGranularity;
	u32 _committedByteCount;
};
