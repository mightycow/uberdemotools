#include "stack_trace.hpp"
#include "types.hpp"


#if defined(UDT_WINDOWS)


void PrintStackTrace(int /*skipCount*/, const char* /*executableName*/)
{
}


#else


#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>


void PrintStackTrace(int skipCount, const char* executableName)
{
	void* addresses[16];
	const int symbolCount = backtrace(addresses, UDT_COUNT_OF(addresses));
	if(skipCount >= symbolCount)
	{
		return;
	}

	char** const messages = backtrace_symbols(addresses, symbolCount);
	if(messages == NULL)
	{
		return;
	}

	printf("Stack trace:\n");
	for(int i = skipCount; i < symbolCount; ++i)
	{
		printf("#%d %s\n", i, messages[i]);

		if(executableName == NULL)
		{
			continue;
		}

		char callBuffer[256];
		sprintf(callBuffer,"addr2line %p -e %s", addresses[i], executableName);
		system(callBuffer);
	}

	free(messages);
}


#endif
