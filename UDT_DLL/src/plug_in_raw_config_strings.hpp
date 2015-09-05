#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"


struct udtParserPlugInRawConfigStrings : udtBaseParserPlugIn
{
public:
	udtParserPlugInRawConfigStrings();
	~udtParserPlugInRawConfigStrings();

	void InitAllocators(u32 demoCount) override;
	u32  GetElementSize() const override;
	void StartDemoAnalysis() override;
	void FinishDemoAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInRawConfigStrings);

	udtVMLinearAllocator _stringAllocator;
	udtVMArray<udtParseDataRawConfigString> _configStrings; // The final array.
	s32 _gameStateIndex;
};
