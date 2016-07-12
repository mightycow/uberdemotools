#pragma once


#include "parser_context.hpp"
#include "plug_in_custom_parser.hpp"


struct udtCuContext_s
{
	udtCuContext_s()
	{
		PlugIn.SetContext(this);
	}

	udtParserContext Context;
	udtCustomParsingPlugIn PlugIn;
	udtVMArray<udtCuCommandMessage> Commands { "CuContext::CommandsArray" };
	udtVMArray<udtString> CommandStrings { "CuContext::CommandStringsArray" };
	udtVMArray<udtString> CommandTokens { "CuContext::CommandTokensArray" };
	udtVMArray<const char*> CommandTokenAddresses { "CuContext::CommandTokensRawArray" };
	udtVMArray<const idEntityStateBase*> ChangedEntities { "CuContext::ChangedEntitiesArray" };
	udtVMLinearAllocator StringAllocator { "CuContext::Strings" };
	udtCuSnapshotMessage Snapshot;
	udtCuGamestateMessage GameState;
	udtCuMessageOutput Message;
	udtMessage InMessage;
};
