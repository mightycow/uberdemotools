#include "parser.hpp"
#include "shared.hpp"
#include "stack_trace.hpp"
#include "utils.hpp"

#include <stdio.h>
#include <stdlib.h>


void PrintHelp()
{
	printf("For a set of two or more demos, creates a new demo using the entitystate\n");
	printf("and playerstate snapshot data of all the specified demos.\n");
	printf("\n");
	printf("UDT_merger [-q] demopath1 demopath2 [demopath3 ... demopathn]\n");
	printf("\n");
	printf("-q  quiet mode: no logging to stdout  (default: off)\n");
	printf("\n");
	printf("Demo merging only works for matching server times.\n");
	printf("In other words, you should only try to merge demos from the same match.\n");
	printf("\n");
	printf("The first-person view of the output demo will be that of the first demo\n");
	printf("specified in the argument list.\n");
}

static bool MergeDemos(char** filePaths, int fileCount)
{
	s32 cancel = 0;

	udtParseArg info;
	memset(&info, 0, sizeof(info));
	info.CancelOperation = &cancel;
	info.MessageCb = &CallbackConsoleMessage;
	info.ProgressCb = &CallbackConsoleProgress;

	const s32 errorCode = udtMergeDemoFiles(&info, (const char**)filePaths, (u32)fileCount);
	if(errorCode != (s32)udtErrorCode::None)
	{
		fprintf(stderr, "udtMergeDemoFiles failed with error: '%s'\n", udtGetErrorCodeString(errorCode));
		return false;
	}

	return true;
}

int udt_main(int argc, char** argv)
{
	if(argc < 2)
	{
		PrintHelp();
		return 0;
	}

	if(!MergeDemos(argv + 1, argc - 1))
	{
		return 1;
	}

	return 0;
}
