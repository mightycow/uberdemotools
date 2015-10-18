#include "system.hpp"
#include "macros.hpp"


#if defined(UDT_WINDOWS) && !(defined(__MINGW32__) || defined (__MINGW64__))


#include <Windows.h>


bool GetProcessorCoreCount(u32& coreCount)
{
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	DWORD bufferByteCount = 0;
	GetLogicalProcessorInformation(buffer, &bufferByteCount);
	if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		return false;
	}
		
	buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc((size_t)bufferByteCount);
	if(buffer == NULL)
	{
		return false;
	}

	if(GetLogicalProcessorInformation(buffer, &bufferByteCount) == FALSE)
	{
		free(buffer);
		return false;
	}

	u32 count = 0;
	const size_t elementCount = (size_t)bufferByteCount / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
	for(size_t i = 0; i < elementCount; ++i)
	{
		if(buffer[i].Relationship == RelationProcessorCore)
		{
			++count;
		}
	}

	free(buffer);

	if(count == 0)
	{
		return false;
	}

	coreCount = count;

	return true;
}


#elif defined(UDT_WINDOWS) && (defined(__MINGW32__) || defined (__MINGW64__))


bool GetProcessorCoreCount(u32&)
{
	return false;
}


#else


#include <unistd.h>


bool GetProcessorCoreCount(u32& coreCount)
{
	// "the number of processors which are currently online (i.e., available)"
	const long result = sysconf(_SC_NPROCESSORS_ONLN);
	if(result == -1)
	{
		return false;
	}

	coreCount = (u32)result;

	return true;
}


#endif
