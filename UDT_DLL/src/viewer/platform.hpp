#pragma once


#include "uberdemotools.h"
#include "shared.hpp"


struct Platform;
struct NVGcontext;

struct PlatformReadOnly
{
	NVGcontext* NVGContext;
};

struct PlatformReadWrite
{
};

#define  InvalidCriticalSectionId  nullptr
typedef void* CriticalSectionId;

typedef void (*Platform_ThreadFunc)(void* userData);

extern void Platform_RequestQuit(Platform& platform);
extern void Platform_GetSharedDataPointers(Platform& platform, const PlatformReadOnly** readOnly, PlatformReadWrite** readWrite);
extern void Platform_SetCursorCapture(Platform& platform, bool enabled);
extern void Platform_NVGBeginFrame(Platform& platform);
extern void Platform_NVGEndFrame(Platform& platform);
extern void Platform_ToggleMaximized(Platform& platform);
extern void Platform_GetCursorPosition(Platform& platform, s32& x, s32& y);
extern void Platform_DebugPrint(const char* format, ...);
extern void Platform_NewThread(Platform_ThreadFunc userEntryPoint, void* userData);
extern void Platform_CreateCriticalSection(CriticalSectionId& cs);
extern void Platform_ReleaseCriticalSection(CriticalSectionId cs);
extern void Platform_EnterCriticalSection(CriticalSectionId cs);
extern void Platform_LeaveCriticalSection(CriticalSectionId cs);
extern void Platform_PrintError(const char* format, ...); // Show error message.
extern void Platform_FatalError(const char* format, ...); // Show error message and stack trace, then quit.
