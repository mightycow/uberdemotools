#include "parser_context.hpp"
#include "api.h"
#include "plug_in_chat.hpp"
#include "plug_in_game_state.hpp"
#include "analysis_obituaries.hpp"

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
	: PlugIns(1 << 24)
	, InputIndices(1 << 20)
{
	DemoCount = 0;

	PlugInAllocator.Init(1 << 16);

#define UDT_BASE_PARSER_ALLOCATOR_ITEM(Enum, Bytes) _parserAllocators[udtBaseParserAllocator::Enum].Init((uptr)(Bytes));
	UDT_BASE_PARSER_ALLOCATOR_LIST(UDT_BASE_PARSER_ALLOCATOR_ITEM)
#undef UDT_BASE_PARSER_ALLOCATOR_ITEM

#define UDT_BASE_PARSER_FIXED_SIZE_ARRAY_ITEM(Enum, Bytes) _fixedSizeArrays[udtBaseParserFixedSizeArray::Enum] = (u8*)malloc((size_t)(Bytes));
	UDT_BASE_PARSER_FIXED_SIZE_ARRAY_LIST(UDT_BASE_PARSER_FIXED_SIZE_ARRAY_ITEM)
#undef UDT_BASE_PARSER_FIXED_SIZE_ARRAY_ITEM

	Parser.SetAllocators(_parserAllocators, _fixedSizeArrays);
}

udtParserContext::~udtParserContext()
{
	DestroyPlugIns();

	for(u32 i = 0; i < (u32)UDT_COUNT_OF(_fixedSizeArrays); ++i)
	{
		free(_fixedSizeArrays[i]);
	}
}

void udtParserContext::Init(u32 demoCount, const u32* plugInIds, u32 plugInCount)
{
	DemoCount = demoCount;

	_plugInTempAllocator.Init(1 << 20);

	for(u32 i = 0; i < plugInCount; ++i)
	{
		const u32 plugInId = plugInIds[i];
		udtBaseParserPlugIn* const plugIn = (udtBaseParserPlugIn*)PlugInAllocator.Allocate(PlugInByteSizes[plugInId]);
		(*PlugInConstructors[plugInId])(plugIn);

		plugIn->Init(demoCount, _plugInTempAllocator);

		AddOnItem item;
		item.Id = (udtParserPlugIn::Id)plugInId;
		item.PlugIn = plugIn;
		PlugIns.Add(item);

		Parser.AddPlugIn(plugIn);
	}
}

void udtParserContext::ResetForNextDemo(bool keepPlugInData)
{
	if(keepPlugInData)
	{
		Context.Reset();
		//Parser.ResetForNextDemo();
	}
	else
	{
		DestroyPlugIns();
		InputIndices.Clear();
		Context.Reset();
		//Parser.ResetForNextDemo();
		PlugInAllocator.Clear();
		PlugIns.Clear();
	}
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

void udtParserContext::DestroyPlugIns()
{
	for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
	{
		PlugIns[i].PlugIn->~udtBaseParserPlugIn();
	}
}
