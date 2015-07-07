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
	virtual void ModifySnapshot(udtdSnapshotData& /*curSnap*/, udtdSnapshotData& /*oldSnap*/) {}
	virtual void AnalyzeConfigString(s32 /*index*/, const char* /*configString*/, u32 /*stringLength*/) {}
};

struct udtdConverter
{
public:
	struct SnapshotInfo
	{
		s32 ServerTime;
		s32 SnapFlags;
	};

	udtdConverter();

	void Init(udtStream& input, udtStream* output, udtProtocol::Id protocol);
	void SetStreams(udtStream& input, udtStream* output);
	void AddPlugIn(udtdConverterPlugIn* plugIn);
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

public:
	s32  GetServerTime() const { return _snapshots[_snapshotReadIndex].ServerTime; } // @TODO: rename/change
	s32  GetServerTimeOld() const { return _snapshots[_snapshotReadIndex ^ 1].ServerTime; } // @TODO: rename/change
	void MergeEntitiesFrom(const udtdConverter& sourceConv, u32 flipThis, u32 flipSource); // @TODO: rename/change
	void MergeEntities(udtdSnapshotData& dest, udtdSnapshotData& destOld, const udtdSnapshotData& source, const udtdSnapshotData& sourceOld);

private:
	idEntityStateBase& GetEntity(s32 idx) { return *(idEntityStateBase*)&_inReadEntities[idx * _protocolSizeOfEntityState]; }
	idEntityStateBase& GetBaseline(s32 idx) { return *(idEntityStateBase*)&_inBaselineEntities[idx * _protocolSizeOfPlayerState]; }

	// @TODO: Re-order the fields.

	udtStream* _input; // The user owns this.
	udtStream* _output; // The user owns this. Optional.
	udtProtocol::Id _protocol;
	u32 _protocolSizeOfEntityState;
	u32 _protocolSizeOfPlayerState;
	u8 _inMessageData[MAX_MSGLEN];
	char _inStringData[BIG_INFO_STRING];
	udtMessage _outMsg;
	udtContext _context;
	u8 _outMsgData[MAX_MSGLEN];
	s32 _outCommandSequence;
	udtdSnapshotData _snapshots[2];
	s32 _snapshotReadIndex;
	u8 _inBaselineEntities[MAX_GENTITIES * sizeof(idLargestEntityState)];
	u8 _inReadEntities[MAX_GENTITIES * sizeof(idLargestEntityState)];
	s32 _inRemovedEntities[MAX_GENTITIES];
	u8 _areaMask[32];
	bool _firstSnapshot;
	s32 _outMessageSequence;
	udtVMArrayWithAlloc<udtdConverterPlugIn*> _plugIns;
};
