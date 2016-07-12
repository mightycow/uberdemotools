#pragma once


#include "stream.hpp"


struct udtJSONWriter
{
public:
	udtJSONWriter();
	~udtJSONWriter();

	void SetOutputStream(udtStream* stream);
	
	void StartFile();
	void EndFile();
	void StartObject();
	void StartObject(const char* name);
	void EndObject();
	void StartArray();
	void StartArray(const char* name);
	void EndArray();

	void WriteIntValue(const char* name, s32 number);
	void WriteBoolValue(const char* name, bool value);
	void WriteStringValue(const char* name, const char* string);

private:
	UDT_NO_COPY_SEMANTICS(udtJSONWriter);

	void WriteNewLine();
	void Write(const char* string);
	void CleanAndWrite(const char* string);

	udtVMLinearAllocator _stringAllocator { "JSONWriter::Strings" };
	udtStream* _stream;
	u32 _itemIndices[16];
	u32 _level;
};
