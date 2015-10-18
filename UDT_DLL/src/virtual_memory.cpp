#include "virtual_memory.hpp"
#include "macros.hpp"


#if defined(_WIN32)


#include <Windows.h>


void* VirtualMemoryReserve(uptr byteCount)
{
	return VirtualAlloc(NULL, (SIZE_T)byteCount, MEM_RESERVE, PAGE_READWRITE);
}

bool VirtualMemoryCommit(void* address, uptr byteCount)
{
	return VirtualAlloc((LPVOID)address, (SIZE_T)byteCount, MEM_COMMIT, PAGE_READWRITE) != NULL;
}

bool VirtualMemoryDecommit(void* address, uptr byteCount)
{
	return VirtualFree((LPVOID)address, (SIZE_T)byteCount, MEM_DECOMMIT) != FALSE;
}

bool VirtualMemoryDecommitAndRelease(void* address, uptr /*byteCount*/)
{
	return VirtualFree((LPVOID)address, 0, MEM_RELEASE) != FALSE;
}


#else


#include <sys/mman.h>
#include <fcntl.h>


void* VirtualMemoryReserve(uptr byteCount)
{
	// "some implementations require fd to be -1 if MAP_ANONYMOUS is specified, and portable applications should ensure this."
	// void* const address = mmap(NULL, (size_t)byteCount, PROT_NONE, MAP_ANONYMOUS, -1, 0);
	// @NOTE: Unfortunately, the code above didn't work on the Linux I tried it on...

	const int fd = open("/dev/zero", O_RDWR);
	void* const address = mmap(NULL, (size_t)byteCount, PROT_NONE, MAP_PRIVATE | MAP_FILE, fd, 0);
	if(address == MAP_FAILED)
	{
		return NULL;
	}

	return address;
}

bool VirtualMemoryCommit(void* address, uptr byteCount)
{
	return mprotect(address, (size_t)byteCount, PROT_READ | PROT_WRITE) == 0;
}

bool VirtualMemoryDecommit(void* address, uptr byteCount)
{
	return mprotect(address, (size_t)byteCount, PROT_NONE) == 0;
}

bool VirtualMemoryDecommitAndRelease(void* address, uptr byteCount)
{
	return munmap(address, (size_t)byteCount) == 0;
}


#endif
