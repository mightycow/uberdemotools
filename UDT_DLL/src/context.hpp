#pragma once


#include "api.h"
#include "huffman.hpp"
#include "tokenizer.hpp"
#include "linear_allocator.hpp"
#include "protocol_conversion.hpp"


// Don't ever allocate an instance of this on the stack.
struct udtContext
{
public:
	typedef char (&ReadStringBufferRef)[MAX_STRING_CHARS];
	typedef char (&ReadBigStringBufferRef)[BIG_INFO_STRING];
	typedef char (&ReadStringLineBufferRef)[MAX_STRING_CHARS];

public:
	udtContext();
	~udtContext();

	bool	SetCallbacks(udtMessageCallback messageCb, udtProgressCallback progressCb, void* progressContext);
	void    Reset();
	void	Destroy();

	void	LogInfo(UDT_PRINTF_FORMAT_ARG const char* format, ...) const UDT_PRINTF_POST_FUNCTION(2, 3);
	void	LogWarning(UDT_PRINTF_FORMAT_ARG const char* format, ...) const UDT_PRINTF_POST_FUNCTION(2, 3);
	void	LogError(UDT_PRINTF_FORMAT_ARG const char* format, ...) const UDT_PRINTF_POST_FUNCTION(2, 3);
	void	LogErrorAndCrash(UDT_PRINTF_FORMAT_ARG const char* format, ...) const UDT_PRINTF_POST_FUNCTION(2, 3);
	void	NotifyProgress(f32 progress) const;

	udtProtocolConverter* GetProtocolConverter(udtProtocol::Id outProtocol, udtProtocol::Id inProtocol);

public:
	char	ReadStringBuffer[MAX_STRING_CHARS];
	char	ReadBigStringBuffer[BIG_INFO_STRING];
	char	ReadStringLineBuffer[MAX_STRING_CHARS];

	CommandLineTokenizer    Tokenizer;

private:
	udtMessageCallback      _messageCallback;  // Can be NULL.
	udtProgressCallback     _progressCallback; // Can be NULL.
	void*                   _progressContext;  // Can be NULL.

	udtProtocolConverter90to68_CPMA _converter90to68;
	udtProtocolConverter73to90      _converter73to90;
	udtProtocolConverterIdentity    _converter68;
	udtProtocolConverterIdentity    _converter73;
	udtProtocolConverterIdentity    _converter90;
};
