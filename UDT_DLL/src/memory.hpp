#pragma once


#include <stdlib.h> // For size_t.


// This will invoke malloc and throw a fatal error if it failed.
extern void* udt_malloc(size_t byteCount);
