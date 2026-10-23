// Copyright (c) 2020 Leedehai. All rights reserved.
// Use of this source code is governed under the LICENSE.txt file.

#ifndef SBLZ_INCLUDE_SBLZ_SBLZ_H_
#define SBLZ_INCLUDE_SBLZ_SBLZ_H_

#include <cstddef>

namespace sblz {

namespace posix {

/// Retrieves the mangled symbol from the running process's memory
/// that corresponds to the function call represented by the input
/// address (instruction address, in the program counter register)
/// returned by system call backtrace(), and then puts that symbol
/// string to the buffer, then returns true on success.
/// As an exception, if the symbol was stripped in the binary, the
/// instruction address will be written instead. This situation is
/// still considered an success.
/// @param address The memory address got from backtrace().
/// @param buffer The output buffer.
/// @param buffer_size Buffer size, including the space for '\0'.
bool Symbolize(void* address, char* buffer, size_t buffer_size);

}  // namespace posix

namespace itanium {

/// Demangles a symbol according to the Itanium C++ ABI and writes
/// the result to the output buffer, then returns true on success.
/// https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling
/// @param symbol The the mangled symbol as a C-string.
/// @param buffer [out] The output buffer
/// @param buffer_size Buffer size, including the space of '\0'.
bool Demangle(const char* symbol, char* buffer, size_t buffer_size);

}  // namespace itanium

}  // namespace sblz

#endif
