#include "analysis_splitter.hpp"


void udtParserPlugInSplitter::InitAllocators(u32)
{
}

void udtParserPlugInSplitter::ProcessGamestateMessage(const udtGamestateCallbackArg& /*info*/, udtBaseParser& parser)
{
	GamestateFileOffsets.Add(parser._inFileOffset);
}
