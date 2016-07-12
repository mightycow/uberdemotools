#include "log.hpp"
#include "array.hpp"
#include "utils.hpp"
#include "platform.hpp"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <new>


struct Message
{
	u32 String;
	u32 Level;
};

struct LogSystem
{
	LogSystem()
	{
		Platform_CreateCriticalSection(CriticalSection);
	}

	~LogSystem()
	{
		Platform_ReleaseCriticalSection(CriticalSection);
	}

	char TempMessageBuffer[4096];
	udtVMArray<Message> Messages { "LogSystem::LogMessagesArray" };
	udtVMLinearAllocator StringAllocator { "LogSystem::Strings" };
	CriticalSectionId CriticalSection = InvalidCriticalSectionId;
	u32 Offset = 0;
};

static LogSystem* LogSystemInstance = nullptr;


namespace Log
{
	void Init()
	{
		LogSystemInstance = (LogSystem*)malloc(sizeof(LogSystem));
		new (LogSystemInstance) LogSystem;
	}

	void Destroy()
	{
		LogSystemInstance->~LogSystem();
		free(LogSystemInstance);
	}

	static void LogMessage(Level::Id level, const char* format, va_list args)
	{
		Lock();

		char* const msg = LogSystemInstance->TempMessageBuffer;
		vsprintf(msg, format, args);

		Message message;
		message.String = udtString::NewClone(LogSystemInstance->StringAllocator, msg).GetOffset();
		message.Level = (u32)level;
		LogSystemInstance->Messages.Add(message);
		if(LogSystemInstance->Offset > 0)
		{
			ShiftOffset(1);
		}

		Unlock();
	}

	void LogMessage(Level::Id level, const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		LogMessage(level, format, args);
		va_end(args);
	}

	void LogInfo(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		LogMessage(Level::Info, format, args);
		va_end(args);
	}

	void LogWarning(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		LogMessage(Level::Warning, format, args);
		va_end(args);
	}

	void LogError(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		LogMessage(Level::Error, format, args);
		va_end(args);
	}

	u32 GetMessageCount()
	{
		return LogSystemInstance->Messages.GetSize();
	}

	const char* GetMessageString(u32 index)
	{
		return LogSystemInstance->StringAllocator.GetStringAt(LogSystemInstance->Messages[index].String);
	}

	u32 GetMessageLevel(u32 index)
	{
		return LogSystemInstance->Messages[index].Level;
	}

	void ShiftOffset(s32 amount)
	{
		LogSystemInstance->Offset = (s32)udt_clamp((s32)LogSystemInstance->Offset + amount, 0, (s32)LogSystemInstance->Messages.GetSize() - 1);
	}

	u32 GetOffset()
	{
		return LogSystemInstance->Offset;
	}

	void Lock()
	{
		Platform_EnterCriticalSection(LogSystemInstance->CriticalSection);
	}

	void Unlock()
	{
		Platform_LeaveCriticalSection(LogSystemInstance->CriticalSection);
	}
}

udtString FormatTime(udtVMLinearAllocator& allocator, u64 milliSeconds)
{
	if(milliSeconds == 0)
	{
		return udtString::NewConstRef("0ms");
	}

	char time[64];
	if(milliSeconds < 1000)
	{
		sprintf(time, "%dms", (int)milliSeconds);
	}
	else
	{
		sprintf(time, "%.3fs", (float)milliSeconds / 1000.0f);
	}

	return udtString::NewClone(allocator, time);
}
