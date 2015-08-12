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
	u32  GetElementSize() const override;
	void StartDemoAnalysis() override;
	void FinishDemoAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInRawCommands);

	udtVMLinearAllocator _allocator;
	udtVMArray<udtParseDataRawCommand> _commands; // The final array.
	s32 _gameStateIndex;
};
