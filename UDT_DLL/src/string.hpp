#pragma once


#include "linear_allocator.hpp"

#include <assert.h>


struct udtString
{
	enum Constants
	{
		InvalidLength = -1
	};

	static udtString NewClone(udtVMLinearAllocator& allocator, const char* input, u32 inputLength = (u32)InvalidLength);
	static udtString NewCloneFromRef(udtVMLinearAllocator& allocator, const udtString& input);
	static udtString NewCleanClone(udtVMLinearAllocator& allocator, udtProtocol::Id protocol, const char* input, u32 inputLength = (u32)InvalidLength);
	static udtString NewCleanCloneFromRef(udtVMLinearAllocator& allocator, udtProtocol::Id protocol, const udtString& input);
	static udtString NewEmptyConstant();
	static udtString NewConstRef(const char* readOnlyString, u32 length = (u32)InvalidLength);
	static udtString NewEmpty(udtVMLinearAllocator& allocator, u32 reservedBytes);
	static udtString NewFromConcatenating(udtVMLinearAllocator& allocator, const udtString& a, const udtString& b);
	static udtString NewFromConcatenatingMultiple(udtVMLinearAllocator& allocator, const udtString** strings, u32 stringCount);
	static udtString NewSubstringRef(const udtString& input, u32 offset, u32 length = (u32)InvalidLength);
	static udtString NewSubstringClone(udtVMLinearAllocator& allocator, const udtString& input, u32 offset, u32 length = (u32)InvalidLength);
	static udtString NewNull();
	static udtString NewCamelCaseClone(udtVMLinearAllocator& allocator, const udtString& name);
	static udtString NewFromAllocAndOffset(udtVMLinearAllocator& allocator, u32 offset, u32 length = (u32)InvalidLength);

	static void Append(udtString& result, const udtString& input);
	static void AppendMultiple(udtString& result, const udtString** strings, u32 stringCount);

	static void MakeLowerCase(udtString& result);
	static void MakeUpperCase(udtString& result);

	static bool ContainsNoCase(u32& charIndex, const udtString& input, const udtString& pattern);
	static bool ContainsNoCase(const udtString& input, const udtString& pattern);
	static bool StartsWithNoCase(const udtString& input, const udtString& pattern);
	static bool EndsWithNoCase(const udtString& input, const udtString& pattern);
	static bool EqualsNoCase(const udtString& a, const udtString& b);

	static bool Contains(u32& charIndex, const udtString& input, const udtString& pattern);
	static bool Contains(const udtString& input, const udtString& pattern);
	static bool StartsWith(const udtString& input, const udtString& pattern);
	static bool EndsWith(const udtString& input, const udtString& pattern);
	static bool Equals(const udtString& a, const udtString& b);

	static bool ContainsNoCase(u32& charIndex, const udtString& input, const char* pattern);
	static bool ContainsNoCase(const udtString& input, const char* pattern);
	static bool StartsWithNoCase(const udtString& input, const char* pattern);
	static bool EndsWithNoCase(const udtString& input, const char* pattern);
	static bool EqualsNoCase(const udtString& a, const char* b);

	static bool Contains(u32& charIndex, const udtString& input, const char* pattern);
	static bool Contains(const udtString& input, const char* pattern);
	static bool StartsWith(const udtString& input, const char* pattern);
	static bool EndsWith(const udtString& input, const char* pattern);
	static bool Equals(const udtString& a, const char* b);

	static bool FindFirstCharacterListMatch(u32& index, const udtString& input, const udtString& charList, u32 offset = 0);
	static bool FindLastCharacterListMatch(u32& index, const udtString& input, const udtString& charList);
	static bool FindFirstCharacterMatch(u32& index, const udtString& input, char pattern, u32 offset = 0);
	static bool FindLastCharacterMatch(u32& index, const udtString& input, char pattern);

	static bool IsNullOrEmpty(const udtString& string);
	static bool IsNullOrEmpty(const char* string);

	static bool IsNull(const udtString& string);
	static bool IsEmpty(const udtString& string);

	// Strips Quake color codes and keeps printable codes only depending on the protocol version.
	static void CleanUp(udtString& result, udtProtocol::Id protocol);

	static void RemoveCharacter(udtString& result, char toRemove);

	static void TrimTrailingCharacter(udtString& result, char toRemove);

	static void RemoveEmCharacter(udtString& result);

#if defined(UDT_WINDOWS)
	static udtString NewFromUTF16(udtVMLinearAllocator& allocator, const wchar_t* inputUTF16);
	static wchar_t*  ConvertToUTF16(udtVMLinearAllocator& allocator, const udtString& inputUTF8);
#endif

	const char* GetPtr() const
	{
		if(ReservedBytes == 0)
		{
			return (const char*)Allocator;
		}

		return (const char*)Allocator->GetStartAddress() + Offset;
	}

	const char* GetPtrSafe(const char* ifNull) const
	{
		const char* const string = GetPtr();

		return string != NULL ? string : (ifNull != NULL ? ifNull : "");
	}

	const char* GetPtrOrEmpty() const
	{
		const char* const string = GetPtr();

		return string != NULL ? string : "";
	}

	char* GetWritePtr() const
	{
		if(ReservedBytes == 0)
		{
			return NULL;
		}

		return (char*)Allocator->GetStartAddress() + Offset;
	}

	bool IsValid() const
	{
		return Allocator != NULL && (ReservedBytes == 0 || (u32)Allocator->GetCurrentByteCount() >= Offset + Length);
	}

	bool IsReadOnly() const
	{
		return ReservedBytes == 0;
	}

	u32 GetOffset() const
	{
		return IsNull(*this) ? UDT_U32_MAX : Offset;
	}

	u32 GetLength() const
	{
		return Length;
	}

	void SetLength(u32 length)
	{
		assert(length < ReservedBytes);

		Length = length;
	}

private:
	udtVMLinearAllocator* Allocator; // The string itself when read-only.
	u32 Offset;
	u32 Length;
	u32 ReservedBytes; // When 0, the string is read-only and Allocator points to a string.
};

// Returns the new length.
extern u32 CleanUpString(char* string, udtProtocol::Id protocol);
