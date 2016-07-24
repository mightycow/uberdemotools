#include "macros.hpp"

#if defined(UDT_MSVC)
#	pragma warning(disable: 4505)
#	pragma warning(push, 0)
#else
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "nanovg/nanovg.h"
#define BLENDISH_IMPLEMENTATION
#include "blendish/blendish.h"

#if defined(UDT_MSVC)
#	pragma warning(pop)
#else
#	pragma GCC diagnostic pop
#endif
