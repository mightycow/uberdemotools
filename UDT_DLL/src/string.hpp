#pragma once


#include "linear_allocator.hpp"


struct udtString
{
	static udtString NewClone(udtVMLinearAllocator& allocator, const char* input, u32 inputLength = 0);
	static udtString NewCloneFromRef(udtVMLinearAllocator& allocator, const udtString& input);
	static udtString NewEmptyConstant();
	static udtString NewConstRef(const char* readOnlyString, u32 length = 0);
	static udtString NewEmpty(udtVMLinearAllocator& allocator, u32 reservedBytes);
	static udtString NewFromConcatenating(udtVMLinearAllocator& allocator, const udtString& a, const udtString& b);
	static udtString NewFromConcatenatingMultiple(udtVMLinearAllocator& allocator, const udtString** strings, u32 stringCount);
	static udtString NewSubstringRef(const udtString& input, u32 offset, u32 length = 0);

	static void Append(udtString& result, const udtString& input);
	static void AppendMultiple(udtString& result, const udtString** strings, u32 stringCount);

	static void MakeLowerCase(udtString& result);
	static void MakeUpperCase(udtString& result);

	static bool ContainsNoCase(u32& charIndex, const udtString& input, const udtString& pattern);
	static bool StartsWithNoCase(const udtString& input, const udtString& pattern);
	static bool EndsWithNoCase(const udtString& input, const udtString& pattern);
	static bool EqualsNoCase(const udtString& a, const udtString& b);

	static bool Contains(u32& charIndex, const udtString& input, const udtString& pattern);
	static bool StartsWith(const udtString& input, const udtString& pattern);
	static bool EndsWith(const udtString& input, const udtString& pattern);
	static bool Equals(const udtString& a, const udtString& b);

	static bool ContainsNoCase(u32& charIndex, const udtString& input, const char* pattern);
	static bool StartsWithNoCase(const udtString& input, const char* pattern);
	static bool EndsWithNoCase(const udtString& input, const char* pattern);
	static bool EqualsNoCase(const udtString& a, const char* b);

	static bool Contains(u32& charIndex, const udtString& input, const char* pattern);
	static bool StartsWith(const udtString& input, const char* pattern);
	static bool EndsWith(const udtString& input, const char* pattern);
	static bool Equals(const udtString& a, const char* b);

	static bool FindFirstCharacterListMatch(u32& index, const udtString& input, const udtString& charList);
	static bool FindLastCharacterListMatch(u32& index, const udtString& input, const udtString& charList);
	static bool FindFirstCharacterMatch(u32& index, const udtString& input, char pattern);
	static bool FindLastCharacterMatch(u32& index, const udtString& input, char pattern);

	static bool IsNullOrEmpty(const udtString& string);

	static void CleanUp(udtString& result); // Strips Quake color codes and keeps printable codes only.

	char* String;
	u32 Length;
	u32 ReservedBytes; // 0 when read-only.
};
