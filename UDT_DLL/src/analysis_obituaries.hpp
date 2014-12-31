#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"


struct udtObituariesAnalyzer
{
public:

	typedef udtVMArray<udtParseDataObituary> ObituaryArray;

public:
	udtObituariesAnalyzer()
	{
		_gameStateIndex = -1;
		_enableNameAllocation = true;
		_playerNamesAllocator.Init(1 << 16, UDT_MEMORY_PAGE_SIZE);
		_tempAllocator.Init(1 << 16, UDT_MEMORY_PAGE_SIZE);

		for(u32 i = 0; i < 64; ++i)
		{
			_playerTeams[i] = -1;
		}
	}

	~udtObituariesAnalyzer()
	{
	}

	void SetNameAllocationEnabled(bool enabled) { _enableNameAllocation = enabled; }
 
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser);

	ObituaryArray Obituaries;

private:
	UDT_NO_COPY_SEMANTICS(udtObituariesAnalyzer);

	const char* AllocatePlayerName(udtBaseParser& parser, s32 playerIdx);

	udtVMLinearAllocator _playerNamesAllocator;
	udtVMLinearAllocator _tempAllocator;
	s32 _playerTeams[64];
	s32 _gameStateIndex;
	bool _enableNameAllocation;
};

struct udtParserPlugInObituaries : udtBaseParserPlugIn
{
public:
	udtParserPlugInObituaries()
	{
	}

	~udtParserPlugInObituaries()
	{
	}

	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
	{
		Analyzer.ProcessSnapshotMessage(arg, parser);
	}

	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
	{
		Analyzer.ProcessGamestateMessage(arg, parser);
	}

	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
	{
		Analyzer.ProcessCommandMessage(arg, parser);
	}

	void FinishAnalysis()
	{
	}

	u32 GetElementCount() const
	{
		return Analyzer.Obituaries.GetSize();
	}

	u32 GetElementSize() const
	{
		return (u32)sizeof(udtParseDataObituary);
	};

	void* GetFirstElementAddress()
	{
		return Analyzer.Obituaries.GetStartAddress();
	}

	udtObituariesAnalyzer Analyzer;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInObituaries);
};
