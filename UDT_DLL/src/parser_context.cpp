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

bool udtParserContext::GetDataInfo(u32 plugInId, void** buffer, u32* count)
{
	for(u32 i = 0; i < PlugIns.GetSize(); ++i)
	{
		if(plugInId == (u32)PlugIns[i].Id)
		{
			udtBaseParserPlugIn* const plugIn = PlugIns[i].PlugIn;
			*buffer = plugIn->GetFirstElementAddress();
			*count = plugIn->GetElementCount();
			return true;
		}
	}

	return false;
}

void udtParserContext::DestroyPlugIns()
{
	for(u32 i = 0; i < PlugIns.GetSize(); ++i)
	{
		PlugIns[i].PlugIn->~udtBaseParserPlugIn();
	}
}
