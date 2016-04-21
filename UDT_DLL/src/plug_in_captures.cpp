#include "plug_in_captures.hpp"


udtParserPlugInCaptures::udtParserPlugInCaptures()
{
}

udtParserPlugInCaptures::~udtParserPlugInCaptures()
{
}

void udtParserPlugInCaptures::InitAllocators(u32 demoCount)
{
	_analyzer.Init(demoCount, TempAllocator);
}

void udtParserPlugInCaptures::CopyBuffersStruct(void* buffersStruct) const
{
	*(udtParseDataCaptureBuffers*)buffersStruct = _buffers;
}

void udtParserPlugInCaptures::UpdateBufferStruct()
{
	_buffers.CaptureCount = _analyzer.Captures.GetSize();
	_buffers.CaptureRanges = BufferRanges.GetStartAddress();
	_buffers.Captures = _analyzer.Captures.GetStartAddress();
	_buffers.StringBuffer = _analyzer.StringAllocator.GetStartAddress();
	_buffers.StringBufferSize = (u32)_analyzer.StringAllocator.GetCurrentByteCount();
}

u32 udtParserPlugInCaptures::GetItemCount() const
{
	return _analyzer.Captures.GetSize();
}

void udtParserPlugInCaptures::StartDemoAnalysis()
{
	_analyzer.StartDemoAnalysis();
}

void udtParserPlugInCaptures::FinishDemoAnalysis()
{
	_analyzer.FinishDemoAnalysis();
}

void udtParserPlugInCaptures::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessGamestateMessage(arg, parser);
}

void udtParserPlugInCaptures::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessCommandMessage(arg, parser);
}

void udtParserPlugInCaptures::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessSnapshotMessage(arg, parser);
}
