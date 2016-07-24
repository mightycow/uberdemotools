#pragma once


#include "message.hpp"
#include "stream.hpp"
#include "array.hpp"
#include "udtd_types.hpp"


struct udtdClientEntity
{
	idLargestEntityState EntityState;
	bool Valid;
};

struct udtdSnapshotData
{
	udtdClientEntity Entities[MAX_GENTITIES];
	idLargestPlayerState PlayerState;
	s32 ServerTime;
};

struct udtdConverterPlugIn
{
	virtual ~udtdConverterPlugIn() {}

	virtual void InitPlugIn(udtProtocol::Id /*protocol*/) {}
	virtual void ModifySnapshot(udtdSnapshotData& /*curSnap*/, udtdSnapshotData& /*oldSnap*/) {}
	virtual void AnalyzeConfigString(s32 /*index*/, const char* /*configString*/, u32 /*stringLength*/) {}
};

// Don't ever allocate an instance of this on the stack.
struct udtdConverter
{
public:
	struct SnapshotInfo
	{
		s32 ServerTime;
		s32 SnapFlags;
	};

	udtdConverter();
	~udtdConverter();

	void ResetForNextDemo(udtStream& input, udtStream* output, udtProtocol::Id protocol);
	void SetStreams(udtStream& input, udtStream* output);
	void AddPlugIn(udtdConverterPlugIn* plugIn);
	void ClearPlugIns();
	bool ProcessNextMessage(udtdMessageType::Id& type);
	bool ProcessNextMessageRead(udtdMessageType::Id& type, SnapshotInfo& snapshot);
	bool ProcessNextMessageWrite(udtdMessageType::Id type, const SnapshotInfo& snapshot);

private:
	bool ProcessGameState();
	void ProcessCommand();
	void ProcessSnapshot();
	void ReadSnapshot(SnapshotInfo& info);
	void WriteSnapshot(const SnapshotInfo& info);
	void ProcessEndOfFile();
	void WriteOutputMessageToFile(bool increaseMessageSequence);
	bool IsPlayerAlreadyDefined(const udtdSnapshotData& snapshot, s32 clientNum, s32 entityNumber);
	void MergePlayerEntity(udtdSnapshotData& dest, udtdSnapshotData& destOld, const udtdSnapshotData& source, const udtdSnapshotData& sourceOld, u32 number);
	void MergeItemEntity(udtdSnapshotData& dest, udtdSnapshotData& destOld, const udtdSnapshotData& source, const udtdSnapshotData& sourceOld, u32 number);

public:
	s32  GetServerTime() const { return _snapshots[_snapshotReadIndex].ServerTime; } // @TODO: rename/change
	s32  GetServerTimeOld() const { return _snapshots[_snapshotReadIndex ^ 1].ServerTime; } // @TODO: rename/change
	void MergeEntitiesFrom(const udtdConverter& sourceConv, u32 flipThis, u32 flipSource); // @TODO: rename/change
	void MergeEntities(udtdSnapshotData& dest, udtdSnapshotData& destOld, const udtdSnapshotData& source, const udtdSnapshotData& sourceOld);

private:
	idEntityStateBase* GetEntity(s32 idx) { return (idEntityStateBase*)&_inReadEntities[idx * _protocolSizeOfEntityState]; }
	idEntityStateBase* GetBaseline(s32 idx) { return (idEntityStateBase*)&_inBaselineEntities[idx * _protocolSizeOfPlayerState]; }

	u8 _inBaselineEntities[MAX_GENTITIES * sizeof(idLargestEntityState)]; // 1 KB * a lot
	u8 _inReadEntities[MAX_GENTITIES * sizeof(idLargestEntityState)]; // 1 KB * a lot
	u8 _outMsgData[ID_MAX_MSG_LENGTH]; // 16 KB
	char _inStringData[BIG_INFO_STRING]; // 8 KB
	s32 _inRemovedEntities[MAX_GENTITIES]; // 4 KB
	udtdSnapshotData _snapshots[2];
	u8 _areaMask[32];
	udtMessage _outMsg;
	udtContext _context;
	udtVMArray<udtdConverterPlugIn*> _plugIns { "UDTDemoConverter::PlugIns" };
	udtStream* _input; // The user owns this.
	udtStream* _output; // The user owns this. Optional.
	udtProtocol::Id _protocol;
	u32 _protocolSizeOfEntityState;
	u32 _protocolSizeOfPlayerState;
	s32 _outCommandSequence;
	s32 _snapshotReadIndex;
	s32 _outMessageSequence;
	bool _firstSnapshot;
	bool _firstMessage;
};
