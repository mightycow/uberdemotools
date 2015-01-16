#include "context.hpp"
#include "common.hpp"
#include "crash.hpp"

#if defined(UDT_MSVC) && defined(UDT_WINDOWS)
#	include <Windows.h> // IsDebuggerPresent, __debugbreak
#endif

#if defined(UDT_MSVC)
#	include <stdio.h>
#	include <stdarg.h>
#endif

#if defined(UDT_GCC)
#	include <stdarg.h>
#endif

#include <stdlib.h> // For exit.


udtContext::udtContext()
	: _messageCallback(NULL)
	, _progressCallback(NULL)
	, _progressContext(NULL)
{
}

udtContext::~udtContext()
{
	Destroy();
}

bool udtContext::SetCallbacks(udtMessageCallback messageCb, udtProgressCallback progressCb, void* progressContext)
{
	_messageCallback = messageCb;
	_progressCallback = progressCb;
	_progressContext = progressContext;

	return true;
}

void udtContext::Reset()
{
}

void udtContext::Destroy()
{
}

void udtContext::LogInfo(UDT_PRINTF_FORMAT_ARG const char* format, ...) const
{
	if(!_messageCallback)
	{
		return;
	}

	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg) - 1, format, argptr);
	va_end(argptr);

	(*_messageCallback)(0, msg);
}

void udtContext::LogWarning(UDT_PRINTF_FORMAT_ARG const char* format, ...) const
{
	if(!_messageCallback)
	{
		return;
	}

	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg) - 1, format, argptr);
	va_end(argptr);

	(*_messageCallback)(1, msg);
}

void udtContext::LogError(UDT_PRINTF_FORMAT_ARG const char* format, ...) const
{
	if(!_messageCallback)
	{
		return;
	}

	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg) - 1, format, argptr);
	va_end(argptr);

	(*_messageCallback)(2, msg);
}

void udtContext::LogErrorAndCrash(UDT_PRINTF_FORMAT_ARG const char* format, ...) const
{
	if(!_messageCallback)
	{
		return;
	}

	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg) - 1, format, argptr);
	va_end(argptr);

	(*_messageCallback)(3, msg);

#if defined(UDT_MSVC) && defined(UDT_WINDOWS)
#	if defined(UDT_DEBUG)
		UDT_DEBUG_BREAK();
#	else
		if(IsDebuggerPresent()) UDT_DEBUG_BREAK();
#	endif
#endif

	FatalError(__FILE__, __LINE__, __FUNCTION__, msg);
}

void udtContext::NotifyProgress(f32 progress) const
{
	if(_progressCallback != NULL)
	{
		(*_progressCallback)(progress, _progressContext);
	}
}
