#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"


struct udtParserPlugInRawCommands : udtBaseParserPlugIn
{
public:
	udtParserPlugInRawCommands();
	~udtParserPlugInRawCommands();

	void InitAllocators(u32 demoCount) override;
	void CopyBuffersStruct(void* buffersStruct) const override;
	void UpdateBufferStruct() override;
	u32  GetItemCount() const override;
	void StartDemoAnalysis() override;
	void FinishDemoAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInRawCommands);

	udtVMLinearAllocator _stringAllocator { "ParserPlugInRawCommands::Strings" };
	udtVMArray<udtParseDataRawCommand> _commands { "ParserPlugInRawCommands::CommandsArray" };
	udtParseDataRawCommandBuffers _buffers;
	s32 _gameStateIndex;
};
