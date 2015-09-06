#include "plug_in_raw_config_strings.hpp"


udtParserPlugInRawConfigStrings::udtParserPlugInRawConfigStrings()
{
}

udtParserPlugInRawConfigStrings::~udtParserPlugInRawConfigStrings()
{
}

void udtParserPlugInRawConfigStrings::InitAllocators(u32 demoCount)
{
	FinalAllocator.Init((uptr)demoCount * (uptr)(1 << 16), "ParserPlugInRawConfigStrings::ConfigStringsArray");
	_stringAllocator.Init((uptr)demoCount * (uptr)(1 << 20), "ParserPlugInRawConfigStrings::Strings");
	_configStrings.SetAllocator(FinalAllocator);
}

u32 udtParserPlugInRawConfigStrings::GetElementSize() const
{
	return (u32)sizeof(udtParseDataRawConfigString);
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
		
		udtParseDataRawConfigString cs;
		cs.GameStateIndex = _gameStateIndex;
		cs.ConfigStringIndex = i;
		cs.RawConfigString = udtString::NewCloneFromRef(_stringAllocator, string).String;
		cs.CleanConfigString = udtString::NewCleanCloneFromRef(_stringAllocator, parser._inProtocol, string).String;
		_configStrings.Add(cs);
	}
}
