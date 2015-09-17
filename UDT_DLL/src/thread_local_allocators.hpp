#pragma once


#include "linear_allocator.hpp"


namespace udtThreadLocalAllocators
{
	// Global calls.
	extern void Init();
	extern void Destroy();

	// Thread-local calls.
	extern udtVMLinearAllocator& GetTempAllocator();
	extern void                  ReleaseThreadLocalAllocators();
}
