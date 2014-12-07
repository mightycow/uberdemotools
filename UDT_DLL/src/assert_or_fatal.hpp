#pragma once


#include "crash.hpp"

#include <assert.h>


#if defined(UDT_DEBUG)
#	define UDT_ASSERT_OR_FATAL(exp)    assert((exp))
#else
#	define UDT_ASSERT_OR_FATAL(exp)    if(!(exp)) FatalError(__FILE__, __LINE__, __FUNCTION__, #exp)
#endif

#define UDT_ASSERT_OR_FATAL_ALWAYS(msg)    UDT_ASSERT_OR_FATAL((msg && false))
