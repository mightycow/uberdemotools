#include "crash.hpp"
#include "utils.hpp"
#include "api.h"

#include <stdio.h>
#include <stdlib.h>


static void DefaultCrashCallback(const char* message)
{
	fprintf(stderr, "\n");
	fprintf(stderr, message);
	exit(666);
}

udtCrashCallback CrashHandler = &DefaultCrashCallback;

void SetCrashHandler(udtCrashCallback crashHandler)
{
	CrashHandler = crashHandler == NULL ? &DefaultCrashCallback : crashHandler;
}

void FatalError(const char* file, int line, const char* function, const char* message)
{
	const size_t fileLength = strlen(file);
	const char* fileName = file;
	u32 sepIdx = 0;
	if(StringFindLastCharacterInList(sepIdx, file, "/\\") && sepIdx < fileLength - 1)
	{
		fileName = file + sepIdx + 1;
	}

	char formattedMsg[512];
	sprintf(formattedMsg, "FATAL ERROR\nFile: %s, line: %d\nFunction: %s\n%s", fileName, line, function, message);

	(*CrashHandler)(formattedMsg);
}
