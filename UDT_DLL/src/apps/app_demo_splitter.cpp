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
		return false;
	}

	const bool result = udtSplitDemoFile(context, &info, filePath) == udtErrorCode::None;
	udtDestroyContext(context);

	return result;
}

static void PrintHelp()
{
	printf("???? help for UDT_splitter ????\n");
	printf("Syntax: UDT_splitter demo_path\n");
}

static void CrashHandler(const char* message)
{
	fprintf(stderr, "\n");
	fprintf(stderr, message);
	fprintf(stderr, "\n");

	PrintStackTrace(3, "UDT_splitter");

	exit(666);
}

int main(int argc, char** argv)
{
	ResetCurrentDirectory(argv[0]);

	if(argc < 2)
	{
		printf("Not enough arguments.\n");
		PrintHelp();
		return 1;
	}

	if(!udtFileStream::Exists(argv[1]))
	{
		printf("Invalid file path.\n");
		PrintHelp();
		return 2;
	}

	udtSetCrashHandler(&CrashHandler);

	const bool success = RunDemoSplitter(argv[1]);

	return success ? 0 : 666;
}
