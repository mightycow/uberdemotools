#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"


struct udtParserPlugInSplitter : udtBaseParserPlugIn
{
public:
	udtParserPlugInSplitter() {}
	~udtParserPlugInSplitter() {}

	void InitAllocators(u32 demoCount);
	u32  GetElementSize() const { return (u32)sizeof(u32); };

	void ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser);

	udtVMArray<u32> GamestateFileOffsets { "ParserPlugInSplitter::GamestateFileOffsetsArray" }; // Final array.

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInSplitter);
};
