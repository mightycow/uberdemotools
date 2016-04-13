#include "stream.hpp"
#include "utils.hpp"


uptr udtStream::ReadAll(udtVMLinearAllocator& allocator)
{
	const u32 length = (u32)Length();
	if(length == 0)
	{
		return uptr(~0);
	}

	const uptr offset = allocator.Allocate(length);
	u8* const data = allocator.GetAddressAt(offset);
	if(Read(data, length, 1) != 1)
	{
		return uptr(~0);
	}

	return offset;
}

udtString udtStream::ReadAllAsString(udtVMLinearAllocator& allocator)
{
	const u32 length = (u32)Length();
	if(length == 0)
	{
		return udtString::NewNull();
	}

	udtString result = udtString::NewEmpty(allocator, length + 1);
	char* const resultPtr = result.GetWritePtr();
	if(Read(resultPtr, length, 1) != 1)
	{
		return udtString::NewNull();
	}
	result.SetLength(length);
	resultPtr[length] = '\0';

	return result;
}
