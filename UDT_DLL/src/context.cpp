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

	char msg[1024];
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

	char msg[1024];
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

	char msg[1024];
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

	char msg[1024];
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

udtProtocolConverter* udtContext::GetProtocolConverter(udtProtocol::Id outProtocol, udtProtocol::Id inProtocol)
{
	if(outProtocol == inProtocol)
	{
		_converterIdentity.SetProtocol(inProtocol);
		return &_converterIdentity;
	}

	if(outProtocol == udtProtocol::Dm68 && inProtocol == udtProtocol::Dm3)
	{
		return &_converter3to68;
	}

	if(outProtocol == udtProtocol::Dm68 && inProtocol == udtProtocol::Dm48)
	{
		return &_converter48to68;
	}

	if(outProtocol == udtProtocol::Dm91 && inProtocol == udtProtocol::Dm73)
	{
		return &_converter73to91;
	}

	if(outProtocol == udtProtocol::Dm91 && inProtocol == udtProtocol::Dm90)
	{
		return &_converter90to91;
	}

	// Please sanitize the input before asking the library to do something it can't.
	FatalError(__FILE__, __LINE__, __FUNCTION__, "Invalid input/output protocol combo");

	return NULL;
}
