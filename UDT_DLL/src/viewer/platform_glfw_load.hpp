typedef int (*PFN_glfwInit)(void);
typedef void (*PFN_glfwTerminate)(void);
typedef void (*PFN_glfwGetVersion)(int*, int*, int*);
typedef GLFWerrorfun(*PFN_glfwSetErrorCallback)(GLFWerrorfun);
typedef void (*PFN_glfwWindowHint)(int, int);
typedef GLFWwindow* (*PFN_glfwCreateWindow)(int, int, const char*, GLFWmonitor*, GLFWwindow*);
typedef void (*PFN_glfwDestroyWindow)(GLFWwindow*);
typedef int (*PFN_glfwWindowShouldClose)(GLFWwindow*);
typedef void (*PFN_glfwSetWindowShouldClose)(GLFWwindow*, int);
typedef void (*PFN_glfwSetWindowUserPointer)(GLFWwindow*, void*);
typedef void* (*PFN_glfwGetWindowUserPointer)(GLFWwindow*);
typedef void (*PFN_glfwSetWindowSizeLimits)(GLFWwindow*, int, int, int, int);
typedef void (*PFN_glfwGetWindowSize)(GLFWwindow*, int*, int*);
typedef void (*PFN_glfwGetFramebufferSize)(GLFWwindow*, int*, int*);
typedef int (*PFN_glfwGetWindowAttrib)(GLFWwindow*, int);
typedef void (*PFN_glfwRestoreWindow)(GLFWwindow*);
typedef void (*PFN_glfwMaximizeWindow)(GLFWwindow*);
typedef void (*PFN_glfwGetCursorPos)(GLFWwindow*, double*, double*);
typedef GLFWkeyfun(*PFN_glfwSetKeyCallback)(GLFWwindow*, GLFWkeyfun);
typedef GLFWcursorposfun(*PFN_glfwSetCursorPosCallback)(GLFWwindow*, GLFWcursorposfun);
typedef GLFWmousebuttonfun(*PFN_glfwSetMouseButtonCallback)(GLFWwindow*, GLFWmousebuttonfun);
typedef GLFWscrollfun(*PFN_glfwSetScrollCallback)(GLFWwindow*, GLFWscrollfun);
typedef GLFWdropfun(*PFN_glfwSetDropCallback)(GLFWwindow*, GLFWdropfun);
typedef GLFWwindowrefreshfun(*PFN_glfwSetWindowRefreshCallback)(GLFWwindow*, GLFWwindowrefreshfun);
typedef void (*PFN_glfwMakeContextCurrent)(GLFWwindow*);
typedef void (*PFN_glfwSwapBuffers)(GLFWwindow*);
typedef void (*PFN_glfwSwapInterval)(int);
typedef GLFWglproc(*PFN_glfwGetProcAddress)(const char*);
typedef double (*PFN_glfwGetTime)(void);
typedef void (*PFN_glfwWaitEventsTimeout)(double);

static PFN_glfwInit ext_glfwInit = nullptr;
static PFN_glfwTerminate ext_glfwTerminate = nullptr;
static PFN_glfwGetVersion ext_glfwGetVersion = nullptr;
static PFN_glfwSetErrorCallback ext_glfwSetErrorCallback = nullptr;
static PFN_glfwWindowHint ext_glfwWindowHint = nullptr;
static PFN_glfwCreateWindow ext_glfwCreateWindow = nullptr;
static PFN_glfwDestroyWindow ext_glfwDestroyWindow = nullptr;
static PFN_glfwWindowShouldClose ext_glfwWindowShouldClose = nullptr;
static PFN_glfwSetWindowShouldClose ext_glfwSetWindowShouldClose = nullptr;
static PFN_glfwSetWindowUserPointer ext_glfwSetWindowUserPointer = nullptr;
static PFN_glfwGetWindowUserPointer ext_glfwGetWindowUserPointer = nullptr;
static PFN_glfwSetWindowSizeLimits ext_glfwSetWindowSizeLimits = nullptr;
static PFN_glfwGetWindowSize ext_glfwGetWindowSize = nullptr;
static PFN_glfwGetFramebufferSize ext_glfwGetFramebufferSize = nullptr;
static PFN_glfwGetWindowAttrib ext_glfwGetWindowAttrib = nullptr;
static PFN_glfwRestoreWindow ext_glfwRestoreWindow = nullptr;
static PFN_glfwMaximizeWindow ext_glfwMaximizeWindow = nullptr;
static PFN_glfwGetCursorPos ext_glfwGetCursorPos = nullptr;
static PFN_glfwSetKeyCallback ext_glfwSetKeyCallback = nullptr;
static PFN_glfwSetCursorPosCallback ext_glfwSetCursorPosCallback = nullptr;
static PFN_glfwSetMouseButtonCallback ext_glfwSetMouseButtonCallback = nullptr;
static PFN_glfwSetScrollCallback ext_glfwSetScrollCallback = nullptr;
static PFN_glfwSetDropCallback ext_glfwSetDropCallback = nullptr;
static PFN_glfwSetWindowRefreshCallback ext_glfwSetWindowRefreshCallback = nullptr;
static PFN_glfwMakeContextCurrent ext_glfwMakeContextCurrent = nullptr;
static PFN_glfwSwapBuffers ext_glfwSwapBuffers = nullptr;
static PFN_glfwSwapInterval ext_glfwSwapInterval = nullptr;
static PFN_glfwGetProcAddress ext_glfwGetProcAddress = nullptr;
static PFN_glfwGetTime ext_glfwGetTime = nullptr;
static PFN_glfwWaitEventsTimeout ext_glfwWaitEventsTimeout = nullptr;

#define glfwInit ext_glfwInit
#define glfwTerminate ext_glfwTerminate
#define glfwGetVersion ext_glfwGetVersion
#define glfwSetErrorCallback ext_glfwSetErrorCallback
#define glfwWindowHint ext_glfwWindowHint
#define glfwCreateWindow ext_glfwCreateWindow
#define glfwDestroyWindow ext_glfwDestroyWindow
#define glfwWindowShouldClose ext_glfwWindowShouldClose
#define glfwSetWindowShouldClose ext_glfwSetWindowShouldClose
#define glfwSetWindowUserPointer ext_glfwSetWindowUserPointer
#define glfwGetWindowUserPointer ext_glfwGetWindowUserPointer
#define glfwSetWindowSizeLimits ext_glfwSetWindowSizeLimits
#define glfwGetWindowSize ext_glfwGetWindowSize
#define glfwGetFramebufferSize ext_glfwGetFramebufferSize
#define glfwGetWindowAttrib ext_glfwGetWindowAttrib
#define glfwRestoreWindow ext_glfwRestoreWindow
#define glfwMaximizeWindow ext_glfwMaximizeWindow
#define glfwGetCursorPos ext_glfwGetCursorPos
#define glfwSetKeyCallback ext_glfwSetKeyCallback
#define glfwSetCursorPosCallback ext_glfwSetCursorPosCallback
#define glfwSetMouseButtonCallback ext_glfwSetMouseButtonCallback
#define glfwSetScrollCallback ext_glfwSetScrollCallback
#define glfwSetDropCallback ext_glfwSetDropCallback
#define glfwSetWindowRefreshCallback ext_glfwSetWindowRefreshCallback
#define glfwMakeContextCurrent ext_glfwMakeContextCurrent
#define glfwSwapBuffers ext_glfwSwapBuffers
#define glfwSwapInterval ext_glfwSwapInterval
#define glfwGetProcAddress ext_glfwGetProcAddress
#define glfwGetTime ext_glfwGetTime
#define glfwWaitEventsTimeout ext_glfwWaitEventsTimeout

#if defined(_WIN32)
#	define LOAD_LIBRARY(name) LoadLibraryA(name)
#	define GET_PROC(handle, name) GetProcAddress(handle, name)
#	define FREE_LIBRARY(handle) FreeLibrary(handle)
typedef HMODULE LibraryHandle;
static const char* s_glfwLibraryNames[] = { "glfw3.dll" };
#else
#	define LOAD_LIBRARY(name) dlopen(name, RTLD_LAZY | RTLD_GLOBAL)
#	define GET_PROC(handle, name) dlsym(handle, name)
#	define FREE_LIBRARY(handle) dlclose(handle)
typedef void* LibraryHandle;
static const char* s_glfwLibraryNames[] = { "libglfw.so.3", "libglfw.so" };
#endif

static LibraryHandle s_glfwInstance = nullptr;

static bool LoadGLFW()
{
	if(s_glfwInstance != nullptr)
	{
		return true;
	}

	for(size_t i = 0, count = UDT_ARRAY_LENGTH(s_glfwLibraryNames); i < count; ++i)
	{
		s_glfwInstance = LOAD_LIBRARY(s_glfwLibraryNames[i]);
		if(s_glfwInstance != nullptr)
		{
			break;
		}
	}

	if(s_glfwInstance == nullptr)
	{
		return false;
	}

#define LOAD_FUNC(name) \
	ext_##name = (PFN_##name)GET_PROC(s_glfwInstance, #name); \
	if(ext_##name == nullptr) return false

	LOAD_FUNC(glfwInit);
	LOAD_FUNC(glfwTerminate);
	LOAD_FUNC(glfwGetVersion);
	LOAD_FUNC(glfwSetErrorCallback);
	LOAD_FUNC(glfwWindowHint);
	LOAD_FUNC(glfwCreateWindow);
	LOAD_FUNC(glfwDestroyWindow);
	LOAD_FUNC(glfwWindowShouldClose);
	LOAD_FUNC(glfwSetWindowShouldClose);
	LOAD_FUNC(glfwSetWindowUserPointer);
	LOAD_FUNC(glfwGetWindowUserPointer);
	LOAD_FUNC(glfwSetWindowSizeLimits);
	LOAD_FUNC(glfwGetWindowSize);
	LOAD_FUNC(glfwGetFramebufferSize);
	LOAD_FUNC(glfwGetWindowAttrib);
	LOAD_FUNC(glfwRestoreWindow);
	LOAD_FUNC(glfwMaximizeWindow);
	LOAD_FUNC(glfwGetCursorPos);
	LOAD_FUNC(glfwSetKeyCallback);
	LOAD_FUNC(glfwSetCursorPosCallback);
	LOAD_FUNC(glfwSetMouseButtonCallback);
	LOAD_FUNC(glfwSetScrollCallback);
	LOAD_FUNC(glfwSetDropCallback);
	LOAD_FUNC(glfwSetWindowRefreshCallback);
	LOAD_FUNC(glfwMakeContextCurrent);
	LOAD_FUNC(glfwSwapBuffers);
	LOAD_FUNC(glfwSwapInterval);
	LOAD_FUNC(glfwGetProcAddress);
	LOAD_FUNC(glfwGetTime);
	LOAD_FUNC(glfwWaitEventsTimeout);

#undef LOAD_FUNC

	return true;
}

static void UnloadGLFW()
{
	if(s_glfwInstance != nullptr)
	{
		FREE_LIBRARY(s_glfwInstance);
		s_glfwInstance = nullptr;
	}
}

#undef LOAD_LIBRARY
#undef GET_PROC
#undef FREE_LIBRARY
