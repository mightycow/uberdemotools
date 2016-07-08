#pragma once


#include "uberdemotools.h"
#include "string.hpp"


namespace Log
{
	struct Level
	{
		enum Id
		{
			Info,
			Warning,
			Error,
			Count
		};
	};

	extern void        Init();
	extern void        Destroy();
	extern void        LogMessage(Level::Id level, const char* format, ...); // Calls Lock and Unlock.
	extern void        LogInfo(const char* format, ...);    // Calls Lock and Unlock.
	extern void        LogWarning(const char* format, ...); // Calls Lock and Unlock.
	extern void        LogError(const char* format, ...);   // Calls Lock and Unlock.
	extern u32         GetMessageCount();
	extern const char* GetMessageString(u32 index);
	extern u32         GetMessageLevel(u32 index);
	extern void        ShiftOffset(s32 amount);
	extern u32         GetOffset();
	extern void        Lock();
	extern void        Unlock();
}

extern udtString FormatTime(udtVMLinearAllocator& allocator, u64 milliSeconds);
