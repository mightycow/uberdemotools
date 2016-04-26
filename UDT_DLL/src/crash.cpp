#include "crash.hpp"
#include "utils.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


static void DefaultCrashCallback(const char* message)
{
	// @NOTE: The "%s" trick is used to avoid bugs in case 
	// message contains one or more format specifiers.
	fprintf(stderr, "\n");
	fprintf(stderr, "%s", message);
	exit(666);
}

udtCrashCallback CrashHandler = &DefaultCrashCallback;

void SetCrashHandler(udtCrashCallback crashHandler)
{
	CrashHandler = crashHandler == NULL ? &DefaultCrashCallback : crashHandler;
}

void FatalError(const char* file, int line, const char* function, UDT_PRINTF_FORMAT_ARG const char* msgFormat, ...)
{
	const udtString fileString = udtString::NewConstRef(file);
	const char* fileName = file;
	u32 sepIdx = 0;
	if(udtString::FindLastCharacterListMatch(sepIdx, fileString, udtString::NewConstRef("/\\")) && 
	   sepIdx < fileString.GetLength() - 1)
	{
		fileName = file + sepIdx + 1;
	}

	char originalMsg[512];
	va_list argptr;
	va_start(argptr, msgFormat);
	vsprintf(originalMsg, msgFormat, argptr);
	va_end(argptr);

	char formattedMsg[1024];
	sprintf(formattedMsg, "FATAL ERROR\nFile: %s, line: %d\nFunction: %s\n%s", fileName, line, function, originalMsg);

	(*CrashHandler)(formattedMsg);
}
