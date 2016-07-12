#include "memory_stream.hpp"


udtReadOnlyMemoryStream::udtReadOnlyMemoryStream()
{
	_buffer = NULL;
	_byteCount = 0;
	_readIndex = 0;
}

udtReadOnlyMemoryStream::~udtReadOnlyMemoryStream()
{
}

bool udtReadOnlyMemoryStream::Open(const void* buffer, u32 byteCount)
{
	if(buffer == NULL)
	{
		return false;
	}

	_buffer = (const u8*)buffer;
	_byteCount = byteCount;
	_readIndex = 0;

	return true;
}

u32 udtReadOnlyMemoryStream::Read(void* dstBuff, u32 elementSize, u32 count)
{
	const u32 byteCount = elementSize * count;
	if(_readIndex + byteCount > _byteCount)
	{
		return 0;
	}

	memcpy(dstBuff, _buffer + _readIndex, (size_t)byteCount);
	_readIndex += byteCount;

	return count;
}

u32 udtReadOnlyMemoryStream::Write(const void* /*srcBuff*/, u32 /*elementSize*/, u32 /*count*/)
{
	return 0;
}

s32 udtReadOnlyMemoryStream::Seek(s32 offset, udtSeekOrigin::Id origin)
{
	switch(origin)
	{
		case udtSeekOrigin::Start: return SetOffsetIfValid(offset);
		case udtSeekOrigin::Current: return SetOffsetIfValid((s32)_readIndex + offset);
		case udtSeekOrigin::End: return SetOffsetIfValid((s32)_byteCount - offset);
		default: return -1;
	}
}

s32 udtReadOnlyMemoryStream::Offset()
{
	return (s32)_readIndex;
}

u64 udtReadOnlyMemoryStream::Length()
{
	return (u64)_byteCount;
}

s32 udtReadOnlyMemoryStream::Close()
{
	return 0;
}

s32 udtReadOnlyMemoryStream::SetOffsetIfValid(s32 newOffset)
{
	if(newOffset < 0 || newOffset >= (s32)_byteCount)
	{
		return -1;
	}

	_readIndex = (u32)newOffset;

	return 0;
}

udtVMMemoryStream::udtVMMemoryStream()
{
	_offset = 0;
}

udtVMMemoryStream::~udtVMMemoryStream()
{
}

u8* udtVMMemoryStream::GetBuffer()
{
	return _buffer.GetStartAddress();
}

void udtVMMemoryStream::Clear()
{
	_buffer.Resize(0);
	_offset = 0;
}

u32 udtVMMemoryStream::Read(void* dstBuff, u32 elementSize, u32 count)
{
	const u32 byteCount = elementSize * count;
	if(_offset + byteCount > _buffer.GetSize())
	{
		return 0;
	}

	memcpy(dstBuff, _buffer.GetStartAddress() + _offset, (size_t)byteCount);
	_offset += byteCount;

	return count;
}

u32 udtVMMemoryStream::Write(const void* srcBuff, u32 elementSize, u32 count)
{
	const u32 oldBufferLength = _buffer.GetSize();
	const u32 byteCount = elementSize * count;
	_buffer.Resize(oldBufferLength + byteCount);

	const u32 offset = _offset;
	u8* const writeAddress = _buffer.GetStartAddress() + offset;
	if(offset != oldBufferLength)
	{
		// We're not appending, so move some of the data up to make room for what we're about to copy.
		memmove(writeAddress, writeAddress + byteCount, (size_t)(oldBufferLength - offset));
	}

	memcpy(writeAddress, srcBuff, (size_t)byteCount);
	_offset += byteCount;

	return count;
}

s32 udtVMMemoryStream::Seek(s32 offset, udtSeekOrigin::Id origin)
{
	switch(origin)
	{
		case udtSeekOrigin::Start: return SetOffsetIfValid(offset);
		case udtSeekOrigin::Current: return SetOffsetIfValid((s32)_offset + offset);
		case udtSeekOrigin::End: return SetOffsetIfValid((s32)_buffer.GetSize() - offset);
		default: return -1;
	}
}

s32 udtVMMemoryStream::Offset()
{
	return (s32)_offset;
}

u64 udtVMMemoryStream::Length()
{
	return (u64)_buffer.GetSize();
}

s32 udtVMMemoryStream::Close()
{
	return 0;
}

s32 udtVMMemoryStream::SetOffsetIfValid(s32 newOffset)
{
	if(newOffset < 0 || newOffset >= (s32)_buffer.GetSize())
	{
		return -1;
	}

	_offset = (u32)newOffset;

	return 0;
}
