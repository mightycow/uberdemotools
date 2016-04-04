#include "analysis_splitter.hpp"


void udtParserPlugInSplitter::InitAllocators(u32 demoCount)
{
	GamestateFileOffsets.Init((uptr)(64 * sizeof(u32)) * (uptr)demoCount, "ParserPlugInSplitter::GamestateFileOffsetsArray");
}

void udtParserPlugInSplitter::ProcessGamestateMessage(const udtGamestateCallbackArg& /*info*/, udtBaseParser& parser)
{
	GamestateFileOffsets.Add(parser._inFileOffset);
}
