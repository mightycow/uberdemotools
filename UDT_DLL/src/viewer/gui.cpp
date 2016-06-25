#include "gui.hpp"
#include "utils.hpp"
#include "platform.hpp"
#include "nanovg_drawing.hpp"

#include <math.h>


bool IsPointInsideDisk(f32 px, f32 py, f32 dx, f32 dy, f32 dr)
{
	const f32 x = px - dx;
	const f32 y = py - dy;

	return (x*x + y*y) <= (dr*dr);
}

bool IsPointInsideRectangle(f32 px, f32 py, f32 rx, f32 ry, f32 rw, f32 rh)
{
	return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}


Widget::Widget()
{
	Pos[0] = 0.0f;
	Pos[1] = 0.0f;
	Dim[0] = 0.0f;
	Dim[1] = 0.0f;
	Hovered = false;
	Enabled = true;
}

Widget::~Widget()
{
}

void Widget::SetRect(f32 x, f32 y, f32 w, f32 h)
{
	Pos[0] = x;
	Pos[1] = y;
	Dim[0] = w;
	Dim[1] = h;
}

void Widget::GetRect(f32& x, f32& y, f32& w, f32& h)
{
	x = Pos[0];
	y = Pos[1];
	w = Dim[0];
	h = Dim[1];
}

void Widget::MouseButtonDown(s32, s32, MouseButton::Id)
{
}

void Widget::MouseButtonUp(s32, s32, MouseButton::Id)
{
}

void Widget::MouseMove(s32 x, s32 y)
{
	Hovered = IsPointInside((f32)x, (f32)y);
}

void Widget::MouseMoveNC(s32, s32)
{
	Hovered = false;
}

void Widget::MouseScroll(s32, s32, s32)
{
}

void Widget::Draw(NVGcontext*)
{
}

bool Widget::IsPointInside(f32 x, f32 y)
{
	return IsPointInsideRectangle(x, y, Pos[0], Pos[1], Dim[0], Dim[1]);
}

WidgetGroup::WidgetGroup()
{
	_widgets.Init(1 << 16, "WidgetGroup::WidgetsArray");
}

WidgetGroup::~WidgetGroup()
{
}

void WidgetGroup::AddWidget(Widget* widget)
{
	_widgets.Add(widget);
}

void WidgetGroup::RemoveWidget(Widget* widget)
{
	for(u32 i = 0, count = _widgets.GetSize(); i < count; ++i)
	{
		if(_widgets[i] == widget)
		{
			_widgets.RemoveUnordered(i);
			break;
		}
	}
}

void WidgetGroup::MouseButtonDown(s32 x, s32 y, MouseButton::Id button)
{
	for(u32 i = 0, count = _widgets.GetSize(); i < count; ++i)
	{
		_widgets[i]->MouseButtonDown(x, y, button);
	}
}

void WidgetGroup::MouseButtonUp(s32 x, s32 y, MouseButton::Id button)
{
	for(u32 i = 0, count = _widgets.GetSize(); i < count; ++i)
	{
		_widgets[i]->MouseButtonUp(x, y, button);
	}
}

void WidgetGroup::MouseMove(s32 x, s32 y)
{
	for(u32 i = 0, count = _widgets.GetSize(); i < count; ++i)
	{
		_widgets[i]->MouseMove(x, y);
	}
}

void WidgetGroup::MouseMoveNC(s32 x, s32 y)
{
	for(u32 i = 0, count = _widgets.GetSize(); i < count; ++i)
	{
		_widgets[i]->MouseMoveNC(x, y);
	}
}

void WidgetGroup::MouseScroll(s32 x, s32 y, s32 scroll)
{
	for(u32 i = 0, count = _widgets.GetSize(); i < count; ++i)
	{
		_widgets[i]->MouseScroll(x, y, scroll);
	}
}

void WidgetGroup::Draw(NVGcontext* nvgContext)
{
	for(u32 i = 0, count = _widgets.GetSize(); i < count; ++i)
	{
		_widgets[i]->Draw(nvgContext);
	}
}

DemoProgressBar::DemoProgressBar()
{
	Pos[0] = 0.0f;
	Pos[1] = 0.0f;
	Dim[0] = 0.0f;
	Dim[1] = 0.0f;
	Radius = 0.0f;
	Progress = 0.0f;
	ProgressChanged = false;
	DraggingSlider = false;
	DragStarted = false;
	DragEnded = false;
}

DemoProgressBar::~DemoProgressBar()
{
}

void DemoProgressBar::SetRadius(f32 r)
{
	Radius = r;
}

void DemoProgressBar::SetProgress(f32 progress)
{
	Progress = progress;
}

bool DemoProgressBar::HasProgressChanged(f32& progress)
{
	const bool changed = ProgressChanged;
	progress = Progress;
	ProgressChanged = false;
	return changed;
}

bool DemoProgressBar::HasDragJustStarted()
{
	const bool started = DragStarted;
	DragStarted = false;
	return started;
}

bool DemoProgressBar::HasDragJustEnded()
{
	const bool ended = DragEnded;
	DragEnded = false;
	return ended;
}

void DemoProgressBar::MouseButtonDown(s32 x, s32 y, MouseButton::Id button)
{
	if(button == MouseButton::Left && IsPointInside((f32)x, (f32)y))
	{
		SetDragging(true);
		ChangeProgress(x, y);
	}
}

void DemoProgressBar::MouseButtonUp(s32 x, s32 y, MouseButton::Id button)
{
	if(button == MouseButton::Left && DraggingSlider)
	{
		ChangeProgress(x, y);
		SetDragging(false);
	}
}

void DemoProgressBar::MouseMove(s32 x, s32 y)
{
	Widget::MouseMove(x, y);
	if(DraggingSlider)
	{
		ChangeProgress(x, y);
	}
}

void DemoProgressBar::MouseMoveNC(s32 x, s32 y)
{
	Widget::MouseMoveNC(x, y);
	if(DraggingSlider)
	{
		ChangeProgress(x, y);
		SetDragging(false);
	}
}

void DemoProgressBar::ChangeProgress(s32 x, s32 y)
{
	const f32 cp[2] = { (f32)x, (f32)y };
	const f32 min = Pos[0];
	const f32 max = Pos[0] + Dim[0];
	const f32 pos = cp[0];
	Progress = udt_clamp((pos - min) / (max - min), 0.0f, 1.0f);
	ProgressChanged = true;
}

void DemoProgressBar::SetDragging(bool dragging)
{
	if(DraggingSlider && !dragging)
	{
		DragEnded = true;
	}
	else if(!DraggingSlider && dragging)
	{
		DragStarted = true;
	}

	DraggingSlider = dragging;
}

void DemoProgressBar::Draw(NVGcontext* nvgContext)
{
	DrawProgressSlider(nvgContext, Pos[0] - Radius, Pos[1], Dim[0] + 2.0f * Radius, Dim[1], Radius, Progress);
}

Button::Button()
{
	Pressed = false;
	Clicked = false;
}

Button::~Button()
{
}

void Button::MouseButtonDown(s32 x, s32 y, MouseButton::Id button)
{
	if(button == MouseButton::Left && IsPointInside((f32)x, (f32)y))
	{
		Pressed = true;
	}
}

void Button::MouseButtonUp(s32, s32, MouseButton::Id button)
{
	if(button == MouseButton::Left && Pressed)
	{
		Clicked = true;
	}
	Pressed = false;
}

void Button::MouseMove(s32 x, s32 y)
{
	if(Pressed && !IsPointInside((f32)x, (f32)y))
	{
		Pressed = false;
	}
}

void Button::MouseMoveNC(s32, s32)
{
	Pressed = false;
}

bool Button::WasClicked()
{
	const bool clicked = Clicked;
	Clicked = false;
	return clicked;
}

PlayPauseButton::PlayPauseButton()
{
	Playing = false;
}

PlayPauseButton::~PlayPauseButton()
{
}

void PlayPauseButton::SetPlaying(bool playing)
{
	Playing = playing;
}

static void DrawPauseBar(NVGcontext* nvgContext, f32 x, f32 y, f32 w, f32 h)
{
	nvgBeginPath(nvgContext);
	nvgRect(nvgContext, x, y, w, h);
	nvgFillPaint(nvgContext, nvgLinearGradient(nvgContext, 0.0f, y, 0.0f, y + h, nvgGrey(235), nvgGrey(180)));
	nvgFill(nvgContext);
	nvgStrokeColor(nvgContext, nvgGrey(0));
	nvgStrokeWidth(nvgContext, 1.5f);
	nvgStroke(nvgContext);
	nvgClosePath(nvgContext);
}

void PlayPauseButton::Draw(NVGcontext* nvgContext)
{
	if(Playing)
	{
		const f32 distRatioX = 0.25f;
		const f32 distRatioY = 0.25f;
		const f32 widthRatio = 0.15f;
		const f32 x0 = Pos[0] + Dim[0] * distRatioX;
		const f32 x1 = Pos[0] + Dim[0] - Dim[0] * distRatioX - Dim[0] * widthRatio;
		const f32 y = Pos[1] + Dim[1] * distRatioY;
		const f32 w = Dim[0] * widthRatio;
		const f32 h = Dim[1] * (1.0f - 2.0f * distRatioY);
		DrawPauseBar(nvgContext, x0, y, w, h);
		DrawPauseBar(nvgContext, x1, y, w, h);
	}
	else
	{
		const f32 ratioX = 0.3f;
		const f32 ratioY = 0.25f;
		const f32 x0 = Pos[0] + Dim[0] * ratioX;
		const f32 y0 = Pos[1] + Dim[1] * ratioY;
		const f32 x1 = x0;
		const f32 y1 = Pos[1] + Dim[1] - Dim[1] * ratioY;
		const f32 x2 = x0 + y1 - y0;
		const f32 y2 = Pos[1] + Dim[1] / 2.0f;
		nvgBeginPath(nvgContext);
		nvgMoveTo(nvgContext, x0, y0);
		nvgLineTo(nvgContext, x1, y1);
		nvgLineTo(nvgContext, x2, y2);
		nvgLineTo(nvgContext, x0, y0);
		nvgFillPaint(nvgContext, nvgLinearGradient(nvgContext, 0.0f, y0, 0.0f, y1, nvgGrey(235), nvgGrey(180)));
		nvgFill(nvgContext);
		nvgStrokeColor(nvgContext, nvgGrey(0));
		nvgStrokeWidth(nvgContext, 1.5f);
		nvgStroke(nvgContext);
		nvgClosePath(nvgContext);
	}
}

void StopButton::Draw(NVGcontext* nvgContext)
{
	const f32 ratio = 0.25f;
	const f32 x = Pos[0] + Dim[0] * ratio;
	const f32 y = Pos[1] + Dim[1] * ratio;
	const f32 w = Dim[0] * (1.0f - 2.0f * ratio);
	const f32 h = Dim[1] * (1.0f - 2.0f * ratio);
	nvgBeginPath(nvgContext);
	nvgRect(nvgContext, x, y, w, h);
	nvgFillPaint(nvgContext, nvgLinearGradient(nvgContext, 0.0f, y, 0.0f, y + h, nvgGrey(235), nvgGrey(180)));
	nvgFill(nvgContext);
	nvgStrokeColor(nvgContext, nvgGrey(0));
	nvgStrokeWidth(nvgContext, 1.5f);
	nvgStroke(nvgContext);
	nvgClosePath(nvgContext);
}

ReverseButton::ReverseButton()
{
	Reversed = false;
}

ReverseButton::~ReverseButton()
{
}

void ReverseButton::SetReversed(bool reversed)
{
	Reversed = reversed;
}

void ReverseButton::Draw(NVGcontext* nvgContext)
{
	f32 ratioX0 = 0.25f;
	f32 ratioX1 = 0.5f;
	f32 ratioX2 = 0.75f;
	if(Reversed)
	{
		ratioX0 = 1.0f - ratioX0;
		ratioX1 = 1.0f - ratioX1;
		ratioX2 = 1.0f - ratioX2;
	}
	const f32 ratioY0 = 0.25f;
	const f32 ratioY1 = 0.35f;
	const f32 x0 = ceilf(Pos[0] + Dim[0] * ratioX0);
	const f32 x1 = floorf(Pos[0] + Dim[0] * ratioX1);
	const f32 x2 = floorf(Pos[0] + Dim[0] * ratioX2);
	const f32 y0 = floorf(Pos[1] + Dim[1] * ratioY0);
	const f32 y1 = ceilf(Pos[1] + Dim[1] * ratioY1);
	const f32 y0m = ceilf(Pos[1] + Dim[1] * (1.0f - ratioY0));
	const f32 y1m = floorf(Pos[1] + Dim[1] * (1.0f - ratioY1));
	const f32 yc = floorf(Pos[1] + Dim[1] / 2.0f);
	nvgBeginPath(nvgContext);
	nvgMoveTo(nvgContext, x2, yc);
	nvgLineTo(nvgContext, x1, y0);
	nvgLineTo(nvgContext, x1, y1);
	nvgLineTo(nvgContext, x0, y1);
	nvgLineTo(nvgContext, x0, y1m);
	nvgLineTo(nvgContext, x1, y1m);
	nvgLineTo(nvgContext, x1, y0m);
	nvgLineTo(nvgContext, x2, yc);
	nvgFillPaint(nvgContext, nvgLinearGradient(nvgContext, 0.0f, Pos[1], 0.0f, Pos[1] + Dim[1], nvgGrey(235), nvgGrey(180)));
	nvgFill(nvgContext);
	nvgStrokeColor(nvgContext, nvgGrey(0));
	nvgStrokeWidth(nvgContext, 1.5f);
	nvgStroke(nvgContext);
	nvgClosePath(nvgContext);
}

ButtonBar::ButtonBar()
{
	_buttons.Init(1 << 16, "ButtonBar::ButtonsArray");
	_radius = 4.0f;
}

ButtonBar::~ButtonBar()
{
}

void ButtonBar::AddButton(Button* button)
{
	_buttons.Add(button);
}

bool ButtonBar::WasClicked(Button*& button)
{
	for(u32 i = 0, count = _buttons.GetSize(); i < count; ++i)
	{
		if(_buttons[i]->WasClicked())
		{
			button = _buttons[i];
			return true;
		}
	}

	return false;
}

void ButtonBar::DoLayout(f32 x, f32 y, f32 h, f32 r)
{
	SetRect(x, y, h * (f32)_buttons.GetSize(), h);
	for(u32 i = 0, count = _buttons.GetSize(); i < count; ++i)
	{
		_buttons[i]->SetRect(x, y, h, h);
		x += h;
	}
	_radius = r;
}

void ButtonBar::MouseButtonDown(s32 x, s32 y, MouseButton::Id button)
{
	for(u32 i = 0, count = _buttons.GetSize(); i < count; ++i)
	{
		_buttons[i]->MouseButtonDown(x, y, button);
	}
}

void ButtonBar::MouseButtonUp(s32 x, s32 y, MouseButton::Id button)
{
	for(u32 i = 0, count = _buttons.GetSize(); i < count; ++i)
	{
		_buttons[i]->MouseButtonUp(x, y, button);
	}
}

void ButtonBar::MouseMove(s32 x, s32 y)
{
	for(u32 i = 0, count = _buttons.GetSize(); i < count; ++i)
	{
		_buttons[i]->MouseMove(x, y);
	}
}

void ButtonBar::MouseMoveNC(s32 x, s32 y)
{
	for(u32 i = 0, count = _buttons.GetSize(); i < count; ++i)
	{
		_buttons[i]->MouseMoveNC(x, y);
	}
}

void ButtonBar::Draw(NVGcontext* nvgContext)
{
	nvgBeginPath(nvgContext);
	nvgRoundedRect(nvgContext, Pos[0], Pos[1] + 2.0f, Dim[0], Dim[1], _radius);
	nvgStrokeColor(nvgContext, nvgGrey(144));
	nvgStrokeWidth(nvgContext, 2.0f);
	nvgStroke(nvgContext);
	nvgClosePath(nvgContext);

	nvgBeginPath(nvgContext);
	nvgRoundedRect(nvgContext, Pos[0], Pos[1], Dim[0], Dim[1], _radius);
	nvgFillColor(nvgContext, nvgGrey(191));
	nvgFill(nvgContext);
	nvgStrokeColor(nvgContext, nvgGrey(73));
	nvgStrokeWidth(nvgContext, 2.0f);
	nvgStroke(nvgContext);
	nvgClosePath(nvgContext);

	for(u32 i = 0, count = _buttons.GetSize(); i < count; ++i)
	{
		_buttons[i]->Draw(nvgContext);
	}

	for(u32 i = 1, count = _buttons.GetSize(); i < count; ++i)
	{
		f32 x, y, w, h;
		_buttons[i]->GetRect(x, y, w, h);
		nvgBeginPath(nvgContext);
		nvgRect(nvgContext, x, y, 1.0f, h);
		nvgFillColor(nvgContext, nvgGrey(0));
		nvgFill(nvgContext);
		nvgClosePath(nvgContext);
	}
}

TitleBar::TitleBar()
{
	Dragging = false;
	OffsetX = 0;
	OffsetY = 0;
	CursorX = 0;
	CursorY = 0;
}

TitleBar::~TitleBar()
{
}

bool TitleBar::IsBeingDragged(s32& x, s32& y)
{
	x = OffsetX;
	y = OffsetY;
	OffsetX = 0;
	OffsetY = 0;

	return Dragging;
}

void TitleBar::MouseButtonDown(s32 x, s32 y, MouseButton::Id button)
{
	CursorX = x;
	CursorY = y;
	if(button == MouseButton::Left && IsPointInside((f32)x, (f32)y))
	{
		Dragging = true;
		OffsetX = 0;
		OffsetY = 0;
	}
}

void TitleBar::MouseButtonUp(s32 x, s32 y, MouseButton::Id button)
{
	CursorX = x;
	CursorY = y;
	if(Dragging && button == MouseButton::Left)
	{
		Dragging = false;
		OffsetX = 0;
		OffsetY = 0;
	}
}

void TitleBar::MouseMove(s32 x, s32 y)
{
	if(Dragging)
	{
		OffsetX = (s32)(x - CursorX);
		OffsetY = (s32)(y - CursorY);
	}
	CursorX = x;
	CursorY = y;
}

void TitleBar::MouseMoveNC(s32 x, s32 y)
{
	CursorX = x;
	CursorY = y;
	if(Dragging)
	{
		Dragging = false;
		OffsetX = 0;
		OffsetY = 0;
	}
}

void TitleBar::Draw(NVGcontext*)
{
}
