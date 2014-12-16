#pragma once


#include "common.hpp"


struct udtBaseParser;

struct udtChangedEntity
{
	idEntityStateBase* Entity;
	bool IsNewEvent;
};

struct udtGamestateCallbackArg
{
	s32 ServerCommandSequence;
	s32 ClientNum;
	s32 ChecksumFeed;
};

struct udtSnapshotCallbackArg
{
	idClientSnapshotBase* Snapshot;
	udtChangedEntity* Entities;
	s32 SnapshotArrayIndex;
	u32 EntityCount;
	s32 ServerTime;
};

struct udtCommandCallbackArg
{
	const char* String;
	u32 StringLength;
	s32 CommandSequence;
};


struct udtBaseParserPlugIn
{
	udtBaseParserPlugIn() {}
	virtual ~udtBaseParserPlugIn() {}

	virtual void  ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void  ProcessSnapshotMessage(const udtSnapshotCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void  ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void  FinishAnalysis() {}
	virtual u32   GetElementCount() const = 0;
	virtual u32   GetElementSize() const = 0;
	virtual void* GetFirstElementAddress() = 0;
};
