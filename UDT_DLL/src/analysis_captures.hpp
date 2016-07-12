#pragma once


#include "parser.hpp"
#include "array.hpp"


struct udtCapturesAnalyzer
{
public:
	udtCapturesAnalyzer();
	~udtCapturesAnalyzer();

	void Init(u32 demoCount, udtVMLinearAllocator* tempAllocator);
	void StartDemoAnalysis();
	void FinishDemoAnalysis();
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);
	void Clear();

private:
	UDT_NO_COPY_SEMANTICS(udtCapturesAnalyzer);

	typedef void (udtCapturesAnalyzer::*ProcessGamestateFunc)(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	typedef void (udtCapturesAnalyzer::*ProcessCommandFunc)(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	typedef void (udtCapturesAnalyzer::*ProcessSnapshotFunc)(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	void ProcessGamestateMessageQLorOSP(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessageQLorOSP(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessageQLorOSP(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	void ProcessGamestateMessageCPMA(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessageCPMA(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessageCPMA(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	void ProcessGamestateMessageDummy(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessageDummy(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessageDummy(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

	void ProcessGamestateMessageClearStates(const udtGamestateCallbackArg& arg, udtBaseParser& parser);

	bool ExtractPlayerIndexFromCaptureMessageQLorOSP(s32& playerIndex, const udtString& playerName, udtProtocol::Id protocol);

	void ProcessPlayerConfigStringQLorOSP(const char* configString, udtBaseParser& parser, s32 playerIndex);
	void ProcessFlagStatusCommandQLorOSP(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessPrintCommandQLorOSP(const udtCommandCallbackArg& arg, udtBaseParser& parser);

	// Teams: 0=red, 1=blue.

	udtString GetPlayerName(s32 playerIndex, udtBaseParser& parser);
	bool      WasFlagPickedUpInBase(u32 teamIndex);

	struct PlayerInfo
	{
		f32 PickupPosition[3];
		f32 Position[3];
		s32 PickupTime;   // Only for QL.
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
			ChangeTime = UDT_S32_MIN;
			InstantCapture = false;
		}

		s32 ChangeTime;
		bool InstantCapture;
	};

	struct LastCaptureQL
	{
		bool IsValid() const
		{
			return Time != UDT_S32_MIN && Distance > 0.0f;
		}

		void Clear()
		{
			Time = UDT_S32_MIN;
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
	udtVMLinearAllocator _playerNameAllocator { "ParserPlugInCaptures::PlayerNames" }; // For QL only.
	udtString _mapName;
	s32 _gameStateIndex;
	s32 _demoTakerIndex;
	PlayerInfo _players[64];
	udtString _playerNames[64];     // For QL only.
	udtString _playerClanNames[64]; // For QL only.
	TeamInfo _teams[2];
	LastCaptureQL _lastCaptureQL;
	PlayerStateQL _playerStateQL;
	FlagStatusCPMA _flagStatusCPMA[2];
	udtVMLinearAllocator* _tempAllocator;
	bool _firstSnapshot;

public:
	udtVMLinearAllocator StringAllocator { "ParserPlugInCaptures::Strings" };
	udtVMArray<udtParseDataCapture> Captures { "ParserPlugInCaptures::CapturesArray" };
};
