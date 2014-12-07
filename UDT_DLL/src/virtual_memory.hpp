#pragma once


#include "types.hpp"


extern void* VirtualMemoryReserve(u32 byteCount);
extern bool  VirtualMemoryCommit(void* address, u32 byteCount);
extern bool  VirtualMemoryDecommit(void* address, u32 byteCount);
extern bool  VirtualMemoryDecommitAndRelease(void* address, u32 byteCount);
