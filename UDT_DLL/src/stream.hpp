#pragma once


#include "context.hpp"
#include "string.hpp"


struct udtSeekOrigin
{
	enum Id
	{
		Current = 1,
		End = 2,
		Start = 0
	};
};

struct udtStream
{
	udtStream() {}
	virtual ~udtStream() {}

	virtual u32 Read(void* dstBuff, u32 elementSize, u32 count) = 0; // Element count successfully read.
	virtual u32 Write(const void* srcBuff, u32 elementSize, u32 count) = 0; // Element count successfully written.
	virtual s32 Seek(s32 offset, udtSeekOrigin::Id origin) = 0; // 0 for success.
	virtual s32 Offset() = 0; // -1 for failure.
	virtual u64 Length() = 0; // -1 for failure.
	virtual s32 Close() = 0; // 0 for success. Must be safe to call more than once.

	uptr      ReadAll(udtVMLinearAllocator& allocator);
	udtString ReadAllAsString(udtVMLinearAllocator& allocator); // Will allocate and set the trailing NULL terminator.
};
