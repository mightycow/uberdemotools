#include "tokenizer.hpp"

#include <stdlib.h>
#include <malloc.h>
#include <string.h>


static const char* cmd_null_string = "";


s32	CommandLineTokenizer::argc() const
{
	return cmd_argc;
}

const char* CommandLineTokenizer::argv(s32 arg) const
{
	if ( (unsigned)arg >= (unsigned)cmd_argc )
		return cmd_null_string;

	if ( (unsigned)arg >= MAX_STRING_TOKENS )
		return cmd_null_string;

	return cmd_argv[arg];	
}

void CommandLineTokenizer::Tokenize(const char* text, qbool ignoreQuotes)
{
	// clear previous args
	cmd_argc = 0;

	if ( !text )
		return;

	Q_strncpyz( cmd_cmd, text, sizeof(cmd_cmd) );

	char* out = cmd_tokenized;

	for(;;) {
		if ( cmd_argc == MAX_STRING_TOKENS ) {
			return;			// this is usually something malicious
		}

		for(;;) {
			// skip whitespace
			while ( *text && *text <= ' ' ) {
				text++;
			}
			if ( !*text ) {
				return;			// all tokens parsed
			}

			// skip // comments
			if ( text[0] == '/' && text[1] == '/' ) {
				return;			// all tokens parsed
			}

			// skip /* */ comments
			if ( text[0] == '/' && text[1] =='*' ) {
				while ( *text && ( text[0] != '*' || text[1] != '/' ) ) {
					text++;
				}
				if ( !*text ) {
					return;		// all tokens parsed
				}
				text += 2;
			} else {
				break;			// we are ready to parse a token
			}
		}

		// handle quoted strings - NOTE: this doesn't handle \" escaping
		if ( !ignoreQuotes && *text == '"' ) {
			cmd_argv[cmd_argc] = out;
			cmd_argc++;
			text++;
			while ( *text && *text != '"' ) {
				*out++ = *text++;
			}
			*out++ = 0;
			if ( !*text ) {
				return;		// all tokens parsed
			}
			text++;
			continue;
		}

		// regular token
		cmd_argv[cmd_argc] = out;
		cmd_argc++;

		// skip until whitespace, quote, or command
		while ( *text > ' ' ) {
			if ( !ignoreQuotes && text[0] == '"' ) {
				break;
			}

			if ( text[0] == '/' && text[1] == '/' ) {
				break;
			}

			// skip /* */ comments
			if ( text[0] == '/' && text[1] =='*' ) {
				break;
			}

			*out++ = *text++;
		}

		*out++ = 0;

		if ( !*text ) {
			return;		// all tokens parsed
		}
	}
}