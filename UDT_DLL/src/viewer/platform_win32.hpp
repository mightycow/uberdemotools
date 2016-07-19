#include "uberdemotools.h"
#include "platform.hpp"
#include "viewer.hpp"
#include "thread_local_allocators.hpp"
#include "scoped_stack_allocator.hpp"
#include "string.hpp"
#include "path.hpp"
#include "utils.hpp"
#include "log.hpp"

// The default 16 KB buffer is too small.
#define FONS_SCRATCH_BUF_SIZE (1 << 16)
#include "nanovg/nanovg.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Windowsx.h>
#include <Shellapi.h>
#include <d3d11.h>
#include <assert.h>
#include <math.h>

#if defined(UDT_MSVC)
#	pragma warning(push, 0)
#endif

#define NANOVG_D3D11_IMPLEMENTATION
#include "nanovg/nanovg_d3d11.h"
#include "nanovg/nanovg.c"

#if defined(UDT_MSVC)
#	pragma warning(pop)
#endif


#define  WINDOW_CLASS_NAME  L"UDT_Viewer"
#define  TIMER_MAIN_ID      1

// The "(void)0,0" trick is to avoid a VC++ bug that makes it generate a warning (conditional expression is constant) it shouldn't.
#define  COM_RELEASE(comObjectPtr)  do { if(comObjectPtr != nullptr) { comObjectPtr->Release(); comObjectPtr = nullptr; } } while((void)0,0)


// @NOTE: The 2 alternatives to doing this are:
// 1. Use __uuidof(ID3D11Texture2D), which is a non-standard MS extension preventing us from building with MingW.
// 2. Use IID_ID3D11Texture2D in dxguid.h and link to dxguid.lib.
// Both of those options are massive piles of suck.
const GUID IID_ID3D11Texture2D = { 0x6f15aaf2, 0xd208, 0x4e89, { 0x9a, 0xb4, 0x48, 0x95, 0x35, 0xd3, 0x4f, 0x9c } };


static LRESULT CALLBACK MainWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

struct WindowState
{
	enum Id
	{
		Normal,
		Minimized,
		Maximized
	};
};


static MouseButton::Id GetButtonId(UINT message)
{
	switch(message)
	{
	case WM_LBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		return MouseButton::Left;

	case WM_MBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		return MouseButton::Middle;

	case WM_RBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		return MouseButton::Right;
	}

	return MouseButton::Unknown;
}

static VirtualKey::Id GetKeyId(WPARAM keyId)
{
	if(keyId >= 0x41 && keyId <= 0x5A)
	{
		return (VirtualKey::Id)((u32)VirtualKey::A + (u32)keyId - 0x41);
	}

	if(keyId >= 0x30 && keyId <= 0x39)
	{
		return (VirtualKey::Id)((u32)VirtualKey::N0 + (u32)keyId - 0x30);
	}

	if(keyId >= VK_NUMPAD0 && keyId <= VK_NUMPAD9)
	{
		return (VirtualKey::Id)((u32)VirtualKey::Numpad0 + (u32)keyId - VK_NUMPAD0);
	}

	if(keyId >= VK_F1 && keyId <= VK_F12)
	{
		return (VirtualKey::Id)((u32)VirtualKey::F1 + (u32)keyId - VK_F1);
	}

	switch(keyId)
	{
		case VK_LEFT: return VirtualKey::LeftArrow;
		case VK_RIGHT: return VirtualKey::RightArrow;
		case VK_UP: return VirtualKey::UpArrow;
		case VK_DOWN: return VirtualKey::DownArrow;
		case VK_PRIOR: return VirtualKey::PageUp;
		case VK_NEXT: return VirtualKey::PageDown;
		case VK_HOME: return VirtualKey::Home;
		case VK_END: return VirtualKey::End;
		case VK_SPACE: return VirtualKey::Space;
		case VK_RETURN: return VirtualKey::Return;
		case VK_ESCAPE: return VirtualKey::Escape;
	}

	return VirtualKey::Unknown;
}


struct Platform
{
	Platform(HINSTANCE instance)
	{
		_instance = instance;
		ZeroMemory(_textures, sizeof(_textures));
	}

	~Platform()
	{
		Destroy();
	}

	void MainLoop()
	{
		SetTimer(_window, TIMER_MAIN_ID, 25, nullptr); // 40 Hz max. refresh rate.

		MSG message;
		for(;;)
		{
			//  0: WM_QUIT was received
			// -1: an error occurred
			const BOOL result = GetMessageW(&message, _window, 0, 0);
			if(result == -1 || result == 0)
			{
				break;
			}

			TranslateMessage(&message);
			DispatchMessageW(&message);
		}
	}

	void ReDraw()
	{
		if(_redrawing)
		{
			return;
		}

		_redrawing = true;

		_deviceContext->ClearRenderTargetView(_renderTargetView, ViewerClearColor);
		_deviceContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		nvgBeginFrame(_nvgContext, _windowClientWidth, _windowClientHeight, 1.0f);

		_renderParams.NVGContext = _nvgContext;
		_renderParams.ClientWidth = _windowClientWidth;
		_renderParams.ClientHeight = _windowClientHeight;
		_viewer->Render(_renderParams);

		nvgEndFrame(_nvgContext);

		_swapChain->Present(0, 0);

		_redrawing = false;
	}

	bool Init()
	{
		WNDCLASSW windowClass;
		ZeroMemory(&windowClass, sizeof(windowClass));
		windowClass.lpfnWndProc = &MainWindowProc;
		windowClass.hInstance = _instance;
		windowClass.lpszClassName = WINDOW_CLASS_NAME;
		windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
		windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		if(RegisterClassW(&windowClass) == 0)
		{
			return false;
		}

		// @NOTE: For borderless, need to specify width and height manually because the default will be 0.
		//const DWORD style = WS_POPUP | WS_CLIPCHILDREN; // borderless
		//const DWORD styleEx = WS_EX_APPWINDOW; // borderless
		const DWORD style = WS_OVERLAPPEDWINDOW;
		const DWORD styleEx = WS_EX_ACCEPTFILES;
		const HWND window = CreateWindowExW(styleEx, windowClass.lpszClassName, L"UDT 2D Viewer", style,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, _instance, nullptr);
		if(window == nullptr)
		{
			return false;
		}
		_window = window;
		SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)this);

		// Title bar, task bar, ...
		const HANDLE iconSmall = LoadImageW(GetModuleHandleW(nullptr), L"MAINICON", IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
		if(iconSmall)
		{
			SendMessage(_window, WM_SETICON, ICON_SMALL, (LPARAM)iconSmall);
		}

		// Alt-tab screen, ...
		const HANDLE iconBig = LoadImageW(GetModuleHandleW(nullptr), L"MAINICON", IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
		if(iconBig)
		{
			SendMessage(_window, WM_SETICON, ICON_BIG, (LPARAM)iconBig);
		}

		UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if defined(DEBUG) || defined(_DEBUG)  
		flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		InitSwapChainDesc(swapChainDesc, window, _windowClientWidth, _windowClientHeight);
		IDXGISwapChain* swapChain = nullptr;
		ID3D11Device* device = nullptr;
		ID3D11DeviceContext* deviceContext = nullptr;
		if(FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0, 
			D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, nullptr, &deviceContext)))
		{
			return false;
		}
		_swapChain = swapChain;
		_device = device;
		_deviceContext = deviceContext;

		RECT rect;
		GetClientRect(window, &rect);
		if(!ResizeBuffers((UINT)(rect.right - rect.left), (UINT)(rect.bottom - rect.top)))
		{
			return false;
		}
		
		NVGcontext* const nvgContext = nvgCreateD3D11(device, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
		if(nvgContext == nullptr)
		{
			return false;
		}
		_nvgContext = nvgContext;

		ZeroMemory(&_sharedReadOnly, sizeof(_sharedReadOnly));
		ZeroMemory(&_sharedReadWrite, sizeof(_sharedReadWrite));
		_sharedReadOnly.NVGContext = nvgContext;

		deviceContext->ClearRenderTargetView(_renderTargetView, ViewerClearColor);
		deviceContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
		swapChain->Present(0, 0);
		ShowWindow(window, SW_SHOW);
		UpdateWindow(window);

		return true;
	}

	bool ResizeBuffers(UINT newWidth, UINT newHeight)
	{
		if(newWidth == _windowClientWidth &&
		   newHeight == _windowClientHeight)
		{
			return true;
		}
		_windowClientWidth = newWidth;
		_windowClientHeight = newHeight;

		COM_RELEASE(_renderTargetView);
		COM_RELEASE(_depthStencilView);
		COM_RELEASE(_depthStencilBuffer);

		if(FAILED(_swapChain->ResizeBuffers(1, _windowClientWidth, _windowClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0)))
		{
			return false;
		}

		ID3D11Texture2D* backBuffer = nullptr;
		if(FAILED(_swapChain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backBuffer)))
		{
			return false;
		}

		ID3D11RenderTargetView* renderTargetView = nullptr;
		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		if(FAILED(_device->CreateRenderTargetView(backBuffer, 0, &renderTargetView)) ||
		   FAILED(_swapChain->GetDesc(&swapChainDesc)))
		{
			return false;
		}
		_renderTargetView = renderTargetView;
		COM_RELEASE(backBuffer);

		D3D11_TEXTURE2D_DESC depthStencilTexDesc;
		ZeroMemory(&depthStencilTexDesc, sizeof(depthStencilTexDesc));
		depthStencilTexDesc.Width = _windowClientWidth;
		depthStencilTexDesc.Height = _windowClientHeight;
		depthStencilTexDesc.MipLevels = 1;
		depthStencilTexDesc.ArraySize = 1;
		depthStencilTexDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilTexDesc.SampleDesc.Count = swapChainDesc.SampleDesc.Count;
		depthStencilTexDesc.SampleDesc.Quality = swapChainDesc.SampleDesc.Quality;
		depthStencilTexDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilTexDesc.CPUAccessFlags = 0;
		depthStencilTexDesc.MiscFlags = 0;

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
		ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
		depthStencilViewDesc.Format = depthStencilTexDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS; // D3D11_DSV_DIMENSION_TEXTURE2D if without MSAA.
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		ID3D11Texture2D* depthStencilBuffer = nullptr;
		ID3D11DepthStencilView* depthStencilView = nullptr;
		if(FAILED(_device->CreateTexture2D(&depthStencilTexDesc, 0, &depthStencilBuffer)) ||
		   FAILED(_device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &depthStencilView)))
		{
			return false;
		}
		_depthStencilBuffer = depthStencilBuffer;
		_depthStencilView = depthStencilView;
		_deviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = (FLOAT)_windowClientWidth;
		viewport.Height = (FLOAT)_windowClientHeight;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		_deviceContext->RSSetViewports(1, &viewport);

		return true;
	}

	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
	{
#define ResizeBuffersFromWindowProc(w, h) do { if(!ResizeBuffers(w, h)) { SendMessageW(_window, WM_CLOSE, 0, 0); return 0; } } while((void)0, 0)

		switch(message)
		{
			case WM_CLOSE: // ALT+F4 or close button clicked.
				DestroyWindow();
				return 0;

			case WM_DESTROY: // Window no longer visible but still alive.
				PostQuitMessage(0);
				return 0;

			case WM_QUIT: // Window already destroyed.
				_isRunning = false;
				return 0;

			case WM_SIZE:
				if(_device != nullptr)
				{
					if(wParam == SIZE_MINIMIZED)
					{
						_windowState = WindowState::Minimized;
					}
					else if(wParam == SIZE_MAXIMIZED)
					{
						_windowState = WindowState::Maximized;
						ResizeBuffersFromWindowProc((UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
						ReDraw();
					}
					else if(wParam == SIZE_RESTORED)
					{
						_windowState = WindowState::Normal;
						ResizeBuffersFromWindowProc((UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
						ReDraw();
					}
				}
				return 0;

			case WM_ENTERSIZEMOVE: // Resize bar grabbed.
				_resizing = true;
				return 0;

			case WM_EXITSIZEMOVE: // Resize bar released.
			{
				_resizing = false;
				RECT rect;
				GetClientRect(_window, &rect);
				ResizeBuffersFromWindowProc((UINT)(rect.right - rect.left), (UINT)(rect.bottom - rect.top));
				ReDraw();
				return 0;
			}

			case WM_GETMINMAXINFO:
				((MINMAXINFO*)lParam)->ptMinTrackSize.x = 640;
				((MINMAXINFO*)lParam)->ptMinTrackSize.y = 480;
				return 0;

			case WM_TIMER:
				if(wParam == TIMER_MAIN_ID &&
				   _windowState != WindowState::Minimized)
				{
					ReDraw();
				}
				return 0;

			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
			{
				Event event;
				ZeroMemory(&event, sizeof(event));
				event.Type = EventType::MouseButtonDown;
				event.MouseButtonId = GetButtonId(message);
				event.CursorPos[0] = (s32)GET_X_LPARAM(lParam);
				event.CursorPos[1] = (s32)GET_Y_LPARAM(lParam);
				_viewer->ProcessEvent(event);
				return 0;
			}

			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
			{
				Event event;
				ZeroMemory(&event, sizeof(event));
				event.Type = EventType::MouseButtonUp;
				event.MouseButtonId = GetButtonId(message);
				event.CursorPos[0] = (s32)GET_X_LPARAM(lParam);
				event.CursorPos[1] = (s32)GET_Y_LPARAM(lParam);
				_viewer->ProcessEvent(event);
				return 0;
			}

			case WM_MOUSEMOVE:
			{
				Event event;
				ZeroMemory(&event, sizeof(event));
				event.Type = EventType::MouseMove;
				event.CursorPos[0] = (s32)GET_X_LPARAM(lParam);
				event.CursorPos[1] = (s32)GET_Y_LPARAM(lParam);
				_viewer->ProcessEvent(event);
				return 0;
			}

			case WM_NCMOUSEMOVE:
			{
				POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				ScreenToClient(_window, &point);
				Event event;
				ZeroMemory(&event, sizeof(event));
				event.Type = EventType::MouseMoveNC;
				event.CursorPos[0] = (s32)point.x;
				event.CursorPos[1] = (s32)point.y;
				_viewer->ProcessEvent(event);
				return 0;
			}

			case WM_MOUSEWHEEL:
			{
				POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				ScreenToClient(_window, &point);
				Event event;
				ZeroMemory(&event, sizeof(event));
				event.Type = EventType::MouseScroll;
				event.MouseButtonId = MouseButton::Middle;
				event.Scroll = (s32)GET_WHEEL_DELTA_WPARAM(wParam);
				event.CursorPos[0] = (s32)point.x;
				event.CursorPos[1] = (s32)point.y;
				_viewer->ProcessEvent(event);
				return 0;
			}

			case WM_KEYDOWN:
			{
				Event event;
				ZeroMemory(&event, sizeof(event));
				event.Type = IsBitSet(&lParam, 30) ? EventType::KeyDownRepeat : EventType::KeyDown;
				event.VirtualKeyId = GetKeyId(wParam);
				_viewer->ProcessEvent(event);
				return 0;
			}

			case WM_KEYUP:
			{
				Event event;
				ZeroMemory(&event, sizeof(event));
				event.Type = EventType::KeyUp;
				event.VirtualKeyId = GetKeyId(wParam);
				_viewer->ProcessEvent(event);
				return 0;
			}

			case WM_DROPFILES:
			{
				const HDROP drop = (HDROP)wParam;
				const UINT fileCount = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
				if(fileCount > 0)
				{
					wchar_t filePathBuffer[1024];
					udtVMLinearAllocator& alloc = udtThreadLocalAllocators::GetTempAllocator();
					udtVMScopedStackAllocator allocScope(alloc);
					u32* const filePathOffsets = (u32*)alloc.AllocateAndGetAddress((uptr)fileCount * (uptr)sizeof(u32));
					const char** const filePaths = (const char**)alloc.AllocateAndGetAddress((uptr)fileCount * (uptr)sizeof(const char*));
					for(UINT i = 0; i < fileCount; ++i)
					{
						DragQueryFileW(drop, i, filePathBuffer, 1024 - 1);
						filePathOffsets[i] = udtString::NewFromUTF16(alloc, filePathBuffer).GetOffset();
					}
					for(UINT i = 0; i < fileCount; ++i)
					{
						filePaths[i] = alloc.GetStringAt(filePathOffsets[i]);
					}
					Event event;
					ZeroMemory(&event, sizeof(event));
					event.Type = EventType::FilesDropped;
					event.DroppedFileCount = (u32)fileCount;
					event.DroppedFilePaths = filePaths;
					_viewer->ProcessEvent(event);
				}
				DragFinish(drop);
				return 0;
			}

			default:
				return DefWindowProcW(_window, message, wParam, lParam);
		}

#undef ResizeBuffersFromWindowProc
	}

	void SetPaused(bool paused)
	{
		_paused = paused;
		Event event;
		ZeroMemory(&event, sizeof(event));
		event.Type = paused ? EventType::Paused : EventType::Unpaused;
		_viewer->ProcessEvent(event);
	}

	void Destroy()
	{
		COM_RELEASE(_renderTargetView);
		COM_RELEASE(_depthStencilView);
		COM_RELEASE(_depthStencilBuffer);
		COM_RELEASE(_swapChain);
		COM_RELEASE(_renderTargetView);

		if(_deviceContext != nullptr)
		{
			_deviceContext->ClearState();
			_deviceContext->Release();
		}

		COM_RELEASE(_device);
		
		DestroyWindow();
	}

	void DestroyWindow()
	{
		if(_window != nullptr)
		{
			::DestroyWindow(_window);
			_window = nullptr;
		}
		
		if(_classRegistered)
		{
			UnregisterClassW(WINDOW_CLASS_NAME, _instance);
			_classRegistered = false;
		}
	}

	struct TextureSlot
	{
		ID3D11Texture2D* Texture;
		ID3D11ShaderResourceView* ShaderResourceView;
		u8 Used;
	};

	PlatformReadOnly _sharedReadOnly;
	PlatformReadWrite _sharedReadWrite;
	RenderParams _renderParams;
	TextureSlot _textures[16];
	IDXGISwapChain* _swapChain = nullptr;
	ID3D11Device* _device = nullptr;
	ID3D11DeviceContext* _deviceContext = nullptr;
	ID3D11RenderTargetView* _renderTargetView = nullptr;
	ID3D11Texture2D* _depthStencilBuffer = nullptr;
	ID3D11DepthStencilView* _depthStencilView = nullptr;
	Viewer* _viewer = nullptr;
	HINSTANCE _instance = nullptr;
	NVGcontext* _nvgContext = nullptr;
	HWND _window = nullptr;
	WindowState::Id _windowState = WindowState::Normal;
	UINT _windowClientWidth = 0;
	UINT _windowClientHeight = 0;
	bool _resizing = false;
	bool _classRegistered = false;
	bool _isRunning = true;
	bool _paused = false;
	bool _redrawing = false;

private:
	void InitSwapChainDesc(DXGI_SWAP_CHAIN_DESC& swapChainDesc, HWND window, UINT width, UINT height)
	{
		// @NOTE: for no AA, sample count = 1 and sample quality = 0
		// @NOTE: D3D11 requires hardware to support 4x and 8x MSAA with D3D11_STANDARD_MULTISAMPLE_PATTERN.
		// So, no need to call ID3D11Device::CheckMultisampleQualityLevels for that.
		ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
		swapChainDesc.BufferCount = 1;
		swapChainDesc.BufferDesc.Width = width;
		swapChainDesc.BufferDesc.Height = height;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = window;
		swapChainDesc.SampleDesc.Count = 4;
		swapChainDesc.SampleDesc.Quality = UINT(D3D11_STANDARD_MULTISAMPLE_PATTERN);
		swapChainDesc.Windowed = TRUE;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;
	}
};

static LRESULT CALLBACK MainWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	Platform* const platform = (Platform*)GetWindowLongPtrW(window, GWLP_USERDATA);
	if(platform == nullptr)
	{
		return DefWindowProcW(window, message, wParam, lParam);
	}

	return platform->WindowProc(message, wParam, lParam);
}

void Platform_RequestQuit(Platform& platform)
{
	SendMessageW(platform._window, WM_CLOSE, 0, 0);
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

void Platform_SetCursorCapture(Platform& platform, bool enabled)
{
	if(enabled)
	{
		SetCapture(platform._window);
	}
	else
	{
		ReleaseCapture();
	}
}

void Platform_NVGBeginFrame(Platform& platform)
{
	nvgBeginFrame(platform._nvgContext, platform._windowClientWidth, platform._windowClientHeight, 1.0f);
}

void Platform_NVGEndFrame(Platform& platform)
{
	nvgEndFrame(platform._nvgContext);
}

void Platform_ToggleMaximized(Platform& platform)
{
	ShowWindow(platform._window, platform._windowState != WindowState::Maximized ? SW_SHOWMAXIMIZED : SW_NORMAL);
}

void Platform_GetCursorPosition(Platform& platform, s32& x, s32& y)
{
	POINT point;
	GetCursorPos(&point);
	ScreenToClient(platform._window, &point);
	x = (s32)point.x;
	y = (s32)point.y;
}

#include "windows.hpp"

static void udtCrashHandler(const char* message)
{
	Platform_FatalError(message);
}

static int Main(HINSTANCE instance)
{
	udtSetCrashHandler(&udtCrashHandler);
	udtInitLibrary();

	Log::Init();
	
	Platform platform(instance);
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

// @NOTE: the command line argument of wWinMain does *not* contain the path to the executable.
// To get that, we call GetCommandLineW instead.
int CALLBACK wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int)
{
	if(IsDebuggerPresent())
	{
		return Main(instance);
	}

	__try
	{
		return Main(instance);
	}
	__except(Win32ExceptionFilter())
	{
		return Win32ExceptionHandler((int)GetExceptionCode());
	}
}
