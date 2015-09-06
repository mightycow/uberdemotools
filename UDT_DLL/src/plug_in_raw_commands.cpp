#include "plug_in_raw_commands.hpp"


udtParserPlugInRawCommands::udtParserPlugInRawCommands()
{
}

udtParserPlugInRawCommands::~udtParserPlugInRawCommands()
{
}

void udtParserPlugInRawCommands::InitAllocators(u32 demoCount)
{
	FinalAllocator.Init((uptr)demoCount * (uptr)(1 << 16), "ParserPlugInRawCommands::CommandsArray");
	_stringAllocator.Init((uptr)demoCount * (uptr)(1 << 20), "ParserPlugInRawCommands::Strings");
	_commands.SetAllocator(FinalAllocator);
}

u32 udtParserPlugInRawCommands::GetElementSize() const
{
	return (u32)sizeof(udtParseDataRawCommand);
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
	const udtString cleanCommand = udtString::NewCleanClone(_stringAllocator, parser._inProtocol, arg.String, arg.StringLength);

	udtParseDataRawCommand info;
	info.GameStateIndex = _gameStateIndex;
	info.ServerTimeMs = parser._inServerTime;
	info.RawCommand = rawCommand.String;
	info.CleanCommand = cleanCommand.String;
	_commands.Add(info);
}

void udtParserPlugInRawCommands::ProcessSnapshotMessage(const udtSnapshotCallbackArg& /*arg*/, udtBaseParser& /*parser*/)
{
}
