#pragma once


#include "uberdemotools.h"
#include "intrusive_list.hpp"


#define    UDT_MEMORY_PAGE_SIZE    4096
#define    UDT_KB(x)               (x << 10)
#define    UDT_MB(x)               (x << 20)
#define    UDT_GB(x)               (x << 30)


//
// A linear allocator using virtual memory to avoid 
// relocating data too often when growing.
//
struct udtVMLinearAllocator
{
public:
	struct Stats
	{
		uptr ReservedByteCount;
		uptr CommittedByteCount;
		uptr UsedByteCount;
		u32 AllocatorCount;
		u32 ResizeCount;
	};
	
	static void GetThreadStats(Stats& stats);
	static void GetThreadAllocators(u32& allocatorCount, udtVMLinearAllocator** allocators);

public:
	udtVMLinearAllocator(const char* name = nullptr);
	~udtVMLinearAllocator();

	void        Init(uptr reservedByteCount);
	uptr        Allocate(uptr byteCount);
	u8*         AllocateAndGetAddress(uptr byteCount);
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
	u32         GetResizeCount() const;
	const char* GetStringAt(uptr offset) const;
	char*       GetWriteStringAt(uptr offset) const;
	u8*         GetAddressAt(uptr offset) const;
	void        SetAlignment(u32 alignment);
	void        SetName(const char* name);

private:
	UDT_NO_COPY_SEMANTICS(udtVMLinearAllocator);

	uptr AllocateWithRelocation(uptr byteCount);
	uptr ComputeNewReservedByteCount();
	void Destroy();

	udtIntrusiveListNode _listNode;
	uptr _usedByteCount; // The index of the first free byte, if any.
	uptr _reservedByteCount;
	uptr _commitByteCountGranularity;
	uptr _committedByteCount;
	uptr _peakUsedByteCount;
	u8* _addressSpaceStart;
	const char* _name;
	u32 _resizeCount;
	u32 _alignment;
};
