#pragma once


#include "crash.hpp"

#include <assert.h>


#if defined(UDT_DEBUG)
#	define UDT_ASSERT_OR_FATAL(exp)                  assert((exp))
#	define UDT_ASSERT_OR_FATAL_MSG(exp, msg, ...)    assert((exp))
#	define UDT_ASSERT_OR_FATAL_ALWAYS(msg, ...)      assert((msg && false))
#else
#	define UDT_ASSERT_OR_FATAL(exp)                  if(!(exp)) FatalError(__FILE__, __LINE__, __FUNCTION__, #exp)
#	define UDT_ASSERT_OR_FATAL_MSG(exp, msg, ...)    if(!(exp)) FatalError(__FILE__, __LINE__, __FUNCTION__, msg, ##__VA_ARGS__)
#	define UDT_ASSERT_OR_FATAL_ALWAYS(msg, ...)      FatalError(__FILE__, __LINE__, __FUNCTION__, msg, ##__VA_ARGS__)
#endif
