#pragma once


#include "context.hpp"
#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"


#define UDT_PRIVATE_PLUG_IN_LIST(N) \
	UDT_PLUG_IN_LIST(N) \
	N(CutByPattern, udtCutByPatternPlugIn,     udtCutSection) \
	N(ConvertToUDT, udtParserPlugInQuakeToUDT, udtNothing)

#define UDT_PRIVATE_PLUG_IN_ITEM(Enum, Type, OutputType) Enum,
struct udtPrivateParserPlugIn
{
	enum Id
	{
		UDT_PRIVATE_PLUG_IN_LIST(UDT_PRIVATE_PLUG_IN_ITEM)
		Count
	};
};
#undef UDT_PRIVATE_PLUG_IN_ITEM


// Don't ever allocate an instance of this on the stack.
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
	void GetPlugInById(udtBaseParserPlugIn*& plugIn, u32 plugInId);

private:
	void DestroyPlugIns();

public:
	udtContext Context;
	udtBaseParser Parser;
	udtVMLinearAllocator PlugInAllocator;
	udtVMArrayWithAlloc<AddOnItem> PlugIns; // There is only 1 (shared) plug-in instance for each plug-in ID passed.
	udtVMArrayWithAlloc<u32> InputIndices;
	udtVMLinearAllocator PlugInTempAllocator;
	u32 DemoCount;
};
