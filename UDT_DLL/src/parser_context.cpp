#include "parser_context.hpp"
#include "api.h"
#include "plug_in_chat.hpp"
#include "plug_in_game_state.hpp"
#include "analysis_obituaries.hpp"
#include "analysis_awards.hpp"

// For the placement new operator.
#include <new>
#include <assert.h>


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
	PlugInCountPerDemo = 0;
	PlugInAllocator.Init(1 << 24, UDT_MEMORY_PAGE_SIZE);
}

udtParserContext::~udtParserContext()
{
	DestroyPlugIns();
}

void udtParserContext::Reset()
{
	DestroyPlugIns();

	DemoCount = 0;
	PlugInCountPerDemo = 0;
	InputIndices.Clear();
	Context.Reset();
	Parser.Reset();
	PlugInAllocator.Clear();
	PlugIns.Clear();
}

void udtParserContext::ResetButKeepPlugInData()
{
	Context.Reset();
	Parser.Reset();
}

void udtParserContext::CreateAndAddPlugIns(const u32* plugInIds, u32 plugInCount)
{
	assert(PlugInCountPerDemo == 0 || PlugInCountPerDemo == plugInCount);

	for(u32 i = 0; i < plugInCount; ++i)
	{
		const u32 plugInId = plugInIds[i];
		udtBaseParserPlugIn* const plugIn = (udtBaseParserPlugIn*)PlugInAllocator.Allocate(PlugInByteSizes[plugInId]);
		(*PlugInConstructors[plugInId])(plugIn);

		AddOnItem item;
		item.Id = (udtParserPlugIn::Id)plugInId;
		item.PlugIn = plugIn;
		PlugIns.Add(item);

		Parser.AddPlugIn(plugIn);
	}

	++DemoCount;
	PlugInCountPerDemo = plugInCount;
}

bool udtParserContext::GetDataInfo(u32 demoIdx, u32 plugInId, void** itemBuffer, u32* itemCount)
{
	if(demoIdx >= DemoCount)
	{
		return false;
	}

	const u32 startIdx = demoIdx * PlugInCountPerDemo;
	const u32 endIdx = startIdx + PlugInCountPerDemo;
	for(u32 i = startIdx; i < endIdx; ++i)
	{
		if(plugInId == (u32)PlugIns[i].Id)
		{
			*itemBuffer = PlugIns[i].PlugIn->GetFirstElementAddress();
			*itemCount = PlugIns[i].PlugIn->GetElementCount();
			return true;
		}
	}

	return false;
}

void udtParserContext::DestroyPlugIns()
{
	for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
	{
		PlugIns[i].PlugIn->~udtBaseParserPlugIn();
	}
}
