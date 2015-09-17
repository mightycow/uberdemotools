#include "tokenizer.hpp"

#include <stdlib.h>
#include <malloc.h>
#include <string.h>


static const char* EmptyString = "";


const char* idTokenizer::GetOriginalCommand() const
{
	return _originalCommand;
}

u32	idTokenizer::GetArgCount() const
{
	return _argCount;
}

const char* idTokenizer::GetArgString(u32 arg) const
{
	if(arg >= _argCount || arg >= MAX_STRING_TOKENS)
	{
		return EmptyString;
	}

	return _argStrings[arg];	
}

u32 idTokenizer::GetArgLength(u32 arg) const
{
	if(arg >= _argCount || arg >= MAX_STRING_TOKENS)
	{
		return 0;
	}

	return _argLengths[arg];
}

u32 idTokenizer::GetArgOffset(u32 arg) const
{
	if(arg >= _argCount || arg >= MAX_STRING_TOKENS)
	{
		return 0;
	}

	return _argOffsets[arg];
}

udtString idTokenizer::GetArg(u32 arg) const
{
	if(arg >= _argCount || arg >= MAX_STRING_TOKENS)
	{
		return udtString::NewEmptyConstant();
	}

	return udtString::NewConstRef(_argStrings[arg], _argLengths[arg]);
}

void idTokenizer::TokenizeImpl(const char* text, bool ignoreQuotes)
{
	// clear previous args
	_argCount = 0;

	if(!text)
		return;

	Q_strncpyz(_originalCommand, text, sizeof(_originalCommand));

	const char* const in = text;
	char* out = _tokenizedCommand;

	for(;;)
	{
		if(_argCount == MAX_STRING_TOKENS)
		{
			return;			// this is usually something malicious
		}

		for(;;)
		{
			// skip whitespace
			while(*text && *text <= ' ')
			{
				text++;
			}
			if(!*text)
			{
				return;			// all tokens parsed
			}

			// skip // comments
			if(text[0] == '/' && text[1] == '/')
			{
				return;			// all tokens parsed
			}

			// skip /* */ comments
			if(text[0] == '/' && text[1] == '*')
			{
				while(*text && (text[0] != '*' || text[1] != '/'))
				{
					text++;
				}
				if(!*text)
				{
					return;		// all tokens parsed
				}
				text += 2;
			}
			else
			{
				break;			// we are ready to parse a token
			}
		}

		// handle quoted strings - NOTE: this doesn't handle \" escaping
		if(!ignoreQuotes && *text == '"')
		{
			_argStrings[_argCount] = out;
			_argOffsets[_argCount] = (u32)(text - in);
			_argCount++;
			text++;
			while(*text && *text != '"')
			{
				*out++ = *text++;
			}
			*out++ = 0;
			if(!*text)
			{
				return;		// all tokens parsed
			}
			text++;
			continue;
		}

		// regular token
		_argStrings[_argCount] = out;
		_argOffsets[_argCount] = (u32)(text - in);
		_argCount++;

		// skip until whitespace, quote, or command
		while(*text > ' ')
		{
			if(!ignoreQuotes && text[0] == '"')
			{
				break;
			}

			if(text[0] == '/' && text[1] == '/')
			{
				break;
			}

			// skip /* */ comments
			if(text[0] == '/' && text[1] == '*')
			{
				break;
			}

			*out++ = *text++;
		}

		*out++ = 0;

		if(!*text)
		{
			return;		// all tokens parsed
		}
	}
}

void idTokenizer::RegisterArgLengths()
{
	if(_argCount == 0)
	{
		return;
	}

	const u32 lastIndex = _argCount - 1;
	for(u32 i = 0; i < lastIndex; ++i)
	{
		// -1 is not counting the the null terminator separating the arguments.
		_argLengths[i] = (u32)(_argStrings[i + 1] - _argStrings[i] - 1);
	}

	_argLengths[lastIndex] = (u32)strlen(_argStrings[lastIndex]);
}

void idTokenizer::Tokenize(const char* text, bool ignoreQuotes)
{
	TokenizeImpl(text, ignoreQuotes);
	RegisterArgLengths();
}
