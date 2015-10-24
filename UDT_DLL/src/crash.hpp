#pragma once


#include "uberdemotools.h"
#include "macros.hpp"


extern void SetCrashHandler(udtCrashCallback crashHandler);
extern void FatalError(const char* file, int line, const char* function, UDT_PRINTF_FORMAT_ARG const char* msgFormat, ...) UDT_PRINTF_POST_FUNCTION(4, 5);
