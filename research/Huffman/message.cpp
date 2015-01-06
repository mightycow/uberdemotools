#include "message.hpp"
#include "huffman.hpp"
#include "huffman_new.hpp"


// if(s32)f == f and (s32)f + (1<<(FLOAT_INT_BITS-1)) < (1 << FLOAT_INT_BITS)
// the f32 will be sent with FLOAT_INT_BITS, otherwise all 32 bits will be sent
#define	FLOAT_INT_BITS	13
#define	FLOAT_INT_BIAS	(1<<(FLOAT_INT_BITS-1))


#pragma warning(disable: 4996)

static void Q_strncpyz(char *dest, const char *src, s32 destsize)
{
	if(dest == NULL || src == NULL || destsize < 1)
	{
		return;
	}

	strncpy(dest, src, destsize - 1);
	dest[destsize - 1] = 0;
}


static const s32 HuffmanLUT[256] =
{
	250315,			// 0
	41193,			// 1
	6292,			// 2
	7106,			// 3
	3730,			// 4
	3750,			// 5
	6110,			// 6
	23283,			// 7
	33317,			// 8
	6950,			// 9
	7838,			// 10
	9714,			// 11
	9257,			// 12
	17259,			// 13
	3949,			// 14
	1778,			// 15
	8288,			// 16
	1604,			// 17
	1590,			// 18
	1663,			// 19
	1100,			// 20
	1213,			// 21
	1238,			// 22
	1134,			// 23
	1749,			// 24
	1059,			// 25
	1246,			// 26
	1149,			// 27
	1273,			// 28
	4486,			// 29
	2805,			// 30
	3472,			// 31
	21819,			// 32
	1159,			// 33
	1670,			// 34
	1066,			// 35
	1043,			// 36
	1012,			// 37
	1053,			// 38
	1070,			// 39
	1726,			// 40
	888,			// 41
	1180,			// 42
	850,			// 43
	960,			// 44
	780,			// 45
	1752,			// 46
	3296,			// 47
	10630,			// 48
	4514,			// 49
	5881,			// 50
	2685,			// 51
	4650,			// 52
	3837,			// 53
	2093,			// 54
	1867,			// 55
	2584,			// 56
	1949,			// 57
	1972,			// 58
	940,			// 59
	1134,			// 60
	1788,			// 61
	1670,			// 62
	1206,			// 63
	5719,			// 64
	6128,			// 65
	7222,			// 66
	6654,			// 67
	3710,			// 68
	3795,			// 69
	1492,			// 70
	1524,			// 71
	2215,			// 72
	1140,			// 73
	1355,			// 74
	971,			// 75
	2180,			// 76
	1248,			// 77
	1328,			// 78
	1195,			// 79
	1770,			// 80
	1078,			// 81
	1264,			// 82
	1266,			// 83
	1168,			// 84
	965,			// 85
	1155,			// 86
	1186,			// 87
	1347,			// 88
	1228,			// 89
	1529,			// 90
	1600,			// 91
	2617,			// 92
	2048,			// 93
	2546,			// 94
	3275,			// 95
	2410,			// 96
	3585,			// 97
	2504,			// 98
	2800,			// 99
	2675,			// 100
	6146,			// 101
	3663,			// 102
	2840,			// 103
	14253,			// 104
	3164,			// 105
	2221,			// 106
	1687,			// 107
	3208,			// 108
	2739,			// 109
	3512,			// 110
	4796,			// 111
	4091,			// 112
	3515,			// 113
	5288,			// 114
	4016,			// 115
	7937,			// 116
	6031,			// 117
	5360,			// 118
	3924,			// 119
	4892,			// 120
	3743,			// 121
	4566,			// 122
	4807,			// 123
	5852,			// 124
	6400,			// 125
	6225,			// 126
	8291,			// 127
	23243,			// 128
	7838,			// 129
	7073,			// 130
	8935,			// 131
	5437,			// 132
	4483,			// 133
	3641,			// 134
	5256,			// 135
	5312,			// 136
	5328,			// 137
	5370,			// 138
	3492,			// 139
	2458,			// 140
	1694,			// 141
	1821,			// 142
	2121,			// 143
	1916,			// 144
	1149,			// 145
	1516,			// 146
	1367,			// 147
	1236,			// 148
	1029,			// 149
	1258,			// 150
	1104,			// 151
	1245,			// 152
	1006,			// 153
	1149,			// 154
	1025,			// 155
	1241,			// 156
	952,			// 157
	1287,			// 158
	997,			// 159
	1713,			// 160
	1009,			// 161
	1187,			// 162
	879,			// 163
	1099,			// 164
	929,			// 165
	1078,			// 166
	951,			// 167
	1656,			// 168
	930,			// 169
	1153,			// 170
	1030,			// 171
	1262,			// 172
	1062,			// 173
	1214,			// 174
	1060,			// 175
	1621,			// 176
	930,			// 177
	1106,			// 178
	912,			// 179
	1034,			// 180
	892,			// 181
	1158,			// 182
	990,			// 183
	1175,			// 184
	850,			// 185
	1121,			// 186
	903,			// 187
	1087,			// 188
	920,			// 189
	1144,			// 190
	1056,			// 191
	3462,			// 192
	2240,			// 193
	4397,			// 194
	12136,			// 195
	7758,			// 196
	1345,			// 197
	1307,			// 198
	3278,			// 199
	1950,			// 200
	886,			// 201
	1023,			// 202
	1112,			// 203
	1077,			// 204
	1042,			// 205
	1061,			// 206
	1071,			// 207
	1484,			// 208
	1001,			// 209
	1096,			// 210
	915,			// 211
	1052,			// 212
	995,			// 213
	1070,			// 214
	876,			// 215
	1111,			// 216
	851,			// 217
	1059,			// 218
	805,			// 219
	1112,			// 220
	923,			// 221
	1103,			// 222
	817,			// 223
	1899,			// 224
	1872,			// 225
	976,			// 226
	841,			// 227
	1127,			// 228
	956,			// 229
	1159,			// 230
	950,			// 231
	7791,			// 232
	954,			// 233
	1289,			// 234
	933,			// 235
	1127,			// 236
	3207,			// 237
	1020,			// 238
	927,			// 239
	1355,			// 240
	768,			// 241
	1040,			// 242
	745,			// 243
	952,			// 244
	805,			// 245
	1073,			// 246
	740,			// 247
	1013,			// 248
	805,			// 249
	1008,			// 250
	796,			// 251
	996,			// 252
	1057,			// 253
	11457,			// 254
	13504			// 255
};


#define INTERNAL_NODE (HUFF_MAX+1)

static s32 longestCodeLength = 0;
static s32 largestSymbol = 0;

static void PrintHuffmanTree(idHuffmanNode* node, s32 code = 0, s32 codeLength = 0)
{
	if(!node)
	{
		return;
	}

	if(node->symbol == INTERNAL_NODE)
	{
		// Bits are read from LSB to MSB.
		PrintHuffmanTree(node->left, code, codeLength + 1);
		PrintHuffmanTree(node->right, code | (1 << (codeLength)), codeLength + 1);
	}
	else
	{
		//printf("{ %d, %d, %d },\n", code, node->symbol, codeLength);
		/*
		printf("%d, %d, %d, ", code, node->symbol, codeLength);
		PrintBitsMSBFirst((u32)code, (u32)codeLength);
		printf(", ");
		PrintBitsLSBFirst((u32)code, (u32)codeLength);
		printf("\n");
		*/
		longestCodeLength = codeLength > longestCodeLength ? codeLength : longestCodeLength;
		largestSymbol = node->symbol > largestSymbol ? node->symbol : largestSymbol;
	}
}

struct DecodeResult
{
	u8 Symbol;
	u8 BitsRead;
};

static DecodeResult ExpectedDecodeResults[1 << 11];

udtMessage::udtMessage()
{
	_huffman.Init(&_huffmanData);
	for(s32 i = 0; i < 256; ++i)
	{
		for(s32 j = 0; j < HuffmanLUT[i]; ++j)
		{
			_huffman.AddRef(&_huffmanData.compressor, (u8)i);
			_huffman.AddRef(&_huffmanData.decompressor, (u8)i);
		}
	}

	// Generate the expected values using id's decoder.
	memset(ExpectedDecodeResults, 0, sizeof(ExpectedDecodeResults));
	for(u32 i = 0; i < (u32)UDT_COUNT_OF(ExpectedDecodeResults); ++i)
	{
		s32 symbol = 0;
		s32 offset = 0;
		_huffman.OffsetReceive(_huffmanData.decompressor.tree, &symbol, (u8*)&i, &offset);
		ExpectedDecodeResults[i].Symbol = (u8)symbol;
		ExpectedDecodeResults[i].BitsRead = (u8)offset;
	}

	// Test the new decoder.
	_huffmanDecoderNew.Init();
	u32 decoderErrorCount = 0;
	for(u32 i = 0; i < (u32)UDT_COUNT_OF(ExpectedDecodeResults); ++i)
	{
		u32 symbol = 0;
		u32 bitsRead = 0;
		_huffmanDecoderNew.ReadSymbol(symbol, bitsRead, i);
		if(ExpectedDecodeResults[i].Symbol != (u8)symbol ||
		   ExpectedDecodeResults[i].BitsRead != (u8)bitsRead)
		{
			++decoderErrorCount;
			printf("Decoder error: got %d,%d instead of %d,%d for code %d\n", 
				   (int)symbol, (int)bitsRead, (int)ExpectedDecodeResults[i].Symbol, (int)ExpectedDecodeResults[i].BitsRead, (int)i);
		}
	}
	printf("New decoder error count: %u\n", decoderErrorCount);
}

void udtMessage::PrintDecoderTree()
{
	PrintHuffmanTree(_huffmanData.decompressor.tree);
	printf("Longest code length: %d\n", (int)longestCodeLength);
	printf("Largest symbol: %d\n", (int)largestSymbol);
}

void udtMessage::Init(u8* data, s32 length) 
{
	Com_Memset(&Buffer, 0, sizeof(idMessage));
	Buffer.data = data;
	Buffer.maxsize = length;
}

void udtMessage::InitOOB(u8* data, s32 length) 
{
	Com_Memset(&Buffer, 0, sizeof(idMessage));
	Buffer.data = data;
	Buffer.maxsize = length;
	Buffer.oob = qtrue;
}

void udtMessage::Clear() 
{
	Buffer.cursize = 0;
	Buffer.overflowed = qfalse;
	Buffer.bit = 0;
}

void udtMessage::Bitstream() 
{
	Buffer.oob = qfalse;
}

void udtMessage::BeginReading() 
{
	Buffer.readcount = 0;
	Buffer.bit = 0;
	Buffer.oob = qfalse;
}

void udtMessage::BeginReadingOOB()
{
	Buffer.readcount = 0;
	Buffer.bit = 0;
	Buffer.oob = qtrue;
}

void udtMessage::Copy(u8* data, s32 length, const udtMessage* src)
{
	if(length < src->Buffer.cursize)
	{
		printf("idMessage::Copy: can't copy s32o a smaller msg_t buffer");
	}

	Com_Memcpy(&Buffer, &src->Buffer, sizeof(idMessage));
	Com_Memcpy(data, src->Buffer.data, src->Buffer.cursize);
	Buffer.data = data;
}

/*
=============================================================================

bit functions

=============================================================================
*/

// negative bit values include signs
void udtMessage::WriteBits(s32 value, s32 bits) 
{
	// this isn't an exact overflow check, but close enough
	if(Buffer.maxsize - Buffer.cursize < 4) 
	{
		Buffer.overflowed = qtrue;
		return;
	}

	if(bits == 0 || bits < -31 || bits > 32) 
	{
		printf("idMessage::WriteBits: bad bits %i", bits);
	}

	if(bits < 0) 
	{
		bits = -bits;
	}

	if(Buffer.oob) 
	{
		if(bits == 8)
		{
			Buffer.data[Buffer.cursize] = (u8)value;
			Buffer.cursize += 1;
			Buffer.bit += 8;
		} 
		else if(bits == 16) 
		{
			unsigned short* sp = (unsigned short*)&Buffer.data[Buffer.cursize];
			*sp = (unsigned short)LittleShort(value);
			Buffer.cursize += 2;
			Buffer.bit += 16;
		} 
		else if(bits == 32) 
		{
			u32* ip = (u32*)&Buffer.data[Buffer.cursize];
			*ip = LittleLong(value);
			Buffer.cursize += 4;
			Buffer.bit += 32;
		} 
		else 
		{
			printf("idMessage::WriteBits: Can't write %d bits\n", bits);
		}
	} 
	else 
	{
		value &= (0xffffffff >> (32-bits));

		if(bits & 7) 
		{
			s32 nbits = bits&7;
			for(s32 i = 0; i < nbits; ++i) 
			{
				_huffman.PutBit(value & 1, Buffer.data, &Buffer.bit);
				value = value >> 1;
			}
			bits = bits - nbits;
		}

		if(bits) 
		{
			for(s32 i = 0; i < bits; i += 8) 
			{
				_huffman.OffsetTransmit(&_huffmanData.compressor, (value & 0xff), Buffer.data, &Buffer.bit);
				value = value >> 8;
			}
		}

		Buffer.cursize = (Buffer.bit >> 3) + 1;
	}
}

s32 udtMessage::ReadBits(s32 bits) 
{
	s32	nbits;

	qbool sgn;
	if(bits < 0) 
	{
		bits = -bits;
		sgn = qtrue;
	}
	else
	{
		sgn = qfalse;
	}

	s32 value = 0;
	if(Buffer.oob) 
	{
		if(bits == 8) 
		{
			value = Buffer.data[Buffer.readcount];
			Buffer.readcount += 1;
			Buffer.bit += 8;
		} 
		else if(bits == 16) 
		{
			unsigned short* sp = (unsigned short*)&Buffer.data[Buffer.readcount];
			value = LittleShort(*sp);
			Buffer.readcount += 2;
			Buffer.bit += 16;
		} 
		else if(bits == 32) 
		{
			u32* ip = (u32*)&Buffer.data[Buffer.readcount];
			value = LittleLong(*ip);
			Buffer.readcount += 4;
			Buffer.bit += 32;
		} 
		else 
		{
			printf("idMessage::ReadBits: Can't read %d bits\n", bits);
		}
	} 
	else
	{
		nbits = 0;

		if(bits & 7) 
		{
			nbits = bits & 7;
			for(s32 i = 0; i < nbits; ++i) 
			{
				value |= (_huffman.GetBit(Buffer.data, &Buffer.bit) << i);
			}
			bits = bits - nbits;
		}

		if(bits)
		{
			for(s32 i = 0; i < bits; i += 8) 
			{
				s32	get;
				_huffman.OffsetReceive(_huffmanData.decompressor.tree, &get, Buffer.data, &Buffer.bit);
				value |= (get << (i+nbits));
			}
		}

		Buffer.readcount = (Buffer.bit>>3)+1;
	}

	if(sgn) 
	{
		if(value & (1 << (bits - 1)))
		{
			value |= -1 ^ ((1 << bits) - 1);
		}
	}

	return value;
}

//
// writing functions
//

void udtMessage::WriteByte(s32 c) 
{
	WriteBits(c, 8);
}

void udtMessage::WriteData(const void* data, s32 length) 
{
	for(s32 i = 0; i < length; ++i) 
	{
		WriteByte(((u8*)data)[i]);
	}
}

void udtMessage::WriteShort(s32 c) 
{
	WriteBits(c, 16);
}

void udtMessage::WriteLong(s32 c) 
{
	WriteBits(c, 32);
}

void udtMessage::WriteString(const char* s, s32 length) 
{
	if(!s) 
	{
		WriteData("", 1);
		return;
	} 
	
	if(length >= MAX_STRING_CHARS)
	{
		printf("idMessage::WriteString: The string's length is >= MAX_STRING_CHARS");
		return;
	}

	char string[MAX_STRING_CHARS];
	Q_strncpyz(string, s, sizeof(string));

	// get rid of 0xff s8s, because old clients don't like them
	for(s32 i = 0 ; i < length; ++i) 
	{
		if(((u8*)string)[i] > 127) 
		{
			string[i] = '.';
		}
	}

	WriteData(string, length + 1);
}

void udtMessage::WriteBigString(const char* s, s32 length) 
{
	if(!s) 
	{
		WriteData("", 1);
	}

	if(length >= BIG_INFO_STRING) 
	{
		printf("idMessage::WriteBigString: The string's length is >= BIG_INFO_STRING");
		return;
	}

	char string[BIG_INFO_STRING];
	Q_strncpyz(string, s, sizeof(string));

	// get rid of 0xff s8s, because old clients don't like them
	for(s32 i = 0; i < length; ++i) 
	{
		if(((u8*)string)[i] > 127) 
		{
			string[i] = '.';
		}
	}

	WriteData(string, length + 1);
}

s32 udtMessage::ReadByte()
{
	s32 c = (u8)ReadBits(8);
	if(Buffer.readcount > Buffer.cursize) 
	{
		c = -1;
	}

	return c;
}

s32 udtMessage::ReadShort()
{
	s32 c = (short)ReadBits(16);
	if(Buffer.readcount > Buffer.cursize) 
	{
		c = -1;
	}

	return c;
}

s32 udtMessage::ReadLong()
{
	s32 c = ReadBits(32);
	if(Buffer.readcount > Buffer.cursize) 
	{
		c = -1;
	}

	return c;
}

char* udtMessage::ReadString(s32& length) 
{
	static char stringBuffer[BIG_INFO_STRING];

	s32 stringLength = 0;
	do 
	{
		// use ReadByte so -1 is out of bounds
		s32 c = ReadByte(); 
		if(c == -1 || c == 0) 
		{
			break;
		}

		// translate all fmt spec to avoid crash bugs
		if(c == '%')
		{
			c = '.';
		}

		// don't allow higher ascii values
		if(c > 127) 
		{
			c = '.';
		}

		stringBuffer[stringLength] = (s8)c;
		stringLength++;
	} 
	while(stringLength < (s32)sizeof(stringBuffer) - 1);

	stringBuffer[stringLength] = 0;
	length = stringLength;

	return stringBuffer;
}

char* udtMessage::ReadBigString(s32& length)
{
	static char stringBuffer[BIG_INFO_STRING];

	s32 stringLength = 0;
	do 
	{
		// use ReadByte so -1 is out of bounds
		s32 c = ReadByte(); 
		if(c == -1 || c == 0)
		{
			break;
		}

		// translate all fmt spec to avoid crash bugs
		if(c == '%') 
		{
			c = '.';
		}

		// don't allow higher ascii values
		if(c > 127) 
		{
			c = '.';
		}

		stringBuffer[stringLength] = (s8)c;
		stringLength++;
	} 
	while(stringLength < (s32)sizeof(stringBuffer) - 1);

	stringBuffer[stringLength] = 0;
	length = stringLength;

	return stringBuffer;
}

char* udtMessage::ReadStringLine(s32& length) 
{
	static char stringBuffer[BIG_INFO_STRING];

	s32 stringLength = 0;
	do 
	{
		// use ReadByte so -1 is out of bounds
		s32 c = ReadByte();
		if(c == -1 || c == 0 || c == '\n') 
		{
			break;
		}

		// translate all fmt spec to avoid crash bugs
		if(c == '%') 
		{
			c = '.';
		}

		// don't allow higher ascii values
		if(c > 127) 
		{
			c = '.';
		}

		stringBuffer[stringLength] = (s8)c;
		stringLength++;
	} 
	while(stringLength < (s32)sizeof(stringBuffer) - 1);
	
	stringBuffer[stringLength] = 0;
	length = stringLength;
	
	return stringBuffer;
}

void udtMessage::ReadData(void* data, s32 len) 
{
	for(s32 i = 0; i < len; ++i) 
	{
		((u8*)data)[i] = (u8)ReadByte();
	}
}

s32 udtMessage::PeekByte()
{
	const s32 bloc = _huffman.GetBloc();
	const s32 readcount = Buffer.readcount;
	const s32 bit = Buffer.bit;

	// Valgrind: don't return an uninitialized value.
	if(Buffer.cursize <= Buffer.readcount)
	{
		return 0;
	}

	const s32 c = ReadByte();

	_huffman.SetBloc(bloc);
	Buffer.readcount = readcount;
	Buffer.bit = bit;

	return c;
}
