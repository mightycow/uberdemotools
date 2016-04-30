#pragma once


#include "uberdemotools.h"
#include "macros.hpp"
#include "tokenizer.hpp"
#include "linear_allocator.hpp"
#include "protocol_conversion.hpp"


// Don't ever allocate an instance of this on the stack.
struct udtContext
{
public:
	udtContext();
	~udtContext();

	bool SetCallbacks(udtMessageCallback messageCb, udtProgressCallback progressCb, void* progressContext);
	void Reset();
	void Destroy();

	void LogInfo(UDT_PRINTF_FORMAT_ARG const char* format, ...) const UDT_PRINTF_POST_FUNCTION(2, 3);
	void LogWarning(UDT_PRINTF_FORMAT_ARG const char* format, ...) const UDT_PRINTF_POST_FUNCTION(2, 3);
	void LogError(UDT_PRINTF_FORMAT_ARG const char* format, ...) const UDT_PRINTF_POST_FUNCTION(2, 3);
	void LogErrorAndCrash(UDT_PRINTF_FORMAT_ARG const char* format, ...) const UDT_PRINTF_POST_FUNCTION(2, 3);
	void NotifyProgress(f32 progress) const;

	udtProtocolConverter* GetProtocolConverter(udtProtocol::Id outProtocol, udtProtocol::Id inProtocol);

public:
	char        ReadStringBuffer[MAX_STRING_CHARS];
	char        ReadBigStringBuffer[BIG_INFO_STRING];
	idTokenizer Tokenizer;

private:
	udtMessageCallback  _messageCallback;  // Can be NULL.
	udtProgressCallback _progressCallback; // Can be NULL.
	void*               _progressContext;  // Can be NULL.

	udtProtocolConverter3to68    _converter3to68;
	udtProtocolConverter48to68   _converter48to68;
	udtProtocolConverter73to91   _converter73to91;
	udtProtocolConverter90to91   _converter90to91;
	udtProtocolConverterIdentity _converterIdentity;
};
