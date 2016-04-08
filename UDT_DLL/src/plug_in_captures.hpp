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

	void ProcessGamestateMessageStrings(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessageStrings(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessageStrings(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	void ProcessGamestateMessageEntities(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessageEntities(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessageEntities(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	void ProcessGamestateMessageDummy(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessageDummy(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessageDummy(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	// Teams: 0=red, 1=blue.

	udtString GetPlayerName(s32 playerIndex, udtBaseParser& parser);
	bool      WasFlagPickedUpInBase(u32 teamIndex);

	struct PlayerInfo
	{
		f32 PickupPosition[3];
		f32 Position[3];
		bool Defined;     // Defined in this snapshot?
		bool PrevDefined; // Defined in the previous snapshot?
		bool PickupPositionValid;
	};

	struct TeamInfo
	{
		s32 PlayerIndex;
		u8 PrevFlagState;
		u8 FlagState;
		bool BasePickup;
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
