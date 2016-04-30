#include "memory.hpp"
#include "assert_or_fatal.hpp"


void* udt_malloc(size_t byteCount)
{
	void* const result = malloc(byteCount);
	UDT_ASSERT_OR_FATAL_MSG(result != NULL, "Call to malloc failed with %u bytes.", (u32)byteCount);

	return result;
}
