#pragma once


#include "common.hpp"
#include "string.hpp"


struct idTokenizer
{
public:
	const char* GetOriginalCommand() const;
	u32         GetArgCount() const;
	const char* GetArgString(u32 arg) const;
	u32         GetArgLength(u32 arg) const;
	u32         GetArgOffset(u32 arg) const;
	udtString   GetArg(u32 arg) const;
	void        Tokenize(const char* text, bool ignoreQuotes = false);

private:
	void        TokenizeImpl(const char* text, bool ignoreQuotes = false);
	void        RegisterArgLengths();

	u32   _argCount;
	char* _argStrings[MAX_STRING_TOKENS]; // Points into _tokenizedCommand.
	u32   _argLengths[MAX_STRING_TOKENS];
	u32   _argOffsets[MAX_STRING_TOKENS];
	char  _tokenizedCommand[BIG_INFO_STRING+MAX_STRING_TOKENS];	// Will have 0 bytes inserted.
	char  _originalCommand[BIG_INFO_STRING]; // The original command we received (no token processing).
};
