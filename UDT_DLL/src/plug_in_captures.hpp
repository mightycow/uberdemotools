#pragma once


#include "parser_plug_in.hpp"
#include "analysis_captures.hpp"


struct udtParserPlugInCaptures : udtBaseParserPlugIn
{
public:
	udtParserPlugInCaptures();
	~udtParserPlugInCaptures();

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
	UDT_NO_COPY_SEMANTICS(udtParserPlugInCaptures);

	udtCapturesAnalyzer _analyzer;
	udtParseDataCaptureBuffers _buffers;
};
