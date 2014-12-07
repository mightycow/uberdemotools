#include "analysis_splitter.hpp"


void DemoSplitterAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& /*info*/, udtBaseParser& parser)
{
	GamestateFileOffsets.Add(parser._inFileOffset);
}
