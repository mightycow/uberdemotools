#include "context.hpp"
#include "common.hpp"

#if defined(_MSC_VER) && defined(_WIN32) && defined(_DEBUG)
#	include <Windows.h> // IsDebuggerPresent, __debugbreak
#endif

#if defined(UDT_MSVC)
#	include <stdio.h>
#	include <stdarg.h>
#endif

#if defined(UDT_GCC)
#	include <stdarg.h>
#endif

#include <stdlib.h> // For exit.


extern const s32 HuffmanLUT[256];


static void DefaultCrashCallback(const char* message)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Fatal error: %s\n", message);
	exit(666);
}


udtContext::udtContext()
	: _messageCallback(NULL)
	, _progressCallback(NULL)
	, _crashCallback(&DefaultCrashCallback)
	, _huffmanInitialized(false)
{
	TempAllocator.Init(1 << 24, 4096);
}

udtContext::~udtContext()
{
	Destroy();
}

bool udtContext::SetCallbacks(udtMessageCallback messageCb, udtProgressCallback progressCb, void* progressContext)
{
	_messageCallback = messageCb;
	_progressCallback = progressCb;
	_progressContext = progressContext;

	return true;
}

void udtContext::SetCrashCallback(udtCrashCallback crashCb) 
{
	if(crashCb != NULL)
	{
		_crashCallback = crashCb;
	}
}

void udtContext::SafeInitHuffman()
{
	if(_huffmanInitialized)
	{
		return;
	}

	_huffmanInitialized = true;
	Huffman.Init(&HuffmanData);
	for(s32 i = 0; i < 256; ++i) 
	{
		for(s32 j = 0; j < HuffmanLUT[i]; ++j) 
		{
			Huffman.AddRef(&HuffmanData.compressor, (u8)i);
			Huffman.AddRef(&HuffmanData.decompressor, (u8)i);
		}
	}
}

void udtContext::Reset()
{
	TempAllocator.Clear();
}

void udtContext::Destroy()
{
}

void udtContext::LogInfo(UDT_PRINTF_FORMAT_ARG const char* format, ...) const
{
	if(!_messageCallback)
	{
		return;
	}

	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg) - 1, format, argptr);
	va_end(argptr);

	(*_messageCallback)(0, msg);
}

void udtContext::LogWarning(UDT_PRINTF_FORMAT_ARG const char* format, ...) const
{
	if(!_messageCallback)
	{
		return;
	}

	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg) - 1, format, argptr);
	va_end(argptr);

	(*_messageCallback)(1, msg);
}

void udtContext::LogError(UDT_PRINTF_FORMAT_ARG const char* format, ...) const
{
	if(!_messageCallback)
	{
		return;
	}

	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg) - 1, format, argptr);
	va_end(argptr);

	(*_messageCallback)(2, msg);
}

void udtContext::LogErrorAndCrash(UDT_PRINTF_FORMAT_ARG const char* format, ...) const
{
	if(!_messageCallback)
	{
		return;
	}

	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg) - 1, format, argptr);
	va_end(argptr);

	(*_messageCallback)(3, msg);

#if defined(_MSC_VER) && defined(_WIN32) && defined(_DEBUG)
	if(IsDebuggerPresent()) __debugbreak();
#endif

	(*_crashCallback)(msg);
}

bool udtContext::NotifyProgress(f32 progress) const
{
	if(_progressCallback != NULL)
	{
		return (*_progressCallback)(progress, _progressContext) != 0;
	}

	return false;
}

const s32 HuffmanLUT[256] = 
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