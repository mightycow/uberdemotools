#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"


struct udtObituariesAnalyzer
{
public:
	udtObituariesAnalyzer()
	{
		_tempAllocator = NULL;
		_enableNameAllocation = true;
	}

	~udtObituariesAnalyzer()
	{
	}

	void SetNameAllocationEnabled(bool enabled) { _enableNameAllocation = enabled; }

	void InitAllocators(u32 demoCount, udtVMLinearAllocator& finalAllocator, udtVMLinearAllocator& tempAllocator);
	void ResetForNextDemo();
 
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser);

	udtVMArray<udtParseDataObituary> Obituaries;

private:
	UDT_NO_COPY_SEMANTICS(udtObituariesAnalyzer);

	const char* AllocatePlayerName(udtBaseParser& parser, s32 playerIdx);

	udtVMLinearAllocator _playerNamesAllocator;
	udtVMLinearAllocator* _tempAllocator;
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

	void InitAllocators(u32 demoCount)
	{
		Analyzer.InitAllocators(demoCount, FinalAllocator, *TempAllocator);
	}

	u32 GetElementSize() const
	{
		return (u32)sizeof(udtParseDataObituary);
	};

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

	udtObituariesAnalyzer Analyzer;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInObituaries);
};
