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
	void CopyBuffersStruct(void* buffersStruct) const override;
	void UpdateBufferStruct() override;
	u32  GetItemCount() const override;
	void StartDemoAnalysis() override;
	void FinishDemoAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInRawConfigStrings);

	udtVMLinearAllocator _stringAllocator { "ParserPlugInRawConfigStrings::Strings" };
	udtVMArray<udtParseDataRawConfigString> _configStrings { "ParserPlugInRawConfigStrings::ConfigStringsArray" };
	udtParseDataRawConfigStringBuffers _buffers;
	s32 _gameStateIndex;
};
