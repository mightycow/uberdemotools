#include "macros.hpp"

#if defined(UDT_WINDOWS) && !defined(UDT_VIEWER_WINDOWS_GLFW)
#	include "platform_win32.hpp"
#else
#	include "platform_glfw.hpp"
#endif
