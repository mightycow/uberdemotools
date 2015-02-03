#include "string.hpp"

#include <string.h>
#include <ctype.h>
#include <assert.h>


#if defined(UDT_DEBUG)
#	define    UDT_ASSERT_OR_RETURN(exp)               assert((exp))
#	define    UDT_ASSERT_OR_RETURN_VALUE(exp, val)    assert((exp))
#else
#	define    UDT_ASSERT_OR_RETURN(exp)               if(!(exp)) return
#	define    UDT_ASSERT_OR_RETURN_VALUE(exp, val)    if(!(exp)) return (val)
#endif


udtString udtString::NewClone(udtVMLinearAllocator& allocator, const char* input, u32 inputLength)
{
	if(input == NULL)
	{
		input = "";
		inputLength = 0;
	}
	else if(inputLength == (u32)InvalidLength)
	{
		inputLength = (u32)strlen(input);
	}

	char* const clone = (char*)allocator.Allocate((uptr)(inputLength + 1));
	memcpy(clone, input, inputLength);
	clone[inputLength] = '\0';

	udtString string;
	string.String = clone;
	string.Length = inputLength;
	string.ReservedBytes = inputLength + 1;

	return string;
}

udtString udtString::NewCloneFromRef(udtVMLinearAllocator& allocator, const udtString& input)
{
	return NewClone(allocator, input.String, input.Length);
}

udtString udtString::NewEmptyConstant()
{
	udtString string;
	string.String = "";
	string.Length = 0;
	string.ReservedBytes = 0;

	return string;
}

udtString udtString::NewConstRef(const char* readOnlyString, u32 length)
{
	udtString string;
	string.String = (char*)readOnlyString; // We're being naughty.
	string.Length = (readOnlyString != NULL) ? (length == (u32)InvalidLength ? (u32)strlen(readOnlyString) : length) : 0;
	string.ReservedBytes = 0;

	return string;
}

udtString udtString::NewRef(const char* readOnlyString, u32 length, u32 reservedBytes)
{
	udtString string;
	string.String = (char*)readOnlyString; // We're being naughty.
	string.Length = length;
	string.ReservedBytes = reservedBytes;

	return string;
}

udtString udtString::NewEmpty(udtVMLinearAllocator& allocator, u32 reservedBytes)
{
	UDT_ASSERT_OR_RETURN_VALUE(reservedBytes > 0, NewEmptyConstant());

	char* const memory = (char*)allocator.Allocate((uptr)reservedBytes);
	*memory = '\0';

	udtString string;
	string.String = memory;
	string.Length = 0;
	string.ReservedBytes = reservedBytes;

	return string;
}

udtString udtString::NewFromConcatenating(udtVMLinearAllocator& allocator, const udtString& a, const udtString& b)
{
	const udtString* strings[2] = { &a, &b };

	return NewFromConcatenatingMultiple(allocator, strings, 2);
}

udtString udtString::NewFromConcatenatingMultiple(udtVMLinearAllocator& allocator, const udtString** strings, u32 stringCount)
{
	UDT_ASSERT_OR_RETURN_VALUE(strings != NULL && stringCount > 0, NewEmptyConstant());

	u32 newLength = 0;
	for(u32 i = 0; i < stringCount; ++i)
	{
		if(strings[i] == NULL)
		{
			continue;
		}

		newLength += strings[i]->Length;
	}

	udtString result = udtString::NewEmpty(allocator, newLength + 1);
	AppendMultiple(result, strings, stringCount);

	return result;
}

udtString udtString::NewFromConcatenatingMultiple(udtVMLinearAllocator& allocator, const char** strings, u32 stringCount)
{
	UDT_ASSERT_OR_RETURN_VALUE(strings != NULL && stringCount > 0, NewEmptyConstant());

	u32 newLength = 0;
	for(u32 i = 0; i < stringCount; ++i)
	{
		if(strings[i] == NULL)
		{
			continue;
		}

		newLength += (u32)strlen(strings[i]);
	}

	char* const newStringBuffer = (char*)allocator.Allocate((u32)newLength + 1);
	UDT_ASSERT_OR_RETURN_VALUE(newStringBuffer != NULL, NewEmptyConstant());

	char* newStringIter = newStringBuffer;
	for(u32 i = 0; i < stringCount; ++i)
	{
		if(strings[i] == NULL)
		{
			continue;
		}

		const u32 length = (u32)strlen(strings[i]);
		memcpy(newStringIter, strings[i], (size_t)length);
		newStringIter += length;
	}
	*newStringIter = '\0';

	udtString result;
	result.String = newStringBuffer;
	result.Length = newLength;
	result.ReservedBytes = newLength + 1;

	return result;
}

udtString udtString::NewSubstringRef(const udtString& input, u32 offset, u32 length)
{
	UDT_ASSERT_OR_RETURN_VALUE(input.String != NULL, NewEmptyConstant());
	if(offset >= input.Length)
	{
		return NewEmptyConstant();
	}

	if(length == (u32)InvalidLength)
	{
		length = input.Length - offset;
	}
	else if(offset + length > input.Length)
	{
		return NewEmptyConstant();
	}

	udtString string;
	string.String = input.String + offset;
	string.Length = length;
	string.ReservedBytes = input.ReservedBytes - offset;

	return string;
}

udtString udtString::NewSubstringClone(udtVMLinearAllocator& allocator, const udtString& input, u32 offset, u32 length)
{
	UDT_ASSERT_OR_RETURN_VALUE(input.String != NULL, NewEmptyConstant());
	if(offset >= input.Length)
	{
		return NewEmptyConstant();
	}

	if(length == (u32)InvalidLength)
	{
		length = input.Length - offset;
	}
	else if(offset + length > input.Length)
	{
		return NewEmptyConstant();
	}

	return udtString::NewClone(allocator, input.String + offset, length);
}

void udtString::Append(udtString& result, const udtString& input)
{
	UDT_ASSERT_OR_RETURN(result.Length + input.Length + 1 <= result.ReservedBytes);

	memcpy(result.String + result.Length, input.String, (size_t)input.Length);
	result.String[result.Length + input.Length] = '\0';
	result.Length += input.Length;
}

void udtString::AppendMultiple(udtString& result, const udtString** strings, u32 stringCount)
{
	UDT_ASSERT_OR_RETURN(strings != NULL && stringCount > 0);

	u32 extraLength = 0;
	for(u32 i = 0; i < stringCount; ++i)
	{
		if(strings[i] == NULL)
		{
			continue;
		}

		extraLength += strings[i]->Length;
	}

	UDT_ASSERT_OR_RETURN(result.Length + extraLength + 1 <= result.ReservedBytes);

	char* newStringIter = result.String + result.Length;
	for(u32 i = 0; i < stringCount; ++i)
	{
		if(strings[i] == NULL)
		{
			continue;
		}

		const u32 length = strings[i]->Length;
		memcpy(newStringIter, strings[i]->String, (size_t)length);
		newStringIter += length;
	}
	*newStringIter = '\0';

	result.Length += extraLength;
}

void udtString::MakeLowerCase(udtString& result)
{
	UDT_ASSERT_OR_RETURN(result.ReservedBytes != 0);

	char* s = result.String;
	while(*s)
	{
		*s = (char)::tolower((int)*s);
		s++;
	}
}

void udtString::MakeUpperCase(udtString& result)
{ 
	UDT_ASSERT_OR_RETURN(result.ReservedBytes != 0);

	char* s = result.String;
	while(*s)
	{
		*s = (char)::toupper((int)*s);
		s++;
	}
}

bool udtString::ContainsNoCase(u32& charIndex, const udtString& input, const udtString& pattern)
{
	UDT_ASSERT_OR_RETURN_VALUE(input.String != NULL && pattern.String != NULL, false);

	if(pattern.Length > input.Length)
	{
		return false;
	}

	const u32 iend = input.Length - pattern.Length;
	for(u32 i = 0; i <= iend; ++i)
	{
		u32 j = 0;
		for(; pattern.String[j]; ++j)
		{
			if(::tolower(input.String[i + j]) != ::tolower(pattern.String[j]))
			{
				break;
			}
		}

		if(pattern.String[j] == '\0')
		{
			charIndex = i;
			return true;
		}
	}

	return false;
}

bool udtString::StartsWithNoCase(const udtString& input, const udtString& pattern)
{
	UDT_ASSERT_OR_RETURN_VALUE(input.String != NULL && pattern.String != NULL, false);

	if(pattern.Length > input.Length)
	{
		return false;
	}

	u32 j = 0;
	for(; pattern.String[j]; ++j)
	{
		if(::tolower(input.String[j]) != ::tolower(pattern.String[j]))
		{
			break;
		}
	}

	if(pattern.String[j] == '\0')
	{
		return true;
	}

	return false;
}

bool udtString::EndsWithNoCase(const udtString& input, const udtString& pattern)
{
	UDT_ASSERT_OR_RETURN_VALUE(input.String != NULL && pattern.String != NULL, false);

	if(pattern.Length > input.Length)
	{
		return false;
	}

	u32 i = input.Length - pattern.Length;
	u32 j = 0;
	for(; pattern.String[j]; ++j)
	{
		if(::tolower(input.String[i + j]) != ::tolower(pattern.String[j]))
		{
			return false;
		}
	}

	return true;
}

bool udtString::EqualsNoCase(const udtString& a, const udtString& b)
{
	UDT_ASSERT_OR_RETURN_VALUE(a.String != NULL && b.String != NULL, false);

	if(a.Length != b.Length)
	{
		return false;
	}

	for(u32 i = 0, end = a.Length; i < end; ++i)
	{
		if(::tolower(a.String[i]) != ::tolower(b.String[i]))
		{
			return false;
		}
	}

	return true;
}

bool udtString::Contains(u32& charIndex, const udtString& input, const udtString& pattern)
{
	UDT_ASSERT_OR_RETURN_VALUE(input.String != NULL && pattern.String != NULL, false);

	const char* const patternAddress = strstr(input.String, pattern.String);
	const bool found = patternAddress != NULL;
	if(found)
	{
		charIndex = (u32)(patternAddress - input.String);
	}

	return found;
}

bool udtString::StartsWith(const udtString& input, const udtString& pattern)
{
	UDT_ASSERT_OR_RETURN_VALUE(input.String != NULL && pattern.String != NULL, false);

	if(pattern.Length > input.Length)
	{
		return false;
	}

	for(u32 i = 0, end = pattern.Length; i < end; ++i)
	{
		if(input.String[i] != pattern.String[i])
		{
			return false;
		}
	}

	return true;
}

bool udtString::EndsWith(const udtString& input, const udtString& pattern)
{
	UDT_ASSERT_OR_RETURN_VALUE(input.String != NULL && pattern.String != NULL, false);

	if(pattern.Length > input.Length)
	{
		return false;
	}

	u32 i = input.Length - pattern.Length;
	u32 j = 0;
	for(; pattern.String[j]; ++j)
	{
		if(input.String[i + j] != pattern.String[j])
		{
			return false;
		}
	}

	return true;
}

bool udtString::Equals(const udtString& a, const udtString& b)
{
	UDT_ASSERT_OR_RETURN_VALUE(a.String != NULL && b.String != NULL, false);

	if(a.Length != b.Length)
	{
		return false;
	}

	for(u32 i = 0, end = a.Length; i < end; ++i)
	{
		if(a.String[i] != b.String[i])
		{
			return false;
		}
	}

	return true;
}

bool udtString::ContainsNoCase(u32& charIndex, const udtString& input, const char* pattern)
{ 
	return ContainsNoCase(charIndex, input, udtString::NewConstRef(pattern));
}

bool udtString::StartsWithNoCase(const udtString& input, const char* pattern)
{ 
	return StartsWithNoCase(input, udtString::NewConstRef(pattern));
}

bool udtString::EndsWithNoCase(const udtString& input, const char* pattern)
{ 
	return EndsWithNoCase(input, udtString::NewConstRef(pattern));
}

bool udtString::EqualsNoCase(const udtString& a, const char* b)
{ 
	return EqualsNoCase(a, udtString::NewConstRef(b));
}

bool udtString::Contains(u32& charIndex, const udtString& input, const char* pattern)
{ 
	return Contains(charIndex, input, udtString::NewConstRef(pattern));
}

bool udtString::StartsWith(const udtString& input, const char* pattern)
{ 
	return StartsWith(input, udtString::NewConstRef(pattern));
}

bool udtString::EndsWith(const udtString& input, const char* pattern)
{ 
	return EndsWith(input, udtString::NewConstRef(pattern));
}

bool udtString::Equals(const udtString& a, const char* b)
{ 
	return Equals(a, udtString::NewConstRef(b));
}

bool udtString::FindFirstCharacterListMatch(u32& index, const udtString& input, const udtString& charList)
{
	UDT_ASSERT_OR_RETURN_VALUE(input.String != NULL && charList.String != NULL, false);

	for(u32 i = 0; i < input.Length; ++i)
	{
		for(u32 j = 0; j < charList.Length; ++j)
		{
			if(input.String[i] == charList.String[j])
			{
				index = i;
				return true;
			}
		}
	}

	return false;
}

bool udtString::FindLastCharacterListMatch(u32& index, const udtString& input, const udtString& charList)
{
	UDT_ASSERT_OR_RETURN_VALUE(input.String != NULL && charList.String != NULL, false);

	for(s32 i = (s32)input.Length - 1; i >= 0; --i)
	{
		for(u32 j = 0; j < charList.Length; ++j)
		{
			if(input.String[i] == charList.String[j])
			{
				index = (u32)i;
				return true;
			}
		}
	}

	return false;
}

bool udtString::FindFirstCharacterMatch(u32& index, const udtString& input, char pattern)
{
	UDT_ASSERT_OR_RETURN_VALUE(input.String != NULL, false);

	for(u32 i = 0; i < input.Length; ++i)
	{
		if(input.String[i] == pattern)
		{
			index = i;
			return true;
		}
	}

	return false;
}

bool udtString::FindLastCharacterMatch(u32& index, const udtString& input, char pattern)
{ 
	UDT_ASSERT_OR_RETURN_VALUE(input.String != NULL, false);

	for(s32 i = (s32)input.Length - 1; i >= 0; --i)
	{
		if(input.String[i] == pattern)
		{
			index = (u32)i;
			return true;
		}
	}

	return false;
}

bool udtString::IsNullOrEmpty(const udtString& string)
{
	return string.String == NULL || *string.String == '\0';
}

bool udtString::IsNullOrEmpty(const char* string)
{
	return string == NULL || *string == '\0';
}

static UDT_FORCE_INLINE bool IsColorString(const char* s)
{
	return (s != NULL) && (s[0] == '^') && (s[1] != '\0') && (s[1] != '^');
}

void udtString::CleanUp(udtString& result)
{
	if(result.String == NULL)
	{
		return;
	}

	// Make sure we're not trying to modify a read-only string.
	UDT_ASSERT_OR_RETURN(result.ReservedBytes > 0);

	u32 newLength = 0;
	char* dest = result.String;
	char* source = result.String;
	char c;
	while((c = *source) != 0)
	{
		if(IsColorString(source))
		{
			source++;
		}
		else if(c >= 0x20 && c <= 0x7E)
		{
			*dest++ = c;
			newLength++;
		}
		source++;
	}
	*dest = '\0';

	result.Length = newLength;
}
