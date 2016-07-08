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

	void        Init();
	void        LogMessage(Level::Id level, const char* format, ...);
	void        LogInfo(const char* format, ...);
	void        LogWarning(const char* format, ...);
	void        LogError(const char* format, ...);
	u32         GetMessageCount();
	const char* GetMessageString(u32 index);
	u32         GetMessageLevel(u32 index);
	void        ShiftOffset(s32 amount);
	u32         GetOffset();
}

extern udtString FormatTime(udtVMLinearAllocator& allocator, u64 milliSeconds);
