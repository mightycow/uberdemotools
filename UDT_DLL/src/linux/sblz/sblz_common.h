// Copyright (c) 2020 Leedehai. All rights reserved.
// Use of this source code is governed under the LICENSE.txt file.

#if defined(__clang__) || defined(__GNUC__)

#if !defined(__clang__) && defined(__GNUC__)
#warning "You should use Clang, though GCC is supported with best efforts."
#endif

#if defined(__linux__)
#define OS_LINUX 1
#elif defined(__APPLE__)
#define OS_MACOS 1
#endif

#define EXPORT __attribute__((visibility("default")))

#else

#error "This compiler is not supported."

#endif
