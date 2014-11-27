#pragma once


#include "common.hpp"


struct CommandLineTokenizer
{
public:
	s32			argc() const;
	const char* argv(s32 arg) const;
	void		Tokenize(const char* text, qbool ignoreQuotes = qfalse);

private:
	s32			cmd_argc;
	char*		cmd_argv[MAX_STRING_TOKENS];		// points into cmd_tokenized
	char		cmd_tokenized[BIG_INFO_STRING+MAX_STRING_TOKENS];	// will have 0 bytes inserted
	char		cmd_cmd[BIG_INFO_STRING]; // the original command we received (no token processing)
};