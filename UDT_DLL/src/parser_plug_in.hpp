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
	udtChangedEntity* Entities;
	s32* RemovedEntities;
	s32 SnapshotArrayIndex;
	u32 EntityCount;
	u32 RemovedEntityCount;
	s32 ServerTime;
};

struct udtCommandCallbackArg
{
	const char* String;
	u32 StringLength;
	s32 CommandSequence;
	s32 ConfigStringIndex; // Only valid if IsConfigString is true.
	bool IsConfigString;
};


struct udtBaseParserPlugIn
{
	udtBaseParserPlugIn() 
		: TempAllocator(NULL)
		, CurrentArrayStartAddress(NULL)
		, DemoCount(0)
		, DemoIndex(0)
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
		ArraysAllocator.Init((uptr)demoCount * (uptr)sizeof(ArrayInfo), "ParserPlugIn::ArraysArray");
		Arrays.SetAllocator(ArraysAllocator);
		Arrays.Resize(demoCount);
		InitAllocators(demoCount);
	}

	// Call for each demo.
	void StartProcessingDemo()
	{
		TempAllocator->Clear();
		CurrentArrayStartAddress = FinalAllocator.GetCurrentAddress();

		StartDemoAnalysis();
	}

	// Call for each demo.
	void FinishProcessingDemo()
	{
		FinishDemoAnalysis();

		if(FinalAllocator.GetStartAddress() != NULL)
		{
			// For Analyzer plug-ins.
			u8* const endAddress = FinalAllocator.GetCurrentAddress();
			Arrays[DemoIndex].StartAddress = CurrentArrayStartAddress;
			Arrays[DemoIndex].ElementCount = (u32)(endAddress - CurrentArrayStartAddress) / GetElementSize();
		}
		else
		{
			// For Modifier plug-ins.
			Arrays[DemoIndex].StartAddress = NULL;
			Arrays[DemoIndex].ElementCount = 0;
		}

		++DemoIndex;
	}

	void* GetFirstElementAddress(u32 demoIndex) const
	{
		assert(demoIndex < DemoCount);

		return Arrays[demoIndex].StartAddress;
	}

	u32 GetElementCount(u32 demoIndex) const
	{ 
		assert(demoIndex < DemoCount);

		return Arrays[demoIndex].ElementCount;
	}

	virtual void InitAllocators(u32 demoCount) = 0; // Initialize your private allocators, including FinalAllocator.
	virtual u32  GetElementSize() const = 0;

	virtual void ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessSnapshotMessage(const udtSnapshotCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	
protected:
	virtual void StartDemoAnalysis() {}
	virtual void FinishDemoAnalysis() {}

	udtVMLinearAllocator FinalAllocator; // The allocator that will allocate the final array.
	udtVMLinearAllocator* TempAllocator; // Don't create your own temp allocator, use this one.
	
private:
	struct ArrayInfo
	{
		u8* StartAddress;
		u32 ElementCount;
	};

	udtVMArray<ArrayInfo> Arrays;         // Final size: DemoCount.
	udtVMLinearAllocator ArraysAllocator; // The allocator used by Arrays.
	u8* CurrentArrayStartAddress;
	u32 DemoCount;
	u32 DemoIndex;
};
