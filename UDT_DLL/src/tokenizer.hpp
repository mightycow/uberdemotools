#pragma once


#include "common.hpp"


struct CommandLineTokenizer
{
public:
	int			argc() const;
	const char* argv(int arg) const;
	void		Tokenize(const char* text, qbool ignoreQuotes = qfalse);

private:
	int			cmd_argc;
	char*		cmd_argv[MAX_STRING_TOKENS];		// points into cmd_tokenized
	char		cmd_tokenized[BIG_INFO_STRING+MAX_STRING_TOKENS];	// will have 0 bytes inserted
	char		cmd_cmd[BIG_INFO_STRING]; // the original command we received (no token processing)
};