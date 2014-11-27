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

public:
	udtParserContext();
	~udtParserContext();

	void Reset();
	void CreateAndAddPlugIn(u32 plugInId);
	bool GetDataInfo(u32 plugInId, void** buffer, u32* count);

private:
	void DestroyPlugIns();

public:
	udtContext Context;
	udtBaseParser Parser;
	udtVMLinearAllocator PlugInAllocator;
	udtVMArray<AddOnItem> PlugIns;
};