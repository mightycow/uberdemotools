#pragma once


#include "parser.hpp"
#include "timer.hpp"


struct udtParserRunner
{
	udtParserRunner();

	bool Init(udtBaseParser& parser, udtStream& file, const s32* cancelOperation);
	bool ParseNextMessage(); // Returns true as long as there's supposed to be more to read.
	void FinishParsing();
	bool WasSuccess() const;

private:
	UDT_NO_COPY_SEMANTICS(udtParserRunner);

private:
	void SetSuccess(bool success);

	udtMessage _inMsg;
	udtTimer _timer;
	u64 _fileStartOffset;
	u64 _fileOffset;
	u64 _maxByteCount;
	udtBaseParser* _parser;
	udtStream* _file;
	const s32* _cancelOperation;
	bool _success;
};
