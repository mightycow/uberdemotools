#pragma once


#include "context.hpp"
#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"
#include "modifier_context.hpp"
#include "json_writer_context.hpp"
#include "read_only_sequ_file_stream.hpp"


#define UDT_PRIVATE_PLUG_IN_LIST(N) \
	UDT_PLUG_IN_LIST(N) \
	N(FindPatterns, "", udtPatternSearchPlugIn,    udtCutSection) \
	N(ConvertToUDT, "", udtParserPlugInQuakeToUDT, udtNothing)

#define UDT_PRIVATE_PLUG_IN_ITEM(Enum, Desc, Type, OutputType) Enum,
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
struct udtParserContext_s
{
private:
	struct AddOnItem
	{
		udtBaseParserPlugIn* PlugIn;
		udtParserPlugIn::Id Id;
	};

public:
	udtParserContext_s();
	~udtParserContext_s();

	bool Init(u32 demoCount, const u32* plugInIds = NULL, u32 plugInCount = 0); // Called once for all.
	void ResetForNextDemo(bool keepPlugInData); // Called once per demo processed.
	bool CopyBuffersStruct(u32 plugInId, void* buffersStruct);
	void UpdatePlugInBufferStructs();
	u32  GetDemoCount() const { return DemoCount; }
	void GetPlugInById(udtBaseParserPlugIn*& plugIn, u32 plugInId);

private:
	void DestroyPlugIns();

public:
	udtContext Context;
	udtBaseParser Parser;
	udtModifierContext ModifierContext;
	udtJSONWriterContext JSONWriterContext;
	udtVMLinearAllocator PlugInAllocator { "ParserContext::PlugIn" };
	udtVMArray<AddOnItem> PlugIns { "ParserContext::PlugInsArray" }; // There is only 1 (shared) plug-in instance for each plug-in ID passed.
	udtVMArray<u32> InputIndices { "ParserContext::InputIndicesArray" };
	udtVMLinearAllocator PlugInTempAllocator { "ParserContext::PlugInTemp" };
#if defined(UDT_WINDOWS)
	udtReadOnlySequentialFileStream DemoReader;
#endif
	u32 DemoCount;
};


#if defined(UDT_WINDOWS)

struct udtStreamScopeGuard
{
	udtStreamScopeGuard(udtStream& stream)
		: _stream(stream)
	{
	}

	~udtStreamScopeGuard()
	{
		_stream.Close();
	}

private:
	UDT_NO_COPY_SEMANTICS(udtStreamScopeGuard);

	udtStream& _stream;
};

#endif


#if defined(UDT_WINDOWS)
#	define UDT_INIT_DEMO_FILE_READER(name, filePath, context) \
		udtReadOnlySequentialFileStream& name = context->DemoReader; \
		udtStreamScopeGuard name##ScopeGuard(name); \
		if(!name.Open(filePath)) return false;
#	define UDT_INIT_DEMO_FILE_READER_AT(name, filePath, context, offset) \
		udtReadOnlySequentialFileStream& name = context->DemoReader; \
		udtStreamScopeGuard name##ScopeGuard(name); \
		if(!name.Open(filePath, offset)) return false;
#else
#	define UDT_INIT_DEMO_FILE_READER(name, filePath, context) \
		udtFileStream name; \
		if(!name.Open(filePath, udtFileOpenMode::Read)) return false;
#	define UDT_INIT_DEMO_FILE_READER_AT(name, filePath, context, offset) \
		udtFileStream name; \
		if(!name.Open(filePath, udtFileOpenMode::Read)) return false; \
		if(fileOffset > 0 && name.Seek((s32)offset, udtSeekOrigin::Start) != 0) return false;

#endif
