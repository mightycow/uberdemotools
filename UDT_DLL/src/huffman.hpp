#pragma once


#include "uberdemotools.h"
#include "macros.hpp"


namespace udtHuffman
{
	UDT_FORCE_INLINE void PutBit(u8* fout, s32 bitIndex, s32 bit)
	{
		if((bitIndex & 7) == 0)
		{
			fout[(bitIndex >> 3)] = 0;
		}

		fout[(bitIndex >> 3)] |= bit << (bitIndex & 7);
	}

	// Get the bit at bitIndex in the LSB of the result and have all other bits be 0.
	UDT_FORCE_INLINE s32 GetBit(s32 bitIndex, const u8* fin)
	{
		return (fin[(bitIndex >> 3)] >> (bitIndex & 7)) & 1;
	}

	extern void OffsetReceive(s32* ch, const u8* fin, s32* offset);
	extern void OffsetTransmit(u8 *fout, s32 *offset, s32 ch);
}
