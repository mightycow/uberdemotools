#pragma once


#include "context.hpp"
#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"


struct udtParserContext
{
private:
	struct AddOnItem
	{
		udtBaseParserPlugIn* PlugIn;
		udtParserPlugIn::Id Id;
	};

public:
	udtParserContext();
	~udtParserContext();

	void Init(u32 demoCount, const u32* plugInIds = NULL, u32 plugInCount = 0); // Called once for all.
	void ResetForNextDemo(bool keepPlugInData); // Called once per demo processed.
	bool GetDataInfo(u32 demoIdx, u32 plugInId, void** buffer, u32* count);
	u32  GetDemoCount() const { return DemoCount; }

private:
	void DestroyPlugIns();

public:
	udtContext Context;
	udtBaseParser Parser;
	udtVMLinearAllocator PlugInAllocator;
	udtVMArrayWithAlloc<AddOnItem> PlugIns; // There is only 1 (shared) plug-in instance for each plug-in ID passed.
	udtVMArrayWithAlloc<u32> InputIndices;
	u32 DemoCount;

private:
	udtVMLinearAllocator _parserAllocators[udtBaseParserAllocator::Count];
	u8* _fixedSizeArrays[udtBaseParserFixedSizeArray::Count];
	udtVMLinearAllocator _plugInTempAllocator;
};
