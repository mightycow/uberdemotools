#pragma once


#include "common.hpp"


struct udtBaseParser;

struct udtGamestateCallbackArg
{
	s32 ServerCommandSequence;
	s32 ClientNum;
	s32 ChecksumFeed;
};

struct udtSnapshotCallbackArg
{
	s32 ServerTime;
	s32 SnapshotArrayIndex;
	idClientSnapshotBase* Snapshot;
};

struct udtCommandCallbackArg
{
	s32 CommandSequence;
	u32 StringLength;
	const char* String;
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
	virtual void* GetFirstElementAddress() = 0;
};
