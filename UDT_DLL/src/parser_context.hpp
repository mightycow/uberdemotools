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

	void Reset();
	void ResetButKeepPlugInData();
	void CreateAndAddPlugIns(const u32* plugInIds, u32 plugInCount);
	bool GetDataInfo(u32 demoIdx, u32 plugInId, void** buffer, u32* count);
	u32  GetDemoCount() const { return DemoCount; }

private:
	void DestroyPlugIns();

public:
	udtContext Context;
	udtBaseParser Parser;
	udtVMLinearAllocator PlugInAllocator;
	udtVMArray<AddOnItem> PlugIns; // There are DemoCount * PlugInCountPerDemo elements.
	udtVMArray<u32> InputIndices;
	u32 DemoCount;
	u32 PlugInCountPerDemo;
};
