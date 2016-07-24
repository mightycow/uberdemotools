#include "parser_context.hpp"
#include "analysis_pattern_base.hpp"
#include "plug_in_chat.hpp"
#include "plug_in_game_state.hpp"
#include "plug_in_pattern_search.hpp"
#include "plug_in_converter_quake_to_udt.hpp"
#include "plug_in_stats.hpp"
#include "plug_in_raw_commands.hpp"
#include "plug_in_raw_config_strings.hpp"
#include "plug_in_captures.hpp"
#include "plug_in_obituaries.hpp"
#include "plug_in_scores.hpp"

// For the placement new operator.
#include <new>
#include <assert.h>


typedef void(*PlugInConstructionFunc)(udtBaseParserPlugIn*);

template<typename T>
void ConstructPlugInT(udtBaseParserPlugIn* address)
{
	new (address) T;
}


#define UDT_PRIVATE_PLUG_IN_ITEM(Enum, Desc, Type, OutputType) (u32)sizeof(Type),
const u32 PlugInByteSizes[udtPrivateParserPlugIn::Count + 1] =
{
	UDT_PRIVATE_PLUG_IN_LIST(UDT_PRIVATE_PLUG_IN_ITEM)
	0
};
#undef UDT_PRIVATE_PLUG_IN_ITEM

#define UDT_PRIVATE_PLUG_IN_ITEM(Enum, Desc, Type, OutputType) &ConstructPlugInT<Type>,
const PlugInConstructionFunc PlugInConstructors[udtPrivateParserPlugIn::Count + 1] =
{
	UDT_PRIVATE_PLUG_IN_LIST(UDT_PRIVATE_PLUG_IN_ITEM)
	NULL
};
#undef UDT_PRIVATE_PLUG_IN_ITEM

#define UDT_PRIVATE_PLUG_IN_ITEM(Enum, Desc, Type, OutputType) sizeof(Type) +
static const size_t SizeOfAllPlugIns = UDT_PRIVATE_PLUG_IN_LIST(UDT_PRIVATE_PLUG_IN_ITEM) 0;
#undef UDT_PRIVATE_PLUG_IN_ITEM


udtParserContext_s::udtParserContext_s()
{
	DemoCount = 0;

	// @NOTE: This data can never be relocated.
	PlugInAllocator.Init((uptr)SizeOfAllPlugIns);
}

udtParserContext_s::~udtParserContext_s()
{
	DestroyPlugIns();
}

bool udtParserContext_s::Init(u32 demoCount, const u32* plugInIds, u32 plugInCount)
{
#if defined(UDT_WINDOWS)
	if(!DemoReader.Init())
	{
		return false;
	}
#endif

	DemoCount = demoCount;

	for(u32 i = 0; i < plugInCount; ++i)
	{
		const u32 plugInId = plugInIds[i];
		udtBaseParserPlugIn* const plugIn = (udtBaseParserPlugIn*)PlugInAllocator.AllocateAndGetAddress(PlugInByteSizes[plugInId]);
		(*PlugInConstructors[plugInId])(plugIn);

		plugIn->Init(demoCount, PlugInTempAllocator);

		AddOnItem item;
		item.Id = (udtParserPlugIn::Id)plugInId;
		item.PlugIn = plugIn;
		PlugIns.Add(item);

		Parser.AddPlugIn(plugIn);
	}

	return true;
}

void udtParserContext_s::ResetForNextDemo(bool keepPlugInData)
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

bool udtParserContext_s::CopyBuffersStruct(u32 plugInId, void* buffersStruct)
{
	// Look for the right plug-in.
	for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
	{
		AddOnItem& plugIn = PlugIns[i];
		if(plugInId == (u32)plugIn.Id)
		{
			plugIn.PlugIn->CopyBuffersStruct(buffersStruct);
			return true;
		}
	}

	return false;
}

void udtParserContext_s::UpdatePlugInBufferStructs()
{
	for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
	{
		AddOnItem& plugIn = PlugIns[i];
		plugIn.PlugIn->UpdateBufferStruct();
	}
}

void udtParserContext_s::GetPlugInById(udtBaseParserPlugIn*& plugIn, u32 plugInId)
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

void udtParserContext_s::DestroyPlugIns()
{
	for(u32 i = 0, count = PlugIns.GetSize(); i < count; ++i)
	{
		PlugIns[i].PlugIn->~udtBaseParserPlugIn();
	}
}
