#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"


struct udtParserPlugInCaptures : udtBaseParserPlugIn
{
public:
	udtParserPlugInCaptures();
	~udtParserPlugInCaptures();

	void InitAllocators(u32 demoCount) override;
	u32  GetElementSize() const override;
	void StartDemoAnalysis() override;
	void FinishDemoAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInCaptures);

	// Teams: 0=red, 1=blue.

	const char* GetPlayerName(s32 playerIndex, udtBaseParser& parser);
	s32         GetPlayerTeam01(s32 playerIndex, udtBaseParser& parser);

	udtVMLinearAllocator _stringAllocator;
	udtVMArray<udtParseDataCapture> _captures; // The final array.
	const char* _mapName;
	s32 _gameStateIndex;
	s32 _demoTakerIndex;
	s32 _pickupTimeMs[2];
	s32 _previousCaptureCount[2]; // For the first-person player only.
	f32 _pickUpPosition[2][3];
	u8 _prevFlagState[2]; 
	u8 _flagState[2];
	bool _previousCapped[2][64]; // For non-first-person players only.
};
