#include "crash.hpp"
#include "api.h"

#include <stdio.h>
#include <stdlib.h>


static void DefaultCrashCallback(const char* message)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Fatal error: %s\n", message);
	exit(666);
}

udtCrashCallback CrashHandler = &DefaultCrashCallback;

void SetCrashHandler(udtCrashCallback crashHandler)
{
	CrashHandler = crashHandler == NULL ? &DefaultCrashCallback : crashHandler;
}

void FatalError(const char* message)
{
	(*CrashHandler)(message);
}
