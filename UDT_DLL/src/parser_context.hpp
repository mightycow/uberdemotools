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

	struct PlugInData
	{
		u8* Items;
		u32 ItemCount;
	};

public:
	udtParserContext();
	~udtParserContext();

	void Reset();
	void CreateAndAddPlugIn(u32 plugInId);
	bool GetDataInfo(u32 demoIdx, u32 plugInId, void** buffer, u32* count);
	void StartDemo();
	void EndDemo();

private:
	void DestroyPlugIns();

public:
	udtContext Context;
	udtBaseParser Parser;
	udtVMLinearAllocator PlugInAllocator;
	udtVMArray<AddOnItem> PlugIns;
	udtVMArray<PlugInData> DataItems; // There are PlugIns.Size() * DemoCount elements.
	u32 DemoCount;
};
