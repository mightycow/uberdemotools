#pragma once


#include "parser_context.hpp"
#include "plug_in_custom_parser.hpp"


struct udtCuContext_s
{
	udtCuContext_s()
	{
		Commands.Init(UDT_MEMORY_PAGE_SIZE, "CuContext::CommandsArray");
		CommandStrings.Init(UDT_MEMORY_PAGE_SIZE, "CuContext::CommandStringsArray");
		CommandTokens.Init(UDT_MEMORY_PAGE_SIZE, "CuContext::CommandTokensArray");
		CommandTokenAddresses.Init(UDT_MEMORY_PAGE_SIZE, "CuContext::CommandTokensRawArray");
		StringAllocator.Init(UDT_MEMORY_PAGE_SIZE, "CuContext::Strings");
		ChangedEntities.Init(UDT_MEMORY_PAGE_SIZE, "CuContext::ChangedEntitiesArray");
		PlugIn.SetContext(this);
	}

	udtParserContext Context;
	udtCustomParsingPlugIn PlugIn;
	udtVMArray<udtCuCommandMessage> Commands;
	udtVMArray<udtString> CommandStrings;
	udtVMArray<udtString> CommandTokens;
	udtVMArray<const char*> CommandTokenAddresses;
	udtVMArray<const idEntityStateBase*> ChangedEntities;
	udtVMLinearAllocator StringAllocator;
	udtCuSnapshotMessage Snapshot;
	udtCuGamestateMessage GameState;
	udtCuMessageOutput Message;
	udtMessage InMessage;
};
