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

struct PlatformActionType
{
	enum Id
	{
		Quit,
		Minimize,
		Maximize,
		OffsetWindow,
		Count
	};
};

extern void Platform_RequestDraw(Platform& platform);
extern void Platform_RequestQuit(Platform& platform);
extern void Platform_GetSharedDataPointers(Platform& platform, const PlatformReadOnly** readOnly, PlatformReadWrite** readWrite);
extern void Platform_SetCursorCapture(Platform& platform, bool enabled);
extern void Platform_NVGBeginFrame(Platform& platform);
extern void Platform_NVGEndFrame(Platform& platform);
extern void Platform_Draw(Platform& platform);
extern void Platform_DebugPrint(const char* format, ...);
