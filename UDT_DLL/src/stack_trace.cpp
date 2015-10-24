#include "stack_trace.hpp"
#include "uberdemotools.h"
#include "macros.hpp"


#if defined(UDT_WINDOWS)


void PrintStackTrace(FILE* /*file*/, int /*skipCount*/, const char* /*executableName*/)
{
}


#else


#include <stdlib.h>
#include <execinfo.h>


void PrintStackTrace(FILE* file, int skipCount, const char* executableName)
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

	fprintf(file, "Stack trace:\n");
	for(int i = skipCount; i < symbolCount; ++i)
	{
		fprintf(file, "#%d %s\n", i, messages[i]);

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
