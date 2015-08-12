#include "json_writer.hpp"


udtJSONWriter::udtJSONWriter()
{
	_stream = NULL;
	memset(_itemIndices, 0, sizeof(_itemIndices));
	_level = 0;
}

udtJSONWriter::~udtJSONWriter()
{
}

void udtJSONWriter::SetOutputStream(udtStream* stream)
{
	_stream = stream;
}

void udtJSONWriter::StartFile()
{
	memset(_itemIndices, 0, sizeof(_itemIndices));
	_level = 0;

	Write("{");
	++_level;
}

void udtJSONWriter::EndFile()
{
	Write("\r\n}");
}

void udtJSONWriter::StartObject()
{
	if(_itemIndices[_level] > 0)
	{
		Write(",");
	}

	WriteNewLine();
	Write("{");
	++_level;
	_itemIndices[_level] = 0;
}

void udtJSONWriter::StartObject(const char* name)
{
	if(_itemIndices[_level] > 0)
	{
		Write(",");
	}

	WriteNewLine();
	Write("\"");
	Write(name);
	Write("\":");
	WriteNewLine();
	Write("{");
	++_level;
	_itemIndices[_level] = 0;
}

void udtJSONWriter::EndObject()
{
	--_level;
	WriteNewLine();
	Write("}");
	++_itemIndices[_level];
}

void udtJSONWriter::StartArray()
{
	if(_itemIndices[_level] > 0)
	{
		Write(",");
	}

	WriteNewLine();
	Write("[");
	++_level;
	_itemIndices[_level] = 0;
}

void udtJSONWriter::StartArray(const char* name)
{
	if(_itemIndices[_level] > 0)
	{
		Write(",");
	}

	WriteNewLine();
	Write("\"");
	Write(name);
	Write("\":");
	WriteNewLine();
	Write("[");
	++_level;
	_itemIndices[_level] = 0;
}

void udtJSONWriter::EndArray()
{
	--_level;
	WriteNewLine();
	Write("]");
	++_itemIndices[_level];
}

void udtJSONWriter::WriteNewLine()
{
	Write("\r\n");
	for(u32 i = 0; i < _level; ++i)
	{
		Write("\t");
	}
}

void udtJSONWriter::Write(const char* string)
{
	_stream->Write(string, (u32)strlen(string), 1);
}

void udtJSONWriter::WriteIntValue(const char* name, s32 number)
{
	char numberString[64];
	sprintf(numberString, "%d", number);

	if(_itemIndices[_level] > 0)
	{
		Write(",");
	}

	WriteNewLine();
	Write("\"");
	Write(name);
	Write("\": ");
	Write(numberString);
	++_itemIndices[_level];
}

void udtJSONWriter::WriteBoolValue(const char* name, bool value)
{
	if(_itemIndices[_level] > 0)
	{
		Write(",");
	}

	WriteNewLine();
	Write("\"");
	Write(name);
	Write("\": ");
	Write(value ? "true" : "false");
	++_itemIndices[_level];
}

void udtJSONWriter::WriteStringValue(const char* name, const char* string)
{
	if(string == NULL)
	{
		return;
	}

	if(_itemIndices[_level] > 0)
	{
		Write(", ");
	}

	WriteNewLine();
	Write("\"");
	Write(name);
	Write("\": \"");
	Write(string);
	Write("\"");
	++_itemIndices[_level];
}
