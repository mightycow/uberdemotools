#pragma once


#include "uberdemotools.h"
#include "array.hpp"
#include "nanovg/nanovg.h"


#define  InvalidTextureId  (-666)

struct EventType
{
	enum Id
	{
		Paused,
		Unpaused,
		MouseMove,
		MouseMoveNC, // outside of window client area
		MouseButtonDown,
		MouseButtonUp,
		MouseScroll,
		FilesDropped,
		KeyDown,
		KeyUp,
		KeyDownRepeat,
		Count
	};
};

struct MouseButton
{
	enum Id
	{
		Left,
		Middle,
		Right,
		Count,
		Unknown
	};
};

struct VirtualKey
{
	enum Id
	{
		A,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,
		N0,
		N1,
		N2,
		N3,
		N4,
		N5,
		N6,
		N7,
		N8,
		N9,
		Numpad0,
		Numpad1,
		Numpad2,
		Numpad3,
		Numpad4,
		Numpad5,
		Numpad6,
		Numpad7,
		Numpad8,
		Numpad9,
		F1,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		LeftArrow,
		RightArrow,
		UpArrow,
		DownArrow,
		PageUp,
		PageDown,
		Home,
		End,
		Space,
		Return,
		Escape,
		Count,
		Unknown
	};
};

struct Event
{
	s32 CursorPos[2];
	s32 Scroll;
	u32 DroppedFileCount;
	EventType::Id Type;
	MouseButton::Id MouseButtonId;
	VirtualKey::Id VirtualKeyId;
	const char** DroppedFilePaths; // Only valid during the call to ProcessEvent.
};

struct RenderParams
{
	NVGcontext* NVGContext; // In
	u32 ClientWidth;        // In
	u32 ClientHeight;       // In
};
