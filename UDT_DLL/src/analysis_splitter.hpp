#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"


struct DemoSplitterAnalyzer : udtBaseParserPlugIn
{
public:
	DemoSplitterAnalyzer() {}
	~DemoSplitterAnalyzer() {}

	void  ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser);
	u32   GetElementCount() const { return GamestateFileOffsets.GetSize(); }
	void* GetFirstElementAddress() { return GetElementCount() > 0 ? GamestateFileOffsets.GetStartAddress() : NULL; }

	udtVMArray<u32> GamestateFileOffsets;

private:
	UDT_NO_COPY_SEMANTICS(DemoSplitterAnalyzer);
};
