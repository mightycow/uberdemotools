#include "parser.hpp"
#include "file_stream.hpp"
#include "shared.hpp"
#include "utils.hpp"
#include "analysis_splitter.hpp"
#include "stack_trace.hpp"

#include <stdio.h>
#include <stdlib.h>


static bool RunDemoSplitter(const char* filePath)
{
	udtParseArg info;
	memset(&info, 0, sizeof(info));
	info.MessageCb = &CallbackConsoleMessage;
	info.ProgressCb = &CallbackConsoleProgress;

	udtParserContext* const context = udtCreateContext();
	if(context == NULL)
	{
		fprintf(stderr, "udtCreateContext failed.\n");
		return false;
	}

	const s32 errorCode = udtSplitDemoFile(context, &info, filePath);
	const bool success = errorCode == udtErrorCode::None;
	if(!success)
	{
		fprintf(stderr, "udtSplitDemoFile failed with error: '%s'.\n", udtGetErrorCodeString(errorCode));
	}
	udtDestroyContext(context);

	return success;
}

void PrintHelp()
{
	printf("UDT_splitter creates one output demo per gamestate message for demos\n");
	printf("with multiple gamestate messages.\n");
	printf("\n");
	printf("UDT_splitter [-q] demofilepath\n");
	printf("\n");
	printf("-q  quiet mode: no logging to stdout  (default: off)\n");
}

int udt_main(int argc, char** argv)
{
	if(argc < 2)
	{
		PrintHelp();
		return 0;
	}

	if(!udtFileStream::Exists(argv[1]))
	{
		fprintf(stderr, "Invalid demo file path.\n");
		return 1;
	}

	if(!RunDemoSplitter(argv[1]))
	{
		return 1;
	}

	return 0;
}
