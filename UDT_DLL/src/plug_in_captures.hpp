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
	void CopyBuffersStruct(void* buffersStruct) const override;
	void UpdateBufferStruct() override;
	u32  GetItemCount() const override;
	void StartDemoAnalysis() override;
	void FinishDemoAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInCaptures);

	typedef void (udtParserPlugInCaptures::*ProcessGamestateFunc)(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	typedef void (udtParserPlugInCaptures::*ProcessCommandFunc)(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	typedef void (udtParserPlugInCaptures::*ProcessSnapshotFunc)(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	void ProcessGamestateMessageString(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessageString(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessageString(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	void ProcessGamestateMessageEvents(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessageEvents(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessageEvents(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	void ProcessGamestateMessageNada(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessageNada(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessageNada(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	// Teams: 0=red, 1=blue.

	udtString GetPlayerName(s32 playerIndex, udtBaseParser& parser);
	bool      WasFlagPickedUpInBase(u32 teamIndex);

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

	ProcessGamestateFunc _processGamestate;
	ProcessCommandFunc _processCommand;
	ProcessSnapshotFunc _processSnapshot;
	udtVMLinearAllocator _stringAllocator;
	udtVMArray<udtParseDataCapture> _captures;
	udtParseDataCaptureBuffers _buffers;
	udtString _mapName;
	s32 _gameStateIndex;
	s32 _demoTakerIndex;
	PlayerInfo _players[64];
	TeamInfo _teams[2];
	bool _firstSnapshot;
};
