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
		_lastProcessedServerCommandNumber = -1;
		_gameStateIndex = -1;
		_playerNamesAllocator.Init(1 << 16, UDT_MEMORY_PAGE_SIZE);
		_tempAllocator.Init(1 << 16, UDT_MEMORY_PAGE_SIZE);
	}

	~udtObituariesAnalyzer()
	{
	}

	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	ObituaryArray Obituaries;

private:
	UDT_NO_COPY_SEMANTICS(udtObituariesAnalyzer);

	const char* AllocatePlayerName(udtBaseParser& parser, s32 playerIdx);

	udtVMLinearAllocator _playerNamesAllocator;
	udtVMLinearAllocator _tempAllocator;
	s32 _gameStateIndex;
	s32 _lastProcessedServerCommandNumber;
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
