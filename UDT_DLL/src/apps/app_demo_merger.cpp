#include "parser.hpp"
#include "shared.hpp"
#include "stack_trace.hpp"
#include "utils.hpp"

#include <stdio.h>
#include <stdlib.h>


static void PrintHelp()
{
	printf("\n");
	printf("UDT_merger demo_path_1 demo_path_2 [demo_path_3 ... demo_path_n]\n");
}

static int MergeDemos(char** filePaths, int fileCount)
{
	s32 cancel = 0;

	udtParseArg info;
	memset(&info, 0, sizeof(info));
	info.CancelOperation = &cancel;
	info.MessageCb = &CallbackConsoleMessage;
	info.ProgressCb = &CallbackConsoleProgress;

	return (udtMergeDemoFiles(&info, (const char**)filePaths, (u32)fileCount) == (s32)udtErrorCode::None) ? 0 : __LINE__;
}

int udt_main(int argc, char** argv)
{
	if(argc < 2)
	{
		PrintHelp();
		return 1;
	}

	return MergeDemos(argv + 1, argc - 1);
}
