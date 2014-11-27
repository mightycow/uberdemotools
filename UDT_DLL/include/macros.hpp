#pragma once


/*
Compiler macros
*/
#if defined(_MSC_VER)
#	define UDT_MSVC
#	define UDT_MSVC_VER _MSC_VER
#endif

#if defined(__GNUC__)
#	define UDT_GCC
#	define UDT_GCC_VER __GNUC__
#endif

#if defined(__ICC) || defined(__INTEL_COMPILER)
#	define UDT_ICC
#endif

#if defined(__BORLANDC__) || defined(__BCPLUSPLUS__)
#	define UDT_BORLAND
#endif

#if defined(__MINGW32__)
#	define UDT_MINGWIN
#endif

#if defined(__CYGWIN32__)
#	define UDT_CYGWIN
#endif

/*
Hardware target macros
*/
#if defined(_M_IX86) || defined(__i386__)
#	define UDT_X86
#endif

#if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64)
#	define UDT_X64
#endif

#if defined(UDT_X86) || defined(UDT_X64)
#	define UDT_ARCH_X86
#endif

/*
Software target macros
*/
#if defined(_WIN32)
#	define UDT_WIN32
#endif

#if defined(_WIN64)
#	define UDT_WIN64
#endif

#if defined(UDT_WIN32) || defined(UDT_WIN64)
#	define UDT_WIN
#	define UDT_WINDOWS
#else
#	define UDT_LIN
#	define UDT_LINUX
#endif

/*
Qualifier macros
*/
#if defined(UDT_MSVC) || defined(UDT_ICC_WIN)
#	define UDT_INLINE       __inline
#	define UDT_FORCE_INLINE __forceinline
#elif defined(UDT_GCC) || defined(UDT_ICC_LIN)
#	define UDT_INLINE       __inline__
#	define UDT_FORCE_INLINE __inline__
#else
#	define UDT_INLINE
#	define UDT_FORCE_INLINE
#endif

#if defined(UDT_MSVC) || defined(UDT_ICC_WIN)
#	define UDT_RESTRICT      __restrict
#	define UDT_RESTRICT_THIS __restrict
#elif defined(UDT_GCC) || defined(UDT_ICC_LIN)
#	define UDT_RESTRICT      __restrict__
#	define UDT_RESTRICT_THIS __restrict__
#else
#	define UDT_RESTRICT
#	define UDT_RESTRICT_THIS
#endif

#if defined(UDT_MSVC) || defined(UDT_ICC_WIN)
#	define UDT_PACK		pack(push, 1)
#	define UDT_UNPACK	pack(pop)
#elif defined(UDT_GCC) || defined(UDT_ICC_LIN) // @FIXME: Not sure if ICC Linux supports this?
#	define UDT_PACK		_Pragma("pack(push, 1)")
#	define UDT_UNPACK	_Pragma("pack(pop)")
#else
#	define UDT_PACK		
#	define UDT_UNPACK	
#endif

// Static branch prediction helper.
#if (UDT_GCC_VER >= 3) || defined(UDT_ICC)
#	define UDT_LIKELY(cond)    __builtin_expect(!!(cond), 1) 
#	define UDT_UNLIKELY(cond)  __builtin_expect(!!(cond), 0) 
#else 
#	define UDT_LIKELY(cond)    (!!(cond))
#	define UDT_UNLIKELY(cond)  (!!(cond))
#endif

// Helps eliminate the default switch statements tests.
#if defined(UDT_MSVC)
#	define UDT_DEAD_DEFAULT __assume(0)
#else
#	define UDT_DEAD_DEFAULT
#endif

// Debug break.
#if defined(UDT_MSVC) && !defined(UDT_ICC)
#	pragma intrinsic(__debugbreak)
#	define UDT_HAS_DEBUG_BREAK
#	define UDT_DEBUG_BREAK __debugbreak
#else // @TODO: "int 3" as inline assembler for other x86 compilers.
#	define UDT_DEBUG_BREAK
#endif

// Debugging on?
#if defined(__DEBUG) || defined(_DEBUG)
#	define UDT_DEBUG
#endif

// For unused variables or expressions.
#define UDT_UNUSED(x) do { (void)sizeof(x); } while(0)

// For classes that don't have copy semantics.
#define UDT_NO_COPY_SEMANTICS(ClassName) \
private: \
	ClassName(const ClassName&); \
	ClassName& operator=(const ClassName&);

namespace udt { namespace impl
{
	template<bool> struct CompileTimeError;
	template<> struct CompileTimeError<true> {};
}}

#define UDT_STATIC_ASSERT_IMPL(prefix, expr, msg) \
{ ::udt::impl::CompileTimeError<((expr) != 0)> prefix##_##msg; (void)prefix##_##msg; } 

// Assert compile-time conditions and create customized errors inside code sections.
#define UDT_STATIC_ASSERT(expr, msg)	UDT_STATIC_ASSERT_IMPL(ERROR, expr, msg)
#define UDT_TODO(msg)					UDT_STATIC_ASSERT_IMPL(TODO, false, msg)
#define UDT_FIXME(msg)					UDT_STATIC_ASSERT_IMPL(FIXME, false, msg)

// Useful as a macro argument :p
#define UDT_NOTHING

// Determine if we can use Microsoft's inline assembler.
// x86 only for MSVC, x86 and x64 for ICC.
#if (defined(UDT_X86) && (defined(UDT_MSVC) || defined(UDT_ICC))) || \
	(defined(UDT_X64) && defined(UDT_ICC))
#	define UDT_MSVC_INLINE_ASM 1
#endif

// MMX/SSE s32rinsics support.
#if defined(UDT_MSVC) || defined(UDT_ICC) || defined(UDT_GCC)
#	define UDT_INTRINSICS
#endif

#ifndef NULL
#	define NULL 0
#endif

namespace udt { namespace impl
{
	template<typename T, unsigned int N>
	unsigned int countof(T(&)[N])
	{
		return N;
	}
}}

// Number of elements in the specified array. Won't compile if you pass a pointer.
#define UDT_COUNT_OF(array)    (::udt::impl::countof(array))

/*
Static analysis annotation macros.
*/
// "stringIndex" and "firstToCheck" are 1-based indices. 
// If annotating a member function, add 1 to account for the "this" argument.
#if defined(UDT_MSVC)
#	define UDT_PRINTF_FORMAT_ARG _Printf_format_string_
#	define UDT_PRINTF_POST_FUNCTION(stringIndex, firstToCheck)
#elif defined(UDT_GCC)
#	define UDT_PRINTF_FORMAT_ARG
#	define UDT_PRINTF_POST_FUNCTION(stringIndex, firstToCheck) __attribute__ ((format (printf, stringIndex, firstToCheck)))
#else
#	define UDT_PRINTF_FORMAT_ARG
#	define UDT_PRINTF_POST_FUNCTION(stringIndex, firstToCheck)
#endif
