#include "uberdemotools.h"
#if defined(UDT_MSVC)
#	pragma warning(push, 0)
#endif

#define NANOVG_D3D11_IMPLEMENTATION
#include "nanovg/nanovg_d3d11.h"
#include "nanovg/nanovg.c"

#if defined(UDT_MSVC)
#	pragma warning(pop)
#endif
