#include "parser_context.hpp"
#include "api.h"
#include "plug_in_chat.hpp"
#include "plug_in_game_state.hpp"
#include "analysis_obituaries.hpp"
#include "analysis_cut_by_pattern.hpp"

// For the placement new operator.
#include <new>
#include <assert.h>


typedef void(*PlugInConstructionFunc)(udtBaseParserPlugIn*);

template<typename T>
void ConstructPlugInT(udtBaseParserPlugIn* address)
{
	new (address) T;
}


#define UDT_PRIVATE_PLUG_IN_ITEM(Enum, Type, ApiType) (u32)sizeof(Type),
const u32 PlugInByteSizes[udtPrivateParserPlugIn::Count + 1] =
{
	UDT_PRIVATE_PLUG_IN_LIST(UDT_PRIVATE_PLUG_IN_ITEM)
	0
};
#undef UDT_PRIVATE_PLUG_IN_ITEM

#define UDT_PRIVATE_PLUG_IN_ITEM(Enum, Type, ApiType) &ConstructPlugInT<Type>,
const PlugInConstructionFunc PlugInConstructors[udtPrivateParserPlugIn::Count + 1] =
{
	UDT_PRIVATE_PLUG_IN_LIST(UDT_PRIVATE_PLUG_IN_ITEM)
	NULL
};
#undef UDT_PRIVATE_PLUG_IN_ITEM


udtParserContext::udtParserContext()
{
	DemoCount = 0;

	PlugIns.Init(1 << 16);
	InputIndices.Init(1 << 20);
	PlugInAllocator.Init(1 << 16);
	PlugInTempAllocator.Init(1 << 20);

	Parser.InitAllocators();
}

udtParserContext::~udtParserContext()
{
	DestroyPlugIns();
}

void udtParserContext::Init(u32 demoCount, const u32* plugInIds, u32 plugInCount)
{
	DemoCount = demoCount;

	for(u32 i = 0; i < plugInCount; ++i)
	{
		const u32 plugInId = plugInIds[i];
		udtBaseParserPlugIn* const plugIn = (udtBaseParserPlugIn*)PlugInAllocator.Allocate(PlugInByteSizes[plugInId]);
		(*PlugInConstructors[plugInId])(plugIn);

		plugIn->Init(demoCount, PlugInTempAllocator);

		AddOnItem item;
		item.Id = (udtParserPlugIn::Id)plugInId;
		item.PlugIn = plugIn;
		PlugIns.Add(item);

		Parser.AddPlugIn(plugIn);
	}
}

void udtParserContext::ResetForNextDemo(bool keepPlugInData)
{
	if(!keepPlugInData)
	{
		DestroyPlugIns();
		InputIndices.Clear();
		PlugInAllocator.Clear();
		PlugIns.Clear();
		Parser.PlugIns.Clear();
	}

	Context.Reset();
	PlugInTempAllocator.Clear();
}

bool udtParserContext::GetDataInfo(u32 demoIdx, u32 plugInId, void** itemBuffer, u32* itemCount)
{
	if(demoIdx >= DemoCount)
	{
		return false;
	}

	// Look for the right plug-in.
	for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
	{
		AddOnItem& plugIn = PlugIns[i];
		if(plugInId == (u32)plugIn.Id)
		{
			*itemBuffer = plugIn.PlugIn->GetFirstElementAddress(demoIdx);
			*itemCount = plugIn.PlugIn->GetElementCount(demoIdx);
			return true;
		}
	}

	return false;
}

void udtParserContext::GetPlugInById(udtBaseParserPlugIn*& plugIn, u32 plugInId)
{
	plugIn = NULL;
	for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
	{
		if(plugInId == (u32)PlugIns[i].Id)
		{
			plugIn = PlugIns[i].PlugIn;
			break;
		}
	}
}

void udtParserContext::DestroyPlugIns()
{
	for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
	{
		PlugIns[i].PlugIn->~udtBaseParserPlugIn();
	}
}
