#pragma once


#include "api.h"


extern void SetCrashHandler(udtCrashCallback crashHandler);
extern void FatalError(const char* message);
