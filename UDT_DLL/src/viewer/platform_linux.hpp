#include "uberdemotools.h"
#include "platform.hpp"
#include "viewer.hpp"
#include "nanovg/nanovg.h"

#include <stdio.h>
#include <stdarg.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include <GLFW/glfw3.h>

#if defined(UDT_GCC)
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#	pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define NANOVG_GL2_IMPLEMENTATION
#include "nanovg/nanovg_gl.h"
#include "nanovg/nanovg.c"

#if defined(UDT_GCC)
#	pragma GCC diagnostic pop
#endif


static void LogError(const char* format, ...)
{
	char msg[256];
	va_list args;
	va_start(args, format);
	vsprintf(msg, format, args);
	va_end(args);
	fprintf(stderr, "Error: ");
	fprintf(stderr, msg);
	fprintf(stderr, "\n");
}

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
#if (GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 1) || (GLFW_VERSION_MAJOR >= 4)
static void GlobalDropCallback(GLFWwindow* window, int count, const char** paths);
#endif


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
		GLFWwindow* const window = _window;
		NVGcontext* const nvg = _sharedReadOnly.NVGContext;
		RenderParams renderParams;
		
		while(!glfwWindowShouldClose(window))
		{
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
			_viewer->Update();
			_viewer->Render(renderParams);
			nvgEndFrame(nvg);
			
			glfwSwapBuffers(window);
			
			glfwPollEvents();
		}
	}

	bool Init()
	{
		if(!glfwInit())
		{
			LogError("glfwInit failed");
			return false;
		}
	
		glfwSetErrorCallback(&GlobalErrorCallback);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		glfwWindowHint(GLFW_SAMPLES, 4);
		GLFWwindow* const window = glfwCreateWindow(640, 480, "UDT 2D Viewer", NULL, NULL);
		if(window == nullptr)
		{
			LogError("glfwCreateWindow failed");
			return false;
		}
		_window = window;
		
		glfwSetWindowUserPointer(window, this);
#if (GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 2) || (GLFW_VERSION_MAJOR >= 4)
		glfwSetWindowSizeLimits(window, 640, 480, GLFW_DONT_CARE, GLFW_DONT_CARE);
#endif
		glfwMakeContextCurrent(window);
		glfwSetKeyCallback(window, &GlobalKeyCallback);
		glfwSetCursorPosCallback(window, &GlobalCursorPosCallback);
		glfwSetMouseButtonCallback(window, &GlobalMouseButtonCallback);
#if (GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 1) || (GLFW_VERSION_MAJOR >= 4)
		glfwSetDropCallback(window, &GlobalDropCallback);
#endif
		glfwSwapInterval(0);
		
		NVGcontext* const nvg = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
		if(nvg == nullptr)
		{
			LogError("nvgCreateGL2 failed");
			return false;
		}
		_sharedReadOnly.NVGContext = nvg;
		
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
	
	void DropCallback(const char** paths, int count)
	{
		Event event;
		event.Type = EventType::FilesDropped;
		event.DroppedFileCount = (u32)count;
		event.DroppedFilePaths = paths;
		_viewer->ProcessEvent(event);
	}

	PlatformReadOnly _sharedReadOnly;
	PlatformReadWrite _sharedReadWrite;
	Viewer* _viewer = nullptr;
	GLFWwindow* _window = nullptr;
	bool _drawRequested = false;
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

#if (GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 1) || (GLFW_VERSION_MAJOR >= 4)
static void GlobalDropCallback(GLFWwindow* window, int count, const char** paths)
{
	((Platform*)glfwGetWindowUserPointer(window))->DropCallback(paths, count);
}
#endif

void Platform_RequestDraw(Platform& platform)
{
	platform._drawRequested = true;
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

void Platform_PerformAction(Platform& platform, const PlatformAction& action)
{
	switch(action.Type)
	{
		case PlatformActionType::Quit:
			glfwSetWindowShouldClose(platform._window, GL_TRUE);
			break;

		case PlatformActionType::Minimize:
			glfwIconifyWindow(platform._window);
			break;

		case PlatformActionType::Maximize:
#if (GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 1) || (GLFW_VERSION_MAJOR >= 4)
			glfwMaximizeWindow(platform._window);
#endif
			break;

		default:
			break;
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

void Platform_DebugPrint(const char* format, ...)
{
	char msg[256];
	va_list args;
	va_start(args, format);
	vsprintf(msg, format, args);
	va_end(args);
	printf("\n");
	printf(msg);
	printf("\n");
}

int main(int argc, char** argv)
{
	udtInitLibrary();
	
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

	platform.MainLoop();

shut_down:
	udtShutDownLibrary();

	return result;
}
