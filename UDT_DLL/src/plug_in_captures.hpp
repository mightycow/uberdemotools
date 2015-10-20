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
	bool        WasFlagPickedUpInBase(u32 teamIndex);

	struct PlayerInfo
	{
		s32 CaptureCount;
		s32 PickupTimeMs;
		s32 PrevCaptureCount;
		f32 PickupPosition[3];
		f32 Position[3];
		bool Capped;
		bool PrevCapped;
		bool BasePickup;
		bool HasFlag;
		bool PrevHasFlag;
	};

	struct TeamInfo
	{
		u8 PrevFlagState;
		u8 FlagState;
	};

	udtVMLinearAllocator _stringAllocator;
	udtVMArray<udtParseDataCapture> _captures; // The final array.
	const char* _mapName;
	s32 _gameStateIndex;
	s32 _demoTakerIndex;
	PlayerInfo _players[64];
	TeamInfo _teams[2];
	bool _firstSnapshot;
};
