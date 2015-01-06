#pragma once


#include "macros.hpp"


#if defined(UDT_MSVC)

	typedef __int8   s8;
	typedef __int16 s16;
	typedef __int32 s32;
	typedef __int64 s64;
	typedef unsigned __int8   u8;
	typedef unsigned __int16 u16;
	typedef unsigned __int32 u32;
	typedef unsigned __int64 u64;

#elif defined(UDT_GCC)

#	include <stdint.h>

	typedef int8_t    s8;
	typedef int16_t  s16;
	typedef int32_t  s32;
	typedef int64_t  s64;
	typedef uint8_t   u8;
	typedef uint16_t u16;
	typedef uint32_t u32;
	typedef uint64_t u64;

#else

#error Sorry, your compiler is not supported.

#endif

#if defined(UDT_X86)
typedef s32 sptr;
typedef u32 uptr;
#else
typedef s64 sptr;
typedef u64 uptr;
#endif

typedef float  f32;
typedef double f64;

#define S32_MIN     (-2147483647 - 1)
#define S32_MAX     (2147483647)
#define U32_MAX     (0xFFFFFFFF)
