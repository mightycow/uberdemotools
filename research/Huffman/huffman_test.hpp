#pragma once


#include "common.hpp"

#include <algorithm>
#include <vector>
#include <assert.h>


struct HuffmanTableEntry
{
	u16 Code;
	u8 Symbol;
	u8 CodeLength;
};

static HuffmanTableEntry GlobalHuffmanLUT[] =
{
	// @TODO: Does code=256 with length=11 need to be handled at all?
	//{ 256, 256, 11 }, // @NOTE: This is handled as a special case.
	{ 0, 208, 9 },
	{ 1280, 247, 11 },
	{ 768, 243, 10 },
	{ 128, 70, 9 },
	{ 384, 146, 9 },
	{ 64, 117, 7 },
	{ 32, 195, 6 },
	{ 16, 6, 7 },
	{ 80, 65, 7 },
	{ 48, 101, 7 },
	{ 112, 71, 9 },
	{ 368, 90, 9 },
	{ 240, 241, 10 },
	{ 752, 45, 10 },
	{ 496, 18, 9 },
	{ 8, 126, 7 },
	{ 72, 2, 7 },
	{ 40, 105, 8 },
	{ 168, 91, 9 },
	{ 424, 251, 10 },
	{ 936, 245, 10 },
	{ 104, 125, 7 },
	{ 24, 237, 8 },
	{ 152, 108, 8 },
	{ 88, 17, 9 },
	{ 344, 219, 10 },
	{ 856, 249, 10 },
	{ 216, 95, 8 },
	{ 56, 176, 9 },
	{ 312, 168, 9 },
	{ 184, 199, 8 },
	{ 120, 47, 8 },
	{ 248, 223, 10 },
	{ 760, 227, 10 },
	{ 504, 19, 9 },
	{ 4, 67, 7 },
	{ 68, 34, 9 },
	{ 324, 62, 9 },
	{ 196, 107, 9 },
	{ 452, 141, 9 },
	{ 36, 255, 6 },
	{ 20, 43, 10 },
	{ 532, 185, 10 },
	{ 276, 160, 9 },
	{ 148, 40, 9 },
	{ 404, 217, 10 },
	{ 916, 215, 10 },
	{ 84, 192, 8 },
	{ 212, 31, 8 },
	{ 52, 9, 7 },
	{ 116, 139, 8 },
	{ 244, 24, 9 },
	{ 500, 46, 9 },
	{ 12, 110, 8 },
	{ 140, 113, 8 },
	{ 76, 130, 7 },
	{ 44, 163, 10 },
	{ 556, 201, 10 },
	{ 300, 80, 9 },
	{ 172, 15, 9 },
	{ 428, 41, 10 },
	{ 940, 181, 10 },
	{ 108, 3, 7 },
	{ 28, 104, 6 },
	{ 60, 97, 8 },
	{ 188, 61, 9 },
	{ 444, 187, 10 },
	{ 956, 179, 10 },
	{ 124, 66, 7 },
	{ 2, 0, 2 },
	{ 1, 134, 8 },
	{ 129, 142, 9 },
	{ 385, 211, 10 },
	{ 897, 189, 10 },
	{ 65, 102, 8 },
	{ 193, 221, 10 },
	{ 705, 239, 10 },
	{ 449, 165, 10 },
	{ 961, 177, 10 },
	{ 33, 68, 8 },
	{ 161, 4, 8 },
	{ 97, 169, 10 },
	{ 609, 235, 10 },
	{ 353, 55, 9 },
	{ 225, 121, 8 },
	{ 17, 5, 8 },
	{ 145, 225, 9 },
	{ 401, 59, 10 },
	{ 913, 231, 10 },
	{ 81, 69, 8 },
	{ 209, 224, 9 },
	{ 465, 167, 10 },
	{ 977, 244, 10 },
	{ 49, 157, 10 },
	{ 561, 233, 10 },
	{ 305, 144, 9 },
	{ 177, 53, 8 },
	{ 113, 229, 10 },
	{ 625, 44, 10 },
	{ 369, 85, 10 },
	{ 881, 75, 10 },
	{ 241, 57, 9 },
	{ 497, 200, 9 },
	{ 9, 196, 7 },
	{ 73, 232, 7 },
	{ 41, 129, 7 },
	{ 105, 10, 7 },
	{ 25, 119, 8 },
	{ 153, 226, 10 },
	{ 665, 183, 10 },
	{ 409, 58, 9 },
	{ 89, 116, 7 },
	{ 57, 14, 8 },
	{ 185, 213, 10 },
	{ 697, 252, 10 },
	{ 441, 159, 10 },
	{ 953, 209, 10 },
	{ 121, 115, 8 },
	{ 249, 153, 10 },
	{ 761, 250, 10 },
	{ 505, 161, 10 },
	{ 1017, 37, 10 },
	{ 5, 248, 10 },
	{ 517, 238, 10 },
	{ 261, 93, 9 },
	{ 133, 112, 8 },
	{ 69, 202, 10 },
	{ 581, 155, 10 },
	{ 325, 149, 10 },
	{ 837, 171, 10 },
	{ 197, 180, 10 },
	{ 709, 242, 10 },
	{ 453, 205, 10 },
	{ 965, 36, 10 },
	{ 37, 16, 7 },
	{ 101, 127, 7 },
	{ 21, 8, 5 },
	{ 13, 54, 9 },
	{ 269, 212, 10 },
	{ 781, 38, 10 },
	{ 141, 191, 10 },
	{ 653, 253, 10 },
	{ 397, 25, 10 },
	{ 909, 218, 10 },
	{ 77, 143, 9 },
	{ 333, 175, 10 },
	{ 845, 206, 10 },
	{ 205, 173, 10 },
	{ 717, 35, 10 },
	{ 461, 214, 10 },
	{ 973, 39, 10 },
	{ 45, 13, 6 },
	{ 29, 207, 10 },
	{ 541, 246, 10 },
	{ 285, 204, 10 },
	{ 797, 166, 10 },
	{ 157, 81, 10 },
	{ 669, 188, 10 },
	{ 413, 76, 9 },
	{ 93, 194, 8 },
	{ 221, 210, 10 },
	{ 733, 164, 10 },
	{ 477, 20, 10 },
	{ 989, 222, 10 },
	{ 61, 151, 10 },
	{ 573, 178, 10 },
	{ 317, 72, 9 },
	{ 189, 106, 9 },
	{ 445, 216, 10 },
	{ 957, 220, 10 },
	{ 125, 131, 7 },
	{ 3, 203, 10 },
	{ 515, 186, 10 },
	{ 259, 193, 9 },
	{ 131, 133, 8 },
	{ 67, 29, 8 },
	{ 195, 49, 8 },
	{ 35, 228, 10 },
	{ 547, 236, 10 },
	{ 291, 60, 10 },
	{ 803, 23, 10 },
	{ 163, 122, 8 },
	{ 99, 73, 10 },
	{ 611, 190, 10 },
	{ 355, 145, 10 },
	{ 867, 27, 10 },
	{ 227, 154, 10 },
	{ 739, 170, 10 },
	{ 483, 86, 10 },
	{ 995, 182, 10 },
	{ 19, 12, 7 },
	{ 83, 52, 8 },
	{ 211, 33, 10 },
	{ 723, 230, 10 },
	{ 467, 84, 10 },
	{ 979, 184, 10 },
	{ 51, 42, 10 },
	{ 563, 87, 10 },
	{ 307, 162, 10 },
	{ 819, 79, 10 },
	{ 179, 111, 8 },
	{ 115, 123, 8 },
	{ 243, 96, 9 },
	{ 499, 63, 10 },
	{ 1011, 21, 10 },
	{ 11, 11, 7 },
	{ 75, 120, 8 },
	{ 203, 174, 10 },
	{ 715, 89, 10 },
	{ 459, 140, 9 },
	{ 43, 148, 10 },
	{ 555, 22, 10 },
	{ 299, 156, 10 },
	{ 811, 152, 10 },
	{ 171, 26, 10 },
	{ 683, 77, 10 },
	{ 427, 98, 9 },
	{ 107, 150, 10 },
	{ 619, 172, 10 },
	{ 363, 82, 10 },
	{ 875, 83, 10 },
	{ 235, 94, 9 },
	{ 491, 28, 10 },
	{ 1003, 158, 10 },
	{ 27, 1, 5 },
	{ 7, 56, 9 },
	{ 263, 234, 10 },
	{ 775, 198, 10 },
	{ 135, 135, 8 },
	{ 71, 114, 8 },
	{ 199, 92, 9 },
	{ 455, 78, 10 },
	{ 967, 197, 10 },
	{ 39, 48, 7 },
	{ 103, 136, 8 },
	{ 231, 137, 8 },
	{ 23, 118, 8 },
	{ 151, 100, 9 },
	{ 407, 51, 9 },
	{ 87, 138, 8 },
	{ 215, 88, 10 },
	{ 727, 74, 10 },
	{ 471, 240, 10 },
	{ 983, 147, 10 },
	{ 55, 32, 6 },
	{ 15, 132, 8 },
	{ 143, 109, 9 },
	{ 399, 99, 9 },
	{ 79, 30, 9 },
	{ 335, 103, 9 },
	{ 207, 64, 8 },
	{ 47, 254, 7 },
	{ 111, 124, 8 },
	{ 239, 50, 8 },
	{ 31, 128, 6 },
	{ 63, 7, 6 }
}; // HuffmanTableEntry GlobalHuffmanLUT[]

// Longest code length : 11
// Largest symbol : 256


struct udtNewHuffmanDecoder
{
	// This is not optimal but it's only used for table generation...
	void Write12Bits(u8* firstElement, u32 elementIndex, u16 elementValue)
	{
		// even index ==> keep last  4 bits
		// odd  index ==> keep first 4 bits
		const u32 byteIndex = elementIndex + (elementIndex >> 1);
		const u16 oldShortValue = *(u16*)(firstElement + byteIndex);
		const u32 bitsToShift = (elementIndex & 1) << 2;
		const u32 newValueShifted = (elementValue & 0xFFF) << bitsToShift;
		const u32 oldValueMask = (elementIndex & 1) ? 0xF : (0xF000);
		const u32 oldValueMasked = oldShortValue & oldValueMask;
		const u16 newShortValue = newValueShifted | oldValueMasked;
		*(u16*)(firstElement + byteIndex) = newShortValue;
	}

	// @TODO: Should be optimized/inlined for the final implementation.
	u16 Read12Bits(const u8* firstElement, u32 elementIndex)
	{
		// Example: index 3 == bit index 36 ==> read bytes 4+5
		const u32 byteIndex = elementIndex + (elementIndex >> 1);
		const u32 bitsToShift = (elementIndex & 1) << 2;
		const u16 shortValue = *(u16*)(firstElement + byteIndex);
		return (shortValue >> bitsToShift) & 0xFFF;
	}

	void Init()
	{
		memset(this, 0, sizeof(*this));

		SortTable();

		// Generate first table.
		for(u32 i = 0; i < (u32)UDT_COUNT_OF(GlobalHuffmanLUT); ++i)
		{
			const u32 codeLength = (u32)GlobalHuffmanLUT[i].CodeLength;
			const u32 combinations = 1 << (11 - codeLength);
			const u16 codeBase = GlobalHuffmanLUT[i].Code;
			for(u32 j = 0; j < combinations; ++j)
			{
				const u32 code = (u32)codeBase | (j << codeLength);
				_decoderTable[code] = u16(GlobalHuffmanLUT[i].Symbol) | u16(codeLength << 8);
			}
		}
		_decoderTable[256] = 11 << 8;

		// Generate first table in compact mode.
		for(u32 i = 0; i < 2048; ++i)
		{
			const u16 entry = _decoderTable[i];
			Write12Bits(_decoderTableCompact, i, entry);
		}

		for(u32 i = 0; i < (u32)UDT_COUNT_OF(GlobalHuffmanLUT); ++i)
		{
			const u16 code = GlobalHuffmanLUT[i].Code;
			const u8 symbol = GlobalHuffmanLUT[i].Symbol;
			const u8 codeLength = GlobalHuffmanLUT[i].CodeLength;
			_encoderTable[symbol] = codeLength | (code << 4);
		}

		PrintFinalTables();
	}

	static bool CompareByCodeLength(const HuffmanTableEntry& a, const HuffmanTableEntry& b)
	{
		return a.CodeLength < b.CodeLength;
	}

	static bool CompareByCodeValue(const HuffmanTableEntry& a, const HuffmanTableEntry& b)
	{
		return a.Code < b.Code;
	}

	void SortTable()
	{
		HuffmanTableEntry* const start = GlobalHuffmanLUT;
		HuffmanTableEntry* const end = GlobalHuffmanLUT + UDT_COUNT_OF(GlobalHuffmanLUT);
		std::stable_sort(start, end, &CompareByCodeValue);
		std::stable_sort(start, end, &CompareByCodeLength);
	}

	void PrintSortedTable()
	{
		SortTable();

		for(u32 i = 0; i < (u32)UDT_COUNT_OF(GlobalHuffmanLUT); ++i)
		{
			const HuffmanTableEntry entry = GlobalHuffmanLUT[i];

			printf("%d, %d, %d, ", (int)entry.CodeLength, (int)entry.Code, (int)entry.Symbol);
			PrintBitsLSBFirst((u32)entry.Code, (u32)entry.CodeLength);
			printf(", ");
			PrintBitsMSBFirst((u32)entry.Code, (u32)entry.CodeLength);
			printf("\n");
		}
	}

	void PrintCodesOfLength(u8 length)
	{
		//printf("Codes of length %u:\n", length);
		for(u32 i = 0; i < (u32)UDT_COUNT_OF(GlobalHuffmanLUT); ++i)
		{
			if(GlobalHuffmanLUT[i].CodeLength == length)
			{
				PrintBitsLSBFirst((u32)GlobalHuffmanLUT[i].Code, (u32)GlobalHuffmanLUT[i].CodeLength);
				printf(" == %d", GlobalHuffmanLUT[i].Code);
				printf("\n");
			}
		}
	}
	
	void PrintFinalTables()
	{
		PrintTable("DecoderTable", _decoderTable, (u32)UDT_COUNT_OF(_decoderTable), 16);
		PrintTable("DecoderTableCompact", _decoderTableCompact, (u32)UDT_COUNT_OF(_decoderTableCompact), 12);
		PrintTable("EncoderTable", _encoderTable, (u32)UDT_COUNT_OF(_encoderTable), 8);
	}

	template<typename T>
	const char* GetTypeName()
	{
		return "u8";
	}

	template<>
	const char* GetTypeName<u16>()
	{
		return "u16";
	}

	template<typename T>
	void PrintTable(const char* name, const T* data, u32 elementCount, u32 elementsPerRow)
	{
		printf("static const %s %s[%u] =\n", GetTypeName<T>(), name, elementCount);
		printf("{\n");
		for(u32 i = 0; i < elementCount; i += elementsPerRow)
		{ 
			printf("\t");
			const u32 elementsInThisRow = (i + elementsPerRow <= elementCount) ? elementsPerRow : (elementCount % elementsPerRow);
			for(u32 j = 0; j < elementsInThisRow; ++j)
			{
				printf("%d, ", (int)data[i + j]);
			}
			printf("\n");
		}
		printf("};\n");
	}

	void ReadSymbolNormal(u32& symbol, u32& bitsRead, u32 look)
	{
		const u16 entry = _decoderTable[look & 0x7FF];
		symbol = u32(entry & 0xFF);
		bitsRead += u32(entry >> 8);
	}

	void ReadSymbolCompact(u32& symbol, u32& bitsRead, u32 look)
	{
		const u16 entry = Read12Bits(_decoderTableCompact, look & 0x7FF);
		symbol = u32(entry & 0xFF);
		bitsRead += u32(entry >> 8);
	}

	void ReadSymbol(u32& symbol, u32& bitsRead, u32 look)
	{
		//ReadSymbolNormal(symbol, bitsRead, look);
		ReadSymbolCompact(symbol, bitsRead, look);
	}

	//
	// id interface
	//

	u32 GetBits(u32 bitIndex, const u8* fin)
	{
		return *(u32*)(fin + (bitIndex >> 3)) >> (bitIndex & 7);
	}

	s32 GetBit(s32 bitIndex, const u8* fin)
	{
		return (fin[(bitIndex >> 3)] >> (bitIndex & 7)) & 1;
	}

	void OffsetReceive(s32 *ch, u8 *fin, s32 *offset)
	{
		const u32 input = GetBits(*(u32*)offset, fin);

		u32 bitsRead = 0;
		ReadSymbol(*(u32*)ch, bitsRead, input);

		*offset += (s32)bitsRead;
	}

	void PutBit(u8* fout, s32 bitIndex, s32 bit)
	{
		if((bitIndex & 7) == 0)
		{
			fout[(bitIndex >> 3)] = 0;
		}

		fout[(bitIndex >> 3)] |= bit << (bitIndex & 7);
	}

	void PutBits(u8* fout, u32 bitIndex, u32 bits, u32 bitCount)
	{
		for(u32 i = 0; i < bitCount; ++i)
		{
			PutBit(fout, bitIndex + i, (s32)(bits & 1));
			bits >>= 1;
		}
	}

	void OffsetTransmit(u8 *fout, s32 *offset, s32 ch)
	{
		const u16 result = _encoderTable[ch];
		const u16 bitCount = result & 15;
		const u16 code = (result >> 4) & 0x7FF;

		PutBits(fout, *(u32*)offset, code, bitCount);

		*offset += (s32)bitCount;
	}

	// v3 decoder
	u16 _decoderTable[2048];

	// v4 decoder
	u8 _decoderTableCompact[3072];

	// v1 encoder
	// Bits 0- 3: bits written
	// Bits 4-14: code word
	u16 _encoderTable[256];
};
