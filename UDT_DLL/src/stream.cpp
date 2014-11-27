#include "stream.hpp"
#include "utils.hpp"


u8* udtStream::ReadAll(udtVMLinearAllocator& allocator)
{
	const u32 length = (u32)Length();
	if(length == 0)
	{
		return NULL;
	}

	u8* const data = allocator.Allocate(length);
	if(data == NULL)
	{
		return NULL;
	}

	if(Read(data, length, 1) != 1)
	{
		return NULL;
	}

	return (u8*)data;
}

char* udtStream::ReadAllAsString(udtVMLinearAllocator& allocator)
{
	const u32 length = (u32)Length();
	if(length == 0)
	{
		return NULL;
	}

	char* const data = AllocateSpaceForString(allocator, length);
	if(data == NULL)
	{
		return NULL;
	}

	if(Read(data, length, 1) != 1)
	{
		return NULL;
	}

	data[length] = '\0';

	return data;
}
