#pragma once


#include "analysis_obituaries.hpp"


struct udtParserPlugInObituaries : udtBaseParserPlugIn
{
public:
	udtParserPlugInObituaries()
	{
	}

	~udtParserPlugInObituaries()
	{
	}

	void InitAllocators(u32 demoCount) override
	{
		Analyzer.InitAllocators(demoCount, *TempAllocator);
	}

	void CopyBuffersStruct(void* buffersStruct) const override
	{
		*(udtParseDataObituaryBuffers*)buffersStruct = _buffers;
	}

	void UpdateBufferStruct() override
	{
		_buffers.ObituaryCount = Analyzer.Obituaries.GetSize();
		_buffers.ObituaryRanges = BufferRanges.GetStartAddress();
		_buffers.Obituaries = Analyzer.Obituaries.GetStartAddress();
		_buffers.StringBuffer = Analyzer.GetStringAllocator().GetStartAddress();
		_buffers.StringBufferSize = (u32)Analyzer.GetStringAllocator().GetCurrentByteCount();
	}

	u32  GetItemCount() const override
	{
		return Analyzer.Obituaries.GetSize();
	}

	void StartDemoAnalysis() override
	{
		Analyzer.ResetForNextDemo();
	}

	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override
	{
		Analyzer.ProcessSnapshotMessage(arg, parser);
	}

	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override
	{
		Analyzer.ProcessGamestateMessage(arg, parser);
	}

	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override
	{
		Analyzer.ProcessCommandMessage(arg, parser);
	}

	udtObituariesAnalyzer Analyzer;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInObituaries);

	udtParseDataObituaryBuffers _buffers;
};
