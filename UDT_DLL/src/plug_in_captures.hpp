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

	void ProcessGamestateMessageQL(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessageQL(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessageQL(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	void ProcessGamestateMessageCPMA(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessageCPMA(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessageCPMA(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	void ProcessGamestateMessageDummy(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessageDummy(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessageDummy(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	void ProcessGamestateMessageClearStates(const udtGamestateCallbackArg& arg, udtBaseParser& parser);

	bool ExtractPlayerIndexFromCaptureMessageQL(s32& playerIndex, const udtString& playerName, udtProtocol::Id protocol);

	void ProcessPlayerConfigStringQL(const char* configString, udtBaseParser& parser, s32 playerIndex);
	void ProcessFlagStatusCommandQL(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessPrintCommandQL(const udtCommandCallbackArg& arg, udtBaseParser& parser);

	// Teams: 0=red, 1=blue.

	udtString GetPlayerName(s32 playerIndex, udtBaseParser& parser);
	bool      WasFlagPickedUpInBase(u32 teamIndex);

	struct PlayerInfo
	{
		f32 PickupPosition[3];
		f32 Position[3];
		s32 PickupTime; // Only for QL.
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

	struct FlagStatusCPMA
	{
		void Clear()
		{
			ChangeTime = S32_MIN;
			InstantCapture = false;
		}

		s32 ChangeTime;
		bool InstantCapture;
	};

	struct LastCaptureQL
	{
		bool IsValid() const
		{
			return Time != S32_MIN && Distance > 0.0f;
		}

		void Clear()
		{
			Time = S32_MIN;
			Distance = -1.0f;
		}

		s32 Time;
		f32 Distance;
	};

	struct PlayerStateQL
	{
		void Clear()
		{
			CaptureCount = 0;
			PrevCaptureCount = 0;
			HasFlag = false;
			PrevHasFlag = false;
		}

		s32 CaptureCount;
		s32 PrevCaptureCount;
		bool HasFlag;
		bool PrevHasFlag;
	};

	ProcessGamestateFunc _processGamestate;
	ProcessCommandFunc _processCommand;
	ProcessSnapshotFunc _processSnapshot;
	udtVMLinearAllocator _stringAllocator;
	udtVMLinearAllocator _playerNameAllocator; // For QL only.
	udtVMArray<udtParseDataCapture> _captures;
	udtParseDataCaptureBuffers _buffers;
	udtString _mapName;
	s32 _gameStateIndex;
	s32 _demoTakerIndex;
	PlayerInfo _players[64];
	udtString _playerNames[64]; // For QL only.
	udtString _playerClanNames[64]; // For QL only.
	TeamInfo _teams[2];
	LastCaptureQL _lastCaptureQL;
	PlayerStateQL _playerStateQL;
	FlagStatusCPMA _flagStatusCPMA[2];
	bool _firstSnapshot;
};
