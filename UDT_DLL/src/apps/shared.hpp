#pragma once


#include "types.hpp"


extern void CallbackConsoleMessage(s32 logLevel, const char* message);
extern void CallbackConsoleProgress(f32 progress, void* userData);
