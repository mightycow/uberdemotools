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

	const u32 cloneOffset = (u32)allocator.Allocate((uptr)(inputLength + 1));
	char* const clone = (char*)allocator.GetStartAddress() + cloneOffset;
	memcpy(clone, input, inputLength);
	clone[inputLength] = '\0';

	udtString string;
	string.Allocator = &allocator;
	string.Offset = cloneOffset;
	string.Length = inputLength;
	string.ReservedBytes = inputLength + 1;

	return string;
}

udtString udtString::NewCloneFromRef(udtVMLinearAllocator& allocator, const udtString& input)
{
	// @NOTE: Can't forward the call to the other version because if 
	// allocator is the same as input.Allocator, then input might get relocated
	// after a call to allocator.Allocate.
	const u32 inputLength = input.GetLength();
	udtString clone = NewEmpty(allocator, inputLength + 1);
	char* const clonePtr = clone.GetWritePtr();
	memcpy(clonePtr, input.GetPtr(), inputLength);
	clonePtr[inputLength] = '\0';
	clone.Length = inputLength;

	return clone;
}

udtString udtString::NewCleanClone(udtVMLinearAllocator& allocator, udtProtocol::Id protocol, const char* input, u32 inputLength)
{
	udtString clone = NewClone(allocator, input, inputLength);
	udtString::CleanUp(clone, protocol);

	return clone;
}

udtString udtString::NewCleanCloneFromRef(udtVMLinearAllocator& allocator, udtProtocol::Id protocol, const udtString& input)
{
	udtString clone = NewCloneFromRef(allocator, input);
	udtString::CleanUp(clone, protocol);

	return clone;
}

udtString udtString::NewEmptyConstant()
{
	udtString string;
	string.Allocator = (udtVMLinearAllocator*)""; // We're being naughty.
	string.Offset = 0;
	string.Length = 0;
	string.ReservedBytes = 0;

	return string;
}

udtString udtString::NewConstRef(const char* readOnlyString, u32 length)
{
	udtString string;
	string.Allocator = (udtVMLinearAllocator*)readOnlyString; // We're being naughty.
	string.Offset = 0;
	string.Length = (readOnlyString != NULL) ? (length == (u32)InvalidLength ? (u32)strlen(readOnlyString) : length) : 0;
	string.ReservedBytes = 0;

	return string;
}

udtString udtString::NewEmpty(udtVMLinearAllocator& allocator, u32 reservedBytes)
{
	UDT_ASSERT_OR_RETURN_VALUE(reservedBytes > 0, NewEmptyConstant());

	const u32 offset = (u32)allocator.Allocate((uptr)reservedBytes);
	char* const memory = (char*)allocator.GetStartAddress() + offset;
	*memory = '\0';

	udtString string;
	string.Allocator = &allocator;
	string.Offset = offset;
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

udtString udtString::NewSubstringRef(const udtString& input, u32 offset, u32 length)
{
	UDT_ASSERT_OR_RETURN_VALUE(input.GetPtr() != NULL, NewEmptyConstant());
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
	if(input.IsReadOnly())
	{
		string.Allocator = (udtVMLinearAllocator*)((char*)input.Allocator + offset);
		string.Offset = 0;
		string.Length = length;
		string.ReservedBytes = 0;
	}
	else
	{
		string.Allocator = input.Allocator;
		string.Offset = input.Offset + offset;
		string.Length = length;
		string.ReservedBytes = input.ReservedBytes - offset;
	}

	return string;
}

udtString udtString::NewSubstringClone(udtVMLinearAllocator& allocator, const udtString& input, u32 offset, u32 length)
{
	UDT_ASSERT_OR_RETURN_VALUE(input.GetPtr() != NULL, NewEmptyConstant());
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

	if(input.Allocator != &allocator)
	{
		return udtString::NewClone(allocator, input.GetPtr() + offset, length);
	}

	udtString clone = NewEmpty(allocator, length + 1);
	char* const clonePtr = clone.GetWritePtr();
	memcpy(clonePtr, input.GetPtr() + offset, length);
	clonePtr[length] = '\0';
	clone.Length = length;

	return clone;
}

udtString udtString::NewNull()
{
	udtString string;
	string.Allocator = NULL;
	string.Offset = 0;
	string.Length = 0;
	string.ReservedBytes = 0;

	return string;
}

udtString udtString::NewCamelCaseClone(udtVMLinearAllocator& allocator, const udtString& name)
{
	if(udtString::IsNullOrEmpty(name))
	{
		return name;
	}

	udtString fixed = udtString::NewCloneFromRef(allocator, name);
	char* const start = fixed.GetWritePtr();
	char* dest = start;
	const char* src = start;
	*dest++ = (char)::tolower((int)*src++);
	while(*src)
	{
		if(src[0] != ' ')
		{
			*dest++ = *src++;
			continue;
		}

		if(src[1] == '\0')
		{
			break;
		}

		*dest++ = (char)::toupper((int)src[1]);
		src += 2;
	}

	*dest = '\0';
	fixed.Length = (u32)(dest - start);

	return fixed;
}

udtString udtString::NewFromAllocAndOffset(udtVMLinearAllocator& allocator, u32 offset, u32 length)
{
	assert(offset < allocator.GetCurrentByteCount());

	const u32 realLength = (length != (u32)InvalidLength) ? length : (u32)strlen(allocator.GetStringAt(offset));
	udtString result;
	result.Allocator = &allocator;
	result.Offset = offset;
	result.Length = realLength;
	result.ReservedBytes = realLength + 1;

	return result;
}

void udtString::Append(udtString& result, const udtString& input)
{
	UDT_ASSERT_OR_RETURN(result.Length + input.Length + 1 <= result.ReservedBytes);

	char* const resultString = result.GetWritePtr();
	memcpy(resultString + result.Length, input.GetPtr(), (size_t)input.Length);
	resultString[result.Length + input.Length] = '\0';
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

	char* newStringIter = result.GetWritePtr() + result.Length;
	for(u32 i = 0; i < stringCount; ++i)
	{
		if(strings[i] == NULL)
		{
			continue;
		}

		const u32 length = strings[i]->Length;
		memcpy(newStringIter, strings[i]->GetPtr(), (size_t)length);
		newStringIter += length;
	}
	*newStringIter = '\0';

	result.Length += extraLength;
}

void udtString::MakeLowerCase(udtString& result)
{
	UDT_ASSERT_OR_RETURN(result.ReservedBytes != 0);

	char* s = result.GetWritePtr();
	while(*s)
	{
		*s = (char)::tolower((int)*s);
		s++;
	}
}

void udtString::MakeUpperCase(udtString& result)
{ 
	UDT_ASSERT_OR_RETURN(result.ReservedBytes != 0);

	char* s = result.GetWritePtr();
	while(*s)
	{
		*s = (char)::toupper((int)*s);
		s++;
	}
}

bool udtString::ContainsNoCase(u32& charIndex, const udtString& input, const udtString& pattern)
{
	const char* const inputString = input.GetPtr();
	const char* const patternString = pattern.GetPtr();
	UDT_ASSERT_OR_RETURN_VALUE(inputString != NULL && patternString != NULL, false);

	if(pattern.Length > input.Length)
	{
		return false;
	}

	const u32 iend = input.Length - pattern.Length;
	for(u32 i = 0; i <= iend; ++i)
	{
		u32 j = 0;
		for(; patternString[j]; ++j)
		{
			if(::tolower(inputString[i + j]) != ::tolower(patternString[j]))
			{
				break;
			}
		}

		if(patternString[j] == '\0')
		{
			charIndex = i;
			return true;
		}
	}

	return false;
}

bool udtString::ContainsNoCase(const udtString& input, const udtString& pattern)
{
	u32 charIndex = 0;

	return ContainsNoCase(charIndex, input, pattern);
}

bool udtString::StartsWithNoCase(const udtString& input, const udtString& pattern)
{
	const char* const inputString = input.GetPtr();
	const char* const patternString = pattern.GetPtr();
	UDT_ASSERT_OR_RETURN_VALUE(inputString != NULL && patternString != NULL, false);

	if(pattern.Length > input.Length)
	{
		return false;
	}

	u32 j = 0;
	for(; patternString[j]; ++j)
	{
		if(::tolower(inputString[j]) != ::tolower(patternString[j]))
		{
			break;
		}
	}

	if(patternString[j] == '\0')
	{
		return true;
	}

	return false;
}

bool udtString::EndsWithNoCase(const udtString& input, const udtString& pattern)
{
	const char* const inputString = input.GetPtr();
	const char* const patternString = pattern.GetPtr();
	UDT_ASSERT_OR_RETURN_VALUE(inputString != NULL && patternString != NULL, false);

	if(pattern.Length > input.Length)
	{
		return false;
	}

	u32 i = input.Length - pattern.Length;
	u32 j = 0;
	for(; patternString[j]; ++j)
	{
		if(::tolower(inputString[i + j]) != ::tolower(patternString[j]))
		{
			return false;
		}
	}

	return true;
}

bool udtString::EqualsNoCase(const udtString& a, const udtString& b)
{
	const char* const aString = a.GetPtr();
	const char* const bString = b.GetPtr();
	UDT_ASSERT_OR_RETURN_VALUE(aString != NULL && bString != NULL, false);

	if(a.Length != b.Length)
	{
		return false;
	}

	for(u32 i = 0, end = a.Length; i < end; ++i)
	{
		if(::tolower(aString[i]) != ::tolower(bString[i]))
		{
			return false;
		}
	}

	return true;
}

bool udtString::Contains(u32& charIndex, const udtString& input, const udtString& pattern)
{
	const char* const inputString = input.GetPtr();
	const char* const patternString = pattern.GetPtr();
	UDT_ASSERT_OR_RETURN_VALUE(inputString != NULL && patternString != NULL, false);

	const char* const patternAddress = strstr(inputString, patternString);
	const bool found = patternAddress != NULL;
	if(found)
	{
		charIndex = (u32)(patternAddress - inputString);
	}

	return found;
}

bool udtString::Contains(const udtString& input, const udtString& pattern)
{
	const char* const inputString = input.GetPtr();
	const char* const patternString = pattern.GetPtr();
	UDT_ASSERT_OR_RETURN_VALUE(inputString != NULL && patternString != NULL, false);

	return strstr(inputString, patternString) != NULL;
}

bool udtString::StartsWith(const udtString& input, const udtString& pattern)
{
	const char* const inputString = input.GetPtr();
	const char* const patternString = pattern.GetPtr();
	UDT_ASSERT_OR_RETURN_VALUE(inputString != NULL && patternString != NULL, false);

	if(pattern.Length > input.Length)
	{
		return false;
	}

	for(u32 i = 0, end = pattern.Length; i < end; ++i)
	{
		if(inputString[i] != patternString[i])
		{
			return false;
		}
	}

	return true;
}

bool udtString::EndsWith(const udtString& input, const udtString& pattern)
{
	const char* const inputString = input.GetPtr();
	const char* const patternString = pattern.GetPtr();
	UDT_ASSERT_OR_RETURN_VALUE(inputString != NULL && patternString != NULL, false);

	if(pattern.Length > input.Length)
	{
		return false;
	}

	u32 i = input.Length - pattern.Length;
	u32 j = 0;
	for(; patternString[j]; ++j)
	{
		if(inputString[i + j] != patternString[j])
		{
			return false;
		}
	}

	return true;
}

bool udtString::Equals(const udtString& a, const udtString& b)
{
	const char* const aString = a.GetPtr();
	const char* const bString = b.GetPtr();
	UDT_ASSERT_OR_RETURN_VALUE(aString != NULL && bString != NULL, false);

	if(a.Length != b.Length)
	{
		return false;
	}

	for(u32 i = 0, end = a.Length; i < end; ++i)
	{
		if(aString[i] != bString[i])
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

bool udtString::ContainsNoCase(const udtString& input, const char* pattern)
{
	u32 charIndex = 0;

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

bool udtString::Contains(const udtString& input, const char* pattern)
{
	u32 charIndex = 0;

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

bool udtString::FindFirstCharacterListMatch(u32& index, const udtString& input, const udtString& charList, u32 offset)
{
	const char* const inputString = input.GetPtr();
	const char* const charListString = charList.GetPtr();
	UDT_ASSERT_OR_RETURN_VALUE(inputString != NULL && charListString != NULL, false);

	for(u32 i = offset; i < input.Length; ++i)
	{
		for(u32 j = 0; j < charList.Length; ++j)
		{
			if(inputString[i] == charListString[j])
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
	const char* const inputString = input.GetPtr();
	const char* const charListString = charList.GetPtr();
	UDT_ASSERT_OR_RETURN_VALUE(inputString != NULL && charListString != NULL, false);

	for(s32 i = (s32)input.Length - 1; i >= 0; --i)
	{
		for(u32 j = 0; j < charList.Length; ++j)
		{
			if(inputString[i] == charListString[j])
			{
				index = (u32)i;
				return true;
			}
		}
	}

	return false;
}

bool udtString::FindFirstCharacterMatch(u32& index, const udtString& input, char pattern, u32 offset)
{
	const char* const inputString = input.GetPtr();
	UDT_ASSERT_OR_RETURN_VALUE(inputString != NULL, false);

	for(u32 i = offset; i < input.Length; ++i)
	{
		if(inputString[i] == pattern)
		{
			index = i;
			return true;
		}
	}

	return false;
}

bool udtString::FindLastCharacterMatch(u32& index, const udtString& input, char pattern)
{ 
	const char* const inputString = input.GetPtr();
	UDT_ASSERT_OR_RETURN_VALUE(inputString != NULL, false);

	for(s32 i = (s32)input.Length - 1; i >= 0; --i)
	{
		if(inputString[i] == pattern)
		{
			index = (u32)i;
			return true;
		}
	}

	return false;
}

bool udtString::IsNullOrEmpty(const udtString& string)
{
	const char* const s = string.GetPtr();

	return s == NULL || *s == '\0';
}

bool udtString::IsNullOrEmpty(const char* string)
{
	return string == NULL || *string == '\0';
}

bool udtString::IsNull(const udtString& string)
{
	return string.GetPtr() == NULL;
}

bool udtString::IsEmpty(const udtString& string)
{
	const char* const s = string.GetPtr();

	return s != NULL && *s == '\0';
}

static UDT_FORCE_INLINE bool IsColorString(const char* s)
{
	return (s != NULL) && (s[0] == '^') && (s[1] != '\0') && (s[1] != '^');
}

static bool IsHexChar(char c)
{
	return
		(c >= 0x30 && c <= 0x39) || // digits
		(c >= 0x41 && c <= 0x46) || // uppercase A-F
		(c >= 0x61 && c <= 0x66);   // lowercase a-f
}

static bool IsLongOSPColorString(const udtString& string)
{
	const char* const s = string.GetPtr();
	if(string.GetLength() < 8 || s[0] != '^' || (char)::tolower(s[1]) != 'x')
	{
		return false;
	}

	for(u32 i = 2; i < 8; ++i)
	{
		if(!IsHexChar(s[i]))
		{
			return false;
		}
	}

	return true;
}

u32 CleanUpString(char* string, udtProtocol::Id protocol)
{
	u32 newLength = 0;
	char* const start = string;
	char* dest = start;
	char* source = start;
	char c;
	while((c = *source) != 0)
	{
		if(IsLongOSPColorString(udtString::NewConstRef(source)))
		{
			source += 7;
		}
		else if(IsColorString(source))
		{
			source++;
		}
		// Protocols up to 90 : only keep printable codes.
		// Protocols 91 and up: keep everything.
		else if(protocol >= udtProtocol::Dm91 ||
				(c >= 0x20 && c <= 0x7E))
		{
			*dest++ = c;
			newLength++;
		}
		source++;
	}
	*dest = '\0';

	return newLength;
}

void udtString::CleanUp(udtString& result, udtProtocol::Id protocol)
{
	if(IsNullOrEmpty(result))
	{
		return;
	}

	// Make sure we're not trying to modify a read-only string.
	UDT_ASSERT_OR_RETURN(result.ReservedBytes > 0);

	result.Length = CleanUpString(result.GetWritePtr(), protocol);
}

void udtString::RemoveCharacter(udtString& result, char toRemove)
{
	if(IsNullOrEmpty(result))
	{
		return;
	}

	// Make sure we're not trying to modify a read-only string.
	UDT_ASSERT_OR_RETURN(result.ReservedBytes > 0);

	u32 newLength = 0;
	char* const start = result.GetWritePtr();
	char* dest = start;
	char* source = start;
	char c;
	while((c = *source) != 0)
	{
		if(c != toRemove)
		{
			*dest++ = c;
			newLength++;
		}
		++source;
	}
	*dest = '\0';

	result.Length = newLength;
}

void udtString::TrimTrailingCharacter(udtString& result, char toRemove)
{
	if(IsNullOrEmpty(result))
	{
		return;
	}

	// Make sure we're not trying to modify a read-only string.
	UDT_ASSERT_OR_RETURN(result.ReservedBytes > 0);

	char* const start = result.GetWritePtr();
	char* it = start;
	char* lastChar = start;
	while(*it != 0)
	{
		if(*it != toRemove)
		{
			lastChar = it;
		}
		++it;
	}

	lastChar[1] = '\0';
	result.Length = (u32)(lastChar + 1 - start);
}

void udtString::RemoveEmCharacter(udtString& result)
{
	if(IsNullOrEmpty(result))
	{
		return;
	}

	// Make sure we're not trying to modify a read-only string.
	UDT_ASSERT_OR_RETURN(result.ReservedBytes > 0);

	char* const start = result.GetWritePtr();
	char* d = start;
	char* s = start;
	while(*s != '\0')
	{
		const char c = *s++;
		if(c == '\x19')
		{
			continue;
		}

		*d++ = c;
	}
	*d = '\0';
	result.Length = (u32)(d - start);
}

#if defined(UDT_WINDOWS)

#include <Windows.h>

// Includes the terminating character in the count.
static u32 UTF8toUTF16_GetCharCount(const char* utf8String)
{
	return (u32)MultiByteToWideChar(CP_UTF8, 0, utf8String, -1, nullptr, 0);
}

static bool UTF8toUTF16_Convert(wchar_t* utf16String, u32 utf16StringMaxChars, const char* utf8String)
{
	return MultiByteToWideChar(CP_UTF8, 0, utf8String, -1, utf16String, (int)utf16StringMaxChars) != 0;
}

// Includes the terminating character in the count.
static u32 UTF16toUTF8_GetCharCount(const wchar_t* utf16String)
{
	return (u32)WideCharToMultiByte(CP_UTF8, 0, utf16String, -1, nullptr, 0, nullptr, nullptr);
}

static u32 UTF16toUTF8_Convert(char* utf8String, u32 utf8StringMaxChars, const wchar_t* utf16String)
{
	return (u32)WideCharToMultiByte(CP_UTF8, 0, utf16String, -1, utf8String, (int)utf8StringMaxChars, nullptr, nullptr);
}

udtString udtString::NewFromUTF16(udtVMLinearAllocator& allocator, const wchar_t* utf16String)
{
	if(utf16String == NULL)
	{
		return udtString::NewEmptyConstant();
	}

	const uptr allocatorOffset = allocator.GetCurrentByteCount();
	const u32 charCount = UTF16toUTF8_GetCharCount(utf16String);
	const u32 offset = (u32)allocator.Allocate((uptr)charCount);
	char* const utf8String = (char*)allocator.GetStartAddress() + offset;
	if(!UTF16toUTF8_Convert(utf8String, charCount, utf16String))
	{
		allocator.SetCurrentByteCount(allocatorOffset);
		return udtString::NewEmptyConstant();
	}

	udtString string;
	string.Allocator = &allocator;
	string.Offset = offset;
	string.Length = charCount - 1;
	string.ReservedBytes = charCount;

	return string;
}

wchar_t* udtString::ConvertToUTF16(udtVMLinearAllocator& allocator, const udtString& utf8String)
{
	if(IsNull(utf8String))
	{
		return NULL;
	}

	const uptr allocatorOffset = allocator.GetCurrentByteCount();
	const u32 charCount = UTF8toUTF16_GetCharCount(utf8String.GetPtr());
	const u32 allocCharCount = charCount;
	wchar_t* const utf16String = (wchar_t*)allocator.AllocateAndGetAddress((uptr)allocCharCount * (uptr)sizeof(wchar_t));
	if(!UTF8toUTF16_Convert(utf16String, charCount, utf8String.GetPtr()))
	{
		allocator.SetCurrentByteCount(allocatorOffset);
		return NULL;
	}

	return utf16String;
}

#endif
