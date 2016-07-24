#include "plug_in_raw_config_strings.hpp"
#include "utils.hpp"


udtParserPlugInRawConfigStrings::udtParserPlugInRawConfigStrings()
{
}

udtParserPlugInRawConfigStrings::~udtParserPlugInRawConfigStrings()
{
}

void udtParserPlugInRawConfigStrings::InitAllocators(u32)
{
}

void udtParserPlugInRawConfigStrings::CopyBuffersStruct(void* buffersStruct) const
{
	*(udtParseDataRawConfigStringBuffers*)buffersStruct = _buffers;
}

void udtParserPlugInRawConfigStrings::UpdateBufferStruct()
{
	_buffers.ConfigStringCount = _configStrings.GetSize();
	_buffers.ConfigStringRanges = BufferRanges.GetStartAddress();
	_buffers.ConfigStrings = _configStrings.GetStartAddress();
	_buffers.StringBuffer = _stringAllocator.GetStartAddress();
	_buffers.StringBufferSize = (u32)_stringAllocator.GetCurrentByteCount();
}

u32 udtParserPlugInRawConfigStrings::GetItemCount() const
{
	return _configStrings.GetSize();
}

void udtParserPlugInRawConfigStrings::StartDemoAnalysis()
{
	_gameStateIndex = -1;
}

void udtParserPlugInRawConfigStrings::FinishDemoAnalysis()
{
}

void udtParserPlugInRawConfigStrings::ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& parser)
{
	++_gameStateIndex;

	for(s32 i = 0; i < MAX_CONFIGSTRINGS; ++i)
	{
		const udtString string = parser.GetConfigString(i);
		if(udtString::IsNullOrEmpty(string))
		{
			continue;
		}

		const udtString rawConfigString = udtString::NewCloneFromRef(_stringAllocator, string);		

		udtParseDataRawConfigString cs;
		cs.GameStateIndex = _gameStateIndex;
		cs.ConfigStringIndex = i;
		WriteStringToApiStruct(cs.RawConfigString, rawConfigString);
		_configStrings.Add(cs);
	}
}
