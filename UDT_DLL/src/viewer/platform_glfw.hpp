#include "uberdemotools.h"
#include "platform.hpp"
#include "viewer.hpp"
#include "log.hpp"

// The default (16 KB) is too small.
#define FONS_SCRATCH_BUF_SIZE (1 << 16)
#include "nanovg/nanovg.h"

#include <stdio.h>
#include <stdarg.h>
#if !defined(UDT_WINDOWS)
#	include <execinfo.h>
#	include <signal.h>
#endif

#if defined(UDT_WINDOWS)
#	include <Windows.h>
#	include "GL/glew.h"
#	include "GLFW/glfw3.h"
#else
#	define GL_GLEXT_PROTOTYPES
#	include <GL/gl.h>
#	include <GL/glext.h>
#	include <GLFW/glfw3.h>
#endif

#if defined(UDT_MSVC)
#	pragma warning(push, 0)
#elif defined(UDT_GCC)
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#	pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define NANOVG_GL2_IMPLEMENTATION
#include "nanovg/nanovg_gl.h"
#include "nanovg/nanovg.c"

#if defined(UDT_MSVC)
#	pragma warning(pop)
#elif defined(UDT_GCC)
#	pragma GCC diagnostic pop
#endif


static void GlobalErrorCallback(int error, const char* desc)
{
	fprintf(stderr, "GLFW error %d: %s\n", error, desc);
}

static VirtualKey::Id GetVirtualKeyId(int key)
{
	if(key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
	{
		return (VirtualKey::Id)((int)VirtualKey::N0 + (key - GLFW_KEY_0));
	}
	
	if(key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12)
	{
		return (VirtualKey::Id)((int)VirtualKey::F1 + (key - GLFW_KEY_F1));
	}
	
	if(key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9)
	{
		return (VirtualKey::Id)((int)VirtualKey::Numpad0 + (key - GLFW_KEY_KP_0));
	}
	
	if(key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
	{
		return (VirtualKey::Id)((int)VirtualKey::A + (key - GLFW_KEY_A));
	}

	switch(key)
	{
		case GLFW_KEY_SPACE: return VirtualKey::Space;
		case GLFW_KEY_ENTER: return VirtualKey::Return;
		case GLFW_KEY_ESCAPE: return VirtualKey::Escape;
		case GLFW_KEY_PAGE_UP: return VirtualKey::PageUp;
		case GLFW_KEY_PAGE_DOWN: return VirtualKey::PageDown;
		case GLFW_KEY_LEFT: return VirtualKey::LeftArrow;
		case GLFW_KEY_RIGHT: return VirtualKey::RightArrow;
		case GLFW_KEY_UP: return VirtualKey::UpArrow;
		case GLFW_KEY_DOWN: return VirtualKey::DownArrow;
		default: return VirtualKey::Unknown;
	}
}

static void GlobalKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void GlobalCursorPosCallback(GLFWwindow* window, double x, double y);
static void GlobalMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
static void GlobalScrollCallback(GLFWwindow* window, double dx, double dy);
static void GlobalDropCallback(GLFWwindow* window, int count, const char** paths);
static void GlobalWindowRefreshCallback(GLFWwindow* window);


struct Platform
{
	Platform()
	{
	}

	~Platform()
	{
		Destroy();
	}

	void MainLoop()
	{
		const double MaxWaitTime = 1.0 / 40.0;

		GLFWwindow* const window = _window;
		NVGcontext* const nvg = _sharedReadOnly.NVGContext;
		RenderParams renderParams;
		
		// Time stamp of the last render.
		// Time 0 is the time glfwInit was called.
		double prevTime = -1.0;
		while(!glfwWindowShouldClose(window))
		{
			const double currTime = glfwGetTime();
			const double elapsed = currTime - prevTime;
			if(elapsed < MaxWaitTime)
			{
				glfwWaitEventsTimeout(MaxWaitTime - elapsed);
				continue;
			}
			
			if(glfwGetWindowAttrib(window, GLFW_ICONIFIED))
			{
				prevTime = currTime;
				continue;
			}
			
			int winWidth, winHeight;
			int fbWidth, fbHeight;
			glfwGetWindowSize(window, &winWidth, &winHeight);
			glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
			
			renderParams.ClientWidth = winWidth;
			renderParams.ClientHeight = winHeight;
			renderParams.NVGContext = nvg;
			
			glViewport(0, 0, fbWidth, fbHeight);
			glClearColor(ViewerClearColor[0], ViewerClearColor[1], ViewerClearColor[2], ViewerClearColor[3]);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			
			nvgBeginFrame(nvg, winWidth, winHeight, (float)fbWidth / (float)winWidth);
			_viewer->Render(renderParams);
			nvgEndFrame(nvg);
			
			glfwSwapBuffers(window);
			
			glfwWaitEventsTimeout(MaxWaitTime);

			prevTime = currTime;
		}
	}

	bool Init()
	{
		int glfwMajor, glfwMinor, glfwRevision;
		glfwGetVersion(&glfwMajor, &glfwMinor, &glfwRevision);
		if(glfwMajor < 3)
		{
			Platform_PrintError("The glfw version must be 3.0 or higher");
			return false;
		}
	
		if(!glfwInit())
		{
			Platform_PrintError("glfwInit failed");
			return false;
		}
	
		glfwSetErrorCallback(&GlobalErrorCallback);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		glfwWindowHint(GLFW_SAMPLES, 4);
		GLFWwindow* const window = glfwCreateWindow(1024, 768, "UDT 2D Viewer", NULL, NULL);
		if(window == nullptr)
		{
			Platform_PrintError("glfwCreateWindow failed");
			return false;
		}
		_window = window;
		
		glfwSetWindowUserPointer(window, this);
		if((glfwMajor == 3 && glfwMinor >= 2) || glfwMajor >= 4)
		{
			glfwSetWindowSizeLimits(window, 640, 480, GLFW_DONT_CARE, GLFW_DONT_CARE);
		}
		glfwMakeContextCurrent(window);
#if defined(UDT_WINDOWS)
		glewInit();
#endif
		glfwSetKeyCallback(window, &GlobalKeyCallback);
		glfwSetCursorPosCallback(window, &GlobalCursorPosCallback);
		glfwSetMouseButtonCallback(window, &GlobalMouseButtonCallback);
		glfwSetScrollCallback(window, &GlobalScrollCallback);
		if((glfwMajor == 3 && glfwMinor >= 1) || glfwMajor >= 4)
		{
			glfwSetDropCallback(window, &GlobalDropCallback);
		}
		glfwSetWindowRefreshCallback(window, &GlobalWindowRefreshCallback);
		glfwSwapInterval(0);
		
#if defined(UDT_DEBUG)
		NVGcontext* const nvg = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#else
		NVGcontext* const nvg = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#endif
		if(nvg == nullptr)
		{
			Platform_PrintError("nvgCreateGL2 failed");
			return false;
		}
		_sharedReadOnly.NVGContext = nvg;

		int fbWidth, fbHeight;
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
		glViewport(0, 0, fbWidth, fbHeight);
		glClearColor(ViewerClearColor[0], ViewerClearColor[1], ViewerClearColor[2], ViewerClearColor[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glfwSwapBuffers(window);
		
		return true;
	}

	void Destroy()
	{
		if(_sharedReadOnly.NVGContext != nullptr)
		{
			nvgDeleteGL2(_sharedReadOnly.NVGContext);
		}
		
		if(_window != nullptr)
		{
			glfwDestroyWindow(_window);
		}
		
		glfwTerminate();
	}
	
	void KeyCallback(int action, int key)
	{	
		Event event;
		if(action == GLFW_PRESS)
		{
			event.Type = EventType::KeyDown;
		}
		else if(action == GLFW_RELEASE)
		{
			event.Type = EventType::KeyUp;
		}
		else if(action == GLFW_REPEAT)
		{
			event.Type = EventType::KeyDownRepeat;
		}
		event.VirtualKeyId = GetVirtualKeyId(key);
		_viewer->ProcessEvent(event);
	}
	
	void CursorPosCallback(s32 x, s32 y)
	{
		Event event;
		event.Type = EventType::MouseMove;
		event.CursorPos[0] = x;
		event.CursorPos[1] = y;
		_viewer->ProcessEvent(event);
	}
	
	void MouseButtonCallback(int action, int button)
	{
		Event event;
		if(action == GLFW_PRESS)
		{
			event.Type =  EventType::MouseButtonDown;
		}
		else if(action == GLFW_RELEASE)
		{
			event.Type =  EventType::MouseButtonUp;
		}
		else
		{
			return;
		}
		if(button == GLFW_MOUSE_BUTTON_LEFT)
		{
			event.MouseButtonId = MouseButton::Left;
		}
		else if(button == GLFW_MOUSE_BUTTON_MIDDLE)
		{
			event.MouseButtonId = MouseButton::Middle;
		}
		else if(button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			event.MouseButtonId = MouseButton::Right;
		}
		else
		{
			return;
		}		
		double x, y;
		glfwGetCursorPos(_window, &x, &y);
		event.CursorPos[0] = (s32)x;
		event.CursorPos[1] = (s32)y;
		_viewer->ProcessEvent(event);
	}

	void ScrollCallback(s32 scroll)
	{
		double x, y;
		glfwGetCursorPos(_window, &x, &y);
		Event event;
		event.Type = EventType::MouseScroll;
		event.Scroll = scroll;
		event.CursorPos[0] = (s32)x;
		event.CursorPos[1] = (s32)y;
		_viewer->ProcessEvent(event);
	}
	
	void DropCallback(const char** paths, int count)
	{
		Event event;
		event.Type = EventType::FilesDropped;
		event.DroppedFileCount = (u32)count;
		event.DroppedFilePaths = paths;
		_viewer->ProcessEvent(event);
	}
	
	void WindowRefreshCallback()
	{
		ReDraw();
	}
	
	void ReDraw()
	{
		GLFWwindow* const window = _window;
		NVGcontext* const nvg = _sharedReadOnly.NVGContext;

		int winWidth, winHeight;
		int fbWidth, fbHeight;
		glfwGetWindowSize(window, &winWidth, &winHeight);
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
		
		RenderParams renderParams;	
		renderParams.ClientWidth = winWidth;
		renderParams.ClientHeight = winHeight;
		renderParams.NVGContext = nvg;
	
		glViewport(0, 0, fbWidth, fbHeight);
		glClearColor(ViewerClearColor[0], ViewerClearColor[1], ViewerClearColor[2], ViewerClearColor[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
		nvgBeginFrame(nvg, winWidth, winHeight, (float)fbWidth / (float)winWidth);
		_viewer->Render(renderParams);
		nvgEndFrame(nvg);
	
		glfwSwapBuffers(window);
	}

	PlatformReadOnly _sharedReadOnly;
	PlatformReadWrite _sharedReadWrite;
	Viewer* _viewer = nullptr;
	GLFWwindow* _window = nullptr;
};

static void GlobalKeyCallback(GLFWwindow* window, int key, int, int action, int)
{	
	((Platform*)glfwGetWindowUserPointer(window))->KeyCallback(action, key);
}

static void GlobalCursorPosCallback(GLFWwindow* window, double x, double y)
{
	((Platform*)glfwGetWindowUserPointer(window))->CursorPosCallback((s32)x, (s32)y);
}

static void GlobalMouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
	((Platform*)glfwGetWindowUserPointer(window))->MouseButtonCallback(action, button);
}

static void GlobalScrollCallback(GLFWwindow* window, double, double dy)
{
	((Platform*)glfwGetWindowUserPointer(window))->ScrollCallback((s32)dy);
}

static void GlobalDropCallback(GLFWwindow* window, int count, const char** paths)
{
	((Platform*)glfwGetWindowUserPointer(window))->DropCallback(paths, count);
}

static void GlobalWindowRefreshCallback(GLFWwindow* window)
{
	((Platform*)glfwGetWindowUserPointer(window))->WindowRefreshCallback();
}

void Platform_RequestQuit(Platform& platform)
{
	glfwSetWindowShouldClose(platform._window, GL_TRUE);
}

void Platform_GetSharedDataPointers(Platform& platform, const PlatformReadOnly** readOnly, PlatformReadWrite** readWrite)
{
	if(readOnly != nullptr)
	{
		*readOnly = &platform._sharedReadOnly;
	}

	if(readWrite != nullptr)
	{
		*readWrite = &platform._sharedReadWrite;
	}
}

void Platform_SetCursorCapture(Platform&, bool)
{
}

void Platform_NVGBeginFrame(Platform& platform)
{
	int winWidth, winHeight;
	int fbWidth, fbHeight;
	glfwGetWindowSize(platform._window, &winWidth, &winHeight);
	glfwGetFramebufferSize(platform._window, &fbWidth, &fbHeight);

	nvgBeginFrame(platform._sharedReadOnly.NVGContext, winWidth, winHeight, (float)fbWidth / (float)winWidth);
}

void Platform_NVGEndFrame(Platform& platform)
{
	nvgEndFrame(platform._sharedReadOnly.NVGContext);
}

void Platform_Draw(Platform& platform)
{
	platform.ReDraw();
}

void Platform_ToggleMaximized(Platform& platform)
{
	if(glfwGetWindowAttrib(platform._window, GLFW_MAXIMIZED))
	{
		glfwRestoreWindow(platform._window);
	}
	else
	{
		glfwMaximizeWindow(platform._window);
	}
}

void Platform_GetCursorPosition(Platform& platform, s32& x, s32& y)
{
	double cx, cy;
	glfwGetCursorPos(platform._window, &cx, &cy);
	x = (s32)cx;
	y = (s32)cy;
}

static void udtCrashHandler(const char* message)
{
	Platform_FatalError(message);
}

#if defined(UDT_WINDOWS)

#include "thread_local_allocators.hpp"
#include "scoped_stack_allocator.hpp"
#include "path.hpp"
#include "utils.hpp"
#include "windows.hpp"

static int Main()
{
	udtSetCrashHandler(&udtCrashHandler);
	udtInitLibrary();

	Log::Init();

	Platform platform;
	Viewer viewer(platform);
	platform._viewer = &viewer;

	int argc;
	LPWSTR* const argvWide = CommandLineToArgvW(GetCommandLineW(), &argc);
	udtVMLinearAllocator& alloc = udtThreadLocalAllocators::GetTempAllocator();
	uptr argvOffsets[16];
	char* argv[16];
	argc = udt_min(argc, 16);
	for(int i = 0; i < argc; ++i)
	{
		argvOffsets[i] = udtString::NewFromUTF16(alloc, argvWide[i]).GetOffset();
	}
	for(int i = 0; i < argc; ++i)
	{
		argv[i] = (char*)alloc.GetAddressAt(argvOffsets[i]);
	}
	LocalFree(argvWide);
	ResetCurrentDirectory(argv[0]);

	int result = 0;
	if(!platform.Init() ||
	   !viewer.Init(argc, argv))
	{
		result = 1;
		goto shut_down;
	}

	platform.ReDraw();
	platform.MainLoop();

shut_down:
	viewer.ShutDown();
	Log::Destroy();
	udtShutDownLibrary();

	return result;
}

int CALLBACK wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	__try
	{
		return Main();
	}
	__except(Win32ExceptionFilter())
	{
		return Win32ExceptionHandler((int)GetExceptionCode());
	}
}

#else

#include <pthread.h>
#include "linux.hpp"

static void SignalHandler(int signal)
{
	const char* message = "Unknown signal received, closing now.";
	if(signal == SIGSEGV)
	{
		message = "Segmentation fault (SIGSEGV) detected.";
	}
	else if(signal == SIGFPE)
	{
		message = "Erroneous arithmetic operation (SIGFPE) detected.";
	}
	else if(signal == SIGBUS)
	{
		message = "Bus error (SIGBUS) detected.";
	}

	Platform_FatalError(message);
}

int main(int argc, char** argv)
{
	signal(SIGSEGV, &SignalHandler);
	signal(SIGFPE, &SignalHandler);
	signal(SIGBUS, &SignalHandler);

	udtSetCrashHandler(&udtCrashHandler);
	udtInitLibrary();

	Log::Init();
	
	Platform platform;
	Viewer viewer(platform);
	platform._viewer = &viewer;

	int result = 0;
	if(!platform.Init() || 
	   !viewer.Init(argc, argv))
	{
		result = 1;
		goto shut_down;
	}

	platform.ReDraw();
	platform.MainLoop();

shut_down:
	viewer.ShutDown();
	Log::Destroy();
	udtShutDownLibrary();

	return result;
}

#endif
