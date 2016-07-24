#include "plug_in_raw_commands.hpp"
#include "utils.hpp"


udtParserPlugInRawCommands::udtParserPlugInRawCommands()
{
}

udtParserPlugInRawCommands::~udtParserPlugInRawCommands()
{
}

void udtParserPlugInRawCommands::InitAllocators(u32)
{
}

void udtParserPlugInRawCommands::CopyBuffersStruct(void* buffersStruct) const
{
	*(udtParseDataRawCommandBuffers*)buffersStruct = _buffers;
}

void udtParserPlugInRawCommands::UpdateBufferStruct()
{
	_buffers.CommandCount = _commands.GetSize();
	_buffers.CommandRanges = BufferRanges.GetStartAddress();
	_buffers.Commands = _commands.GetStartAddress();
	_buffers.StringBuffer = _stringAllocator.GetStartAddress();
	_buffers.StringBufferSize = (u32)_stringAllocator.GetCurrentByteCount();
}

u32 udtParserPlugInRawCommands::GetItemCount() const
{
	return _commands.GetSize();
}

void udtParserPlugInRawCommands::StartDemoAnalysis()
{
	_gameStateIndex = -1;
}

void udtParserPlugInRawCommands::FinishDemoAnalysis()
{
}

void udtParserPlugInRawCommands::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/)
{
	++_gameStateIndex;
}

void udtParserPlugInRawCommands::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	const udtString rawCommand = udtString::NewClone(_stringAllocator, arg.String, arg.StringLength);

	udtParseDataRawCommand info;
	info.GameStateIndex = _gameStateIndex;
	info.ServerTimeMs = parser._inServerTime;
	WriteStringToApiStruct(info.RawCommand, rawCommand);
	_commands.Add(info);
}

void udtParserPlugInRawCommands::ProcessSnapshotMessage(const udtSnapshotCallbackArg& /*arg*/, udtBaseParser& /*parser*/)
{
}
