#include "parser_context.hpp"
#include "api.h"
#include "plug_in_chat.hpp"
#include "plug_in_game_state.hpp"

// For the placement new operator.
#include <new>


typedef void(*PlugInConstructionFunc)(udtBaseParserPlugIn*);

template<typename T>
void ConstructPlugInT(udtBaseParserPlugIn* address)
{
	new (address) T;
}

#define UDT_PLUG_IN_ITEM(Enum, Type, ApiType) (u32)sizeof(Type),
const u32 PlugInByteSizes[udtParserPlugIn::Count + 1] =
{
	UDT_PLUG_IN_LIST(UDT_PLUG_IN_ITEM)
	0
};
#undef UDT_PLUG_IN_ITEM

#define UDT_PLUG_IN_ITEM(Enum, Type, ApiType) &ConstructPlugInT<Type>,
const PlugInConstructionFunc PlugInConstructors[udtParserPlugIn::Count + 1] =
{
	UDT_PLUG_IN_LIST(UDT_PLUG_IN_ITEM)
	NULL
};
#undef UDT_PLUG_IN_ITEM


udtParserContext::udtParserContext()
{
	DemoCount = 0;
	PlugInAllocator.Init(1 << 24, 4096);
}

udtParserContext::~udtParserContext()
{
	DestroyPlugIns();
}

void udtParserContext::Reset()
{
	DestroyPlugIns();

	Context.Reset();
	Parser.Reset();
	PlugInAllocator.Clear();
	PlugIns.Clear();
}

void udtParserContext::CreateAndAddPlugIn(u32 plugInId)
{
	udtBaseParserPlugIn* const plugIn = (udtBaseParserPlugIn*)PlugInAllocator.Allocate(PlugInByteSizes[plugInId]);
	(*PlugInConstructors[plugInId])(plugIn);

	AddOnItem item;
	item.Id = (udtParserPlugIn::Id)plugInId;
	item.PlugIn = plugIn;
	PlugIns.Add(item);

	Parser.AddPlugIn(plugIn);
}

bool udtParserContext::GetDataInfo(u32 demoIdx, u32 plugInId, void** itemBuffer, u32* itemCount)
{
	if(demoIdx >= DemoCount)
	{
		return false;
	}

	s32 plugInIdx = -1;
	for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
	{
		if(plugInId == (u32)PlugIns[i].Id)
		{
			plugInIdx = (s32)i;
			break;
		}
	}

	if(plugInIdx == -1)
	{
		return false;
	}

	const u32 itemIdx = demoIdx * PlugIns.GetSize() + plugInIdx;
	*itemBuffer = DataItems[itemIdx].Items;
	*itemCount = DataItems[itemIdx].ItemCount;

	return true;
}

void udtParserContext::StartDemo()
{
	PlugInData data;
	data.ItemCount = 0;
	data.Items = NULL;
	for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
	{
		DataItems.Add(data);
	}

	++DemoCount;
}

void udtParserContext::EndDemo()
{
	if(DemoCount == 1)
	{
		for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
		{
			DataItems[i].Items = (u8*)PlugIns[i].PlugIn->GetFirstElementAddress();
			DataItems[i].ItemCount = PlugIns[i].PlugIn->GetElementCount();
		}

		return;
	}

	const u32 current = (DemoCount - 1) * PlugIns.GetSize();
	const u32 previous = (DemoCount - 2) * PlugIns.GetSize();
	for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
	{
		const u32 itemCount = PlugIns[i].PlugIn->GetElementCount() - DataItems[previous + i].ItemCount;
		const u32 itemSize = PlugIns[i].PlugIn->GetElementSize();
		DataItems[current + i].ItemCount = itemCount;
		DataItems[current + i].Items = DataItems[previous + i].Items + itemCount * itemSize;
	}
}

void udtParserContext::DestroyPlugIns()
{
	for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
	{
		PlugIns[i].PlugIn->~udtBaseParserPlugIn();
	}
}
