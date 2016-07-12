#pragma once


#include "common.hpp"
#include "array.hpp"

#include <assert.h>


struct udtBaseParser;

struct udtNothing
{
};

struct udtChangedEntity
{
	idEntityStateBase* Entity;
	bool IsNewEvent;
};

struct udtMessageBundleCallbackArg
{
	s32 ReliableSequenceAcknowledge;
};

struct udtGamestateCallbackArg
{
	s32 ServerCommandSequence;
	s32 ClientNum;
	s32 ChecksumFeed;
};

struct udtSnapshotCallbackArg
{
	idClientSnapshotBase* Snapshot; // Never NULL.
	idClientSnapshotBase* OldSnapshot; // May be NULL.
	idEntityStateBase** Entities;
	u8* EntityFlags;
	udtChangedEntity* ChangedEntities;
	s32* RemovedEntities;
	s32 SnapshotArrayIndex;
	u32 ChangedEntityCount;
	u32 RemovedEntityCount;
	u32 EntityCount;
	s32 ServerTime;
	s32 CommandNumber;
	s32 MessageNumber;
};

struct udtCommandCallbackArg
{
	const char* String;
	u32 StringLength;
	s32 CommandSequence;
	s32 ConfigStringIndex; // Only valid if IsConfigString is true.
	bool IsConfigString;
	bool IsEmptyConfigString;
};


struct udtBaseParserPlugIn
{
	udtBaseParserPlugIn() 
		: TempAllocator(NULL)
		, DemoCount(0)
		, StartItemCount(0)
	{
	}

	virtual ~udtBaseParserPlugIn() 
	{
	}

	// Call once.
	void Init(u32 demoCount, udtVMLinearAllocator& tempAllocator)
	{
		DemoCount = demoCount;
		TempAllocator = &tempAllocator;
		InitAllocators(demoCount);
	}

	// Call for each demo.
	void StartProcessingDemo()
	{
		TempAllocator->Clear();

		StartItemCount = GetItemCount();

		StartDemoAnalysis();
	}

	// Call for each demo.
	void FinishProcessingDemo()
	{
		FinishDemoAnalysis();

		const u32 firstIndex = StartItemCount;
		const u32 lastIndex = GetItemCount();
		const u32 count = lastIndex - firstIndex;
		udtParseDataBufferRange range;
		range.FirstIndex = firstIndex;
		range.Count = count;
		BufferRanges.Add(range);
	}

	virtual void InitAllocators(u32 demoCount) = 0; // Initialize your private allocators, including FinalAllocator.

	// Only needed for analysis plug-ins.
	virtual void CopyBuffersStruct(void* /*buffersStruct*/) const {}
	virtual void UpdateBufferStruct() {}
	virtual u32  GetItemCount() const { return 0; }

	virtual void ProcessMessageBundleStart(const udtMessageBundleCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessMessageBundleEnd(const udtMessageBundleCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessSnapshotMessage(const udtSnapshotCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	
protected:
	virtual void StartDemoAnalysis() {}
	virtual void FinishDemoAnalysis() {}

	udtVMLinearAllocator* TempAllocator; // Don't create your own temp allocator, use this one.
	udtVMArray<udtParseDataBufferRange> BufferRanges { "BaseParserPlugIn::BufferRangesArray" };
	
private:
	u32 DemoCount;
	u32 StartItemCount;
};
