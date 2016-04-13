#include "parser.hpp"
#include "shared.hpp"
#include "stack_trace.hpp"
#include "utils.hpp"

#include <stdio.h>
#include <stdlib.h>


void PrintHelp()
{
	printf("Applies a sort of anti-lag to make the first-person view of demos look\n");
	printf("more like what the player saw when he was playing (especially for CPMA LG).\n");
	printf("\n");
	printf("UDT_timeshifter [-q] [-s=snapshots] filepath\n");
	printf("\n");
	printf("-q    quiet mode: no logging to stdout  (default: off)\n");
	printf("-s=N  set the snapshot count to N       (default: 2, min: 1, max: 8)\n");
}

static bool TimeShiftDemos(s32 snapshotCount, const char* filePath)
{
	s32 outputErrorCode = 0;
	s32 cancel = 0;

	udtParseArg info;
	memset(&info, 0, sizeof(info));
	info.CancelOperation = &cancel;
	info.MessageCb = &CallbackConsoleMessage;
	info.ProgressCb = &CallbackConsoleProgress;

	udtMultiParseArg extraInfo;
	memset(&extraInfo, 0, sizeof(extraInfo));
	extraInfo.FileCount = 1;
	extraInfo.FilePaths = &filePath;
	extraInfo.OutputErrorCodes = &outputErrorCode;

	udtTimeShiftArg timeShiftArg;
	memset(&timeShiftArg, 0, sizeof(timeShiftArg));
	timeShiftArg.SnapshotCount = snapshotCount;

	const s32 errorCode = udtTimeShiftDemoFiles(&info, &extraInfo, &timeShiftArg);
	if(errorCode != (s32)udtErrorCode::None)
	{
		fprintf(stderr, "udtTimeShiftDemoFiles failed with error: '%s'.\n", udtGetErrorCodeString(errorCode));
		return false;
	}

	return true;
}

int udt_main(int argc, char** argv)
{
	if(argc == 1)
	{
		PrintHelp();
		return 0;
	}

	const char* const inputPath = argv[argc - 1];
	s32 snapshotCount = 2;
	for(int i = 1; i < argc - 1; ++i)
	{
		s32 localSnapshotCount = 2;
		const udtString arg = udtString::NewConstRef(argv[i]);
		if(udtString::StartsWith(arg, "-s=") &&
		   arg.GetLength() >= 4 &&
		   StringParseInt(localSnapshotCount, arg.GetPtr() + 3) &&
		   localSnapshotCount >= 1 &&
		   localSnapshotCount <= 8)
		{
			snapshotCount = (u32)localSnapshotCount;
		}
	}

	if(!TimeShiftDemos(snapshotCount, inputPath))
	{
		return 1;
	}

	return 0;
}
