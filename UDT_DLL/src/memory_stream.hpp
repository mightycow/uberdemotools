#pragma once


#include "stream.hpp"
#include "array.hpp"


struct udtReadOnlyMemoryStream : udtStream
{
public:
	udtReadOnlyMemoryStream();
	~udtReadOnlyMemoryStream();

	bool   Open(const void* buffer, u32 byteCount);

	u32    Read(void* dstBuff, u32 elementSize, u32 count) override;
	u32    Write(const void* srcBuff, u32 elementSize, u32 count) override;
	s32	   Seek(s32 offset, udtSeekOrigin::Id origin) override;
	s32	   Offset() override;
	u64    Length() override;
	s32    Close() override;

private:
	s32    SetOffsetIfValid(s32 newOffset);

	const u8* _buffer;
	u32 _byteCount;
	u32 _readIndex;
};

struct udtVMMemoryStream : udtStream
{
public:
	udtVMMemoryStream();
	~udtVMMemoryStream();

	bool   Open(uptr reservedByteCount);
	u8*    GetBuffer();
	void   Clear();

	u32    Read(void* dstBuff, u32 elementSize, u32 count) override;
	u32    Write(const void* srcBuff, u32 elementSize, u32 count) override;
	s32	   Seek(s32 offset, udtSeekOrigin::Id origin) override;
	s32	   Offset() override;
	u64    Length() override;
	s32    Close() override;

private:
	s32    SetOffsetIfValid(s32 newOffset);

	udtVMArrayWithAlloc<u8> _buffer;
	u32 _offset;
};
