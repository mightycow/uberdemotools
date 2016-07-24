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

	void InitAllocators(u32 demoCount, udtVMLinearAllocator& tempAllocator);
	void ResetForNextDemo();
 
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser);

	udtVMLinearAllocator& GetStringAllocator() { return _stringAllocator; }

	udtVMArray<udtParseDataObituary> Obituaries { "ObituariesAnalyzer::ObituariesArray" };

private:
	UDT_NO_COPY_SEMANTICS(udtObituariesAnalyzer);

	udtString AllocatePlayerName(udtBaseParser& parser, s32 playerIdx);

	udtVMLinearAllocator _stringAllocator { "ObituariesAnalyzer::Strings" };
	udtVMLinearAllocator* _tempAllocator;
	s32 _playerTeams[64];
	s32 _gameStateIndex;
	bool _enableNameAllocation;
};
