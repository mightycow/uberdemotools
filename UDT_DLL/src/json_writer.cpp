#include "json_writer.hpp"


static bool UTF8_NextCodePoint(u32& codePoint, u32& byteCount, const char* input)
{
	if(*input == '\0')
	{
		return false;
	}

	const u8 byte0 = (u8)input[0];
	if(byte0 <= 127)
	{
		codePoint = (u32)byte0;
		byteCount = 1;
		return true;
	}

	// Starts with 110?
	if((byte0 >> 5) == 6)
	{
		const u8 byte1 = (u8)input[1];
		codePoint = ((u32)byte1 & 63) | (((u32)byte0 & 31) << 6);
		byteCount = 2;
		return true;
	}

	// Starts with 1110?
	if((byte0 >> 4) == 14)
	{
		const u8 byte1 = (u8)input[1];
		const u8 byte2 = (u8)input[2];
		codePoint = ((u32)byte2 & 63) | (((u32)byte1 & 63) << 6) | (((u32)byte0 & 15) << 12);
		byteCount = 3;
		return true;
	}
	
	// Starts with 11110?
	if((byte0 >> 3) == 30)
	{
		const u8 byte1 = (u8)input[1];
		const u8 byte2 = (u8)input[2];
		const u8 byte3 = (u8)input[3];
		codePoint = ((u32)byte3 & 63) | (((u32)byte2 & 63) << 6) | (((u32)byte1 & 63) << 12) | (((u32)byte0 & 7) << 18);
		byteCount = 4;
		return true;
	}
	
	return false;
}

struct ShortEscape
{
	u32 CodePoint;
	char OutputChar;
};

#define ENTRY(HexValue, Char) { 0x##HexValue, Char }
static const ShortEscape ShortEscapeCodePoints[] =
{
	ENTRY(0022, '"'),
	ENTRY(005C, '\\'),
	ENTRY(002F, '/'),
	ENTRY(0008, 'b'),
	ENTRY(000C, 'f'),
	ENTRY(000A, 'n'),
	ENTRY(000D, 'r'),
	ENTRY(0009, 't')
};
#undef ENTRY

static const char HexDigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

static bool UTF8_NeedsEscaping(u32& newLength, u32 codePoint)
{
	for(u32 i = 0; i < (u32)UDT_COUNT_OF(ShortEscapeCodePoints); ++i)
	{
		if(codePoint == ShortEscapeCodePoints[i].CodePoint)
		{
			newLength = 2; // Of form: "\A".
			return true;
		}
	}

	// Range: 0x0000 - 0x001F
	if(codePoint <= 0x001F)
	{
		newLength = 6; // Of form: "\uABCD".
		return true;
	}

	return false;
}

static void UTF8_WriteCodePoint(u32& newLength, char* output, u32 codePoint, const char* input)
{
	for(u32 i = 0; i < (u32)UDT_COUNT_OF(ShortEscapeCodePoints); ++i)
	{
		if(codePoint == ShortEscapeCodePoints[i].CodePoint)
		{
			newLength = 2;
			output[0] = '\\';
			output[1] = ShortEscapeCodePoints[i].OutputChar;
			return;
		}
	}

	// Range: 0x0000 - 0x001F
	if(codePoint <= 0x001F)
	{
		newLength = 6;
		output[0] = '\\';
		output[1] = 'u';
		output[2] = HexDigits[(codePoint >> 12) & 15];
		output[3] = HexDigits[(codePoint >> 8) & 15];
		output[4] = HexDigits[(codePoint >> 4) & 15];
		output[5] = HexDigits[codePoint & 15];
		return;
	}

	for(u32 i = 0; i < newLength; ++i)
	{
		output[i] = input[i];
	}
}

static u32 UTF8_LengthForJSON(const char* input)
{
	const char* s = input;
	u32 codePoint = 0;
	u32 byteCount = 0;
	u32 newLength = 0;
	while(UTF8_NextCodePoint(codePoint, byteCount, s))
	{
		s += byteCount;
		UTF8_NeedsEscaping(byteCount, codePoint);
		newLength += byteCount;
	}

	return newLength;
}

static void UTF8_CleanForJSON(char* output, const char* input)
{
	const char* in = input;
	char* out = output;
	u32 codePoint = 0;
	u32 inputByteCount = 0;
	while(UTF8_NextCodePoint(codePoint, inputByteCount, in))
	{
		u32 outputByteCount = inputByteCount;
		UTF8_WriteCodePoint(outputByteCount, out, codePoint, in);
		in += inputByteCount;
		out += outputByteCount;
	}
	*out = '\0';
}

static char* UTF8_CloneForJSON(udtVMLinearAllocator& allocator, const char* input)
{
	const u32 newLength = UTF8_LengthForJSON(input);
	char* const output = (char*)allocator.AllocateAndGetAddress((uptr)newLength + 1);
	UTF8_CleanForJSON(output, input);

	return output;
}


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

void udtJSONWriter::CleanAndWrite(const char* string)
{
	char* cleanCopy = UTF8_CloneForJSON(_stringAllocator, string);
	Write(cleanCopy);
	_stringAllocator.Clear();
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
	CleanAndWrite(string);
	//Write(string);
	Write("\"");
	++_itemIndices[_level];
}
