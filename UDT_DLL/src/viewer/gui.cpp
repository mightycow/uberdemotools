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
	_radius = 0.0f;
	_progress = 0.0f;
	_progressChanged = false;
	_draggingSlider = false;
	_dragStarted = false;
	_dragEnded = false;
}

DemoProgressBar::~DemoProgressBar()
{
}

void DemoProgressBar::SetRadius(f32 r)
{
	_radius = r;
}

void DemoProgressBar::SetProgress(f32 progress)
{
	_progress = progress;
}

bool DemoProgressBar::HasProgressChanged(f32& progress)
{
	const bool changed = _progressChanged;
	progress = _progress;
	_progressChanged = false;
	return changed;
}

bool DemoProgressBar::HasDragJustStarted()
{
	const bool started = _dragStarted;
	_dragStarted = false;
	return started;
}

bool DemoProgressBar::HasDragJustEnded()
{
	const bool ended = _dragEnded;
	_dragEnded = false;
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
	if(button == MouseButton::Left && _draggingSlider)
	{
		ChangeProgress(x, y);
		SetDragging(false);
	}
}

void DemoProgressBar::MouseMove(s32 x, s32 y)
{
	Widget::MouseMove(x, y);
	if(_draggingSlider)
	{
		ChangeProgress(x, y);
	}
}

void DemoProgressBar::MouseMoveNC(s32 x, s32 y)
{
	Widget::MouseMoveNC(x, y);
	if(_draggingSlider)
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
	_progress = udt_clamp((pos - min) / (max - min), 0.0f, 1.0f);
	_progressChanged = true;
}

void DemoProgressBar::SetDragging(bool dragging)
{
	if(_draggingSlider && !dragging)
	{
		_dragEnded = true;
	}
	else if(!_draggingSlider && dragging)
	{
		_dragStarted = true;
	}

	_draggingSlider = dragging;
}

void DemoProgressBar::Draw(NVGcontext* nvgContext)
{
	BNDwidgetState state = BND_DEFAULT;
	if(_draggingSlider)
	{
		state = BND_ACTIVE;
	}
	else if(Hovered)
	{
		state = BND_HOVER;
	}

	const f32 w = Dim[0] + 2.0f * _radius;
	const f32 s = (f32)BND_SCROLLBAR_HEIGHT / w;
	bndScrollBar(nvgContext, Pos[0] - _radius, Pos[1], w, BND_SCROLLBAR_HEIGHT, state, _progress, s);
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
	Widget::MouseMove(x, y);
	if(Pressed && !IsPointInside((f32)x, (f32)y))
	{
		Pressed = false;
	}
}

void Button::MouseMoveNC(s32 x, s32 y)
{
	Widget::MouseMoveNC(x, y);
	Pressed = false;
}

bool Button::WasClicked()
{
	const bool clicked = Clicked;
	Clicked = false;
	return clicked;
}

BNDwidgetState Button::GetState() const
{
	if(Pressed)
	{
		return BND_ACTIVE;
	}

	if(Hovered)
	{
		return BND_HOVER;
	}

	return BND_DEFAULT;
}

PlayPauseButton::PlayPauseButton()
{
	_timer = nullptr;
}

PlayPauseButton::~PlayPauseButton()
{
}

void PlayPauseButton::SetTimerPtr(const udtTimer* timer)
{
	_timer = timer;
}

void PlayPauseButton::Draw(NVGcontext* nvgContext)
{
	const int icon = _timer->IsRunning() ? BND_ICON_PAUSE : BND_ICON_PLAY;
	bndToolButton(nvgContext, Pos[0], Pos[1], BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_RIGHT, GetState(), icon, NULL);
}

void StopButton::Draw(NVGcontext* nvgContext)
{
	bndToolButton(nvgContext, Pos[0], Pos[1], BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_LEFT | BND_CORNER_RIGHT, GetState(), BND_ICONID(0, 1), NULL);
}

ReverseButton::ReverseButton()
{
	_reversed = nullptr;
}

ReverseButton::~ReverseButton()
{
}

void ReverseButton::SetReversedPtr(const bool* reversed)
{
	_reversed = reversed;
}

void ReverseButton::Draw(NVGcontext* nvgContext)
{
	const int icon = *_reversed ? BND_ICONID(16, 2) : BND_ICONID(15, 2);
	bndToolButton(nvgContext, Pos[0], Pos[1], BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_LEFT, GetState(), icon, NULL);
}

CheckBox::CheckBox()
{
	_text = nullptr;
	_active = false;
	_clicked = false;
}

CheckBox::~CheckBox()
{
}

void CheckBox::SetText(const char* text)
{
	_text = text;
}

void CheckBox::SetActivePtr(bool* active)
{
	_active = active;
}

bool CheckBox::WasClicked()
{
	const bool clicked = _clicked;
	_clicked = false;
	return clicked;
}

void CheckBox::MouseButtonDown(s32 x, s32 y, MouseButton::Id button)
{
	if(button == MouseButton::Left && IsPointInside((f32)x, (f32)y))
	{
		_clicked = true;
		*_active = !*_active;
	}
}

void CheckBox::MouseButtonUp(s32, s32, MouseButton::Id)
{
}

void CheckBox::MouseMove(s32 x, s32 y)
{
	Widget::MouseMove(x, y);
}

void CheckBox::MouseMoveNC(s32 x, s32 y)
{
	Widget::MouseMoveNC(x, y);
}

void CheckBox::Draw(NVGcontext* nvgContext)
{
	BNDwidgetState state = BND_DEFAULT;
	if(*_active)
	{
		state = BND_ACTIVE;
	}
	else if(Hovered)
	{
		state = BND_HOVER;
	}

	bndOptionButton(nvgContext, Pos[0], Pos[1], Dim[0], BND_WIDGET_HEIGHT, state, _text);
}

RadioButton::RadioButton()
{
	_text = nullptr;
	_active = false;
	_clicked = false;
}

RadioButton::~RadioButton()
{
}

void RadioButton::SetText(const char* text)
{
	_text = text;
}

void RadioButton::SetActive(bool active)
{
	_active = active;
}

bool RadioButton::IsActive() const
{
	return _active;
}

bool RadioButton::WasClicked()
{
	const bool clicked = _clicked;
	_clicked = false;
	return clicked;
}

void RadioButton::MouseButtonDown(s32 x, s32 y, MouseButton::Id button)
{
	if(button == MouseButton::Left && IsPointInside((f32)x, (f32)y))
	{
		_clicked = true;
		_active = true;
	}
}

void RadioButton::MouseButtonUp(s32, s32, MouseButton::Id)
{
}

void RadioButton::MouseMove(s32 x, s32 y)
{
	Widget::MouseMove(x, y);
}

void RadioButton::MouseMoveNC(s32 x, s32 y)
{
	Widget::MouseMoveNC(x, y);
}

void RadioButton::Draw(NVGcontext* nvgContext)
{
	BNDwidgetState state = BND_DEFAULT;
	if(_active)
	{
		state = BND_ACTIVE;
	}
	else if(Hovered)
	{
		state = BND_HOVER;
	}

	bndRadioButton(nvgContext, Pos[0], Pos[1], Dim[0], BND_WIDGET_HEIGHT, BND_CORNER_NONE, state, -1, _text);
}

RadioGroup::RadioGroup()
{
	_radioButtons.Init(1 << 16, "RadioGroup::RadioButtonsArray");
	_selectedIndex = 0;
	_selectionChanged = false;
}

RadioGroup::~RadioGroup()
{
}

bool RadioGroup::HasSelectionChanged()
{
	const bool changed = _selectionChanged;
	_selectionChanged = false;
	return changed;
}

u32 RadioGroup::GetSelectedIndex() const
{
	return _selectedIndex;
}

void RadioGroup::AddRadioButton(RadioButton* radioButton)
{
	radioButton->SetActive(_radioButtons.GetSize() == 0);
	_radioButtons.Add(radioButton);
}

void RadioGroup::RemoveRadioButton(RadioButton* radioButton)
{
	for(u32 i = 0, count = _radioButtons.GetSize(); i < count; ++i)
	{
		if(_radioButtons[i] == radioButton)
		{
			_radioButtons.RemoveUnordered(i);
			break;
		}
	}
}

void RadioGroup::MouseButtonDown(s32 x, s32 y, MouseButton::Id button)
{
	for(u32 i = 0, count = _radioButtons.GetSize(); i < count; ++i)
	{
		_radioButtons[i]->MouseButtonDown(x, y, button);
		if(_radioButtons[i]->WasClicked())
		{
			_selectionChanged = true;
			_selectedIndex = i;
		}
	}

	if(!_selectionChanged)
	{
		return;
	}

	for(u32 i = 0, count = _radioButtons.GetSize(); i < count; ++i)
	{
		_radioButtons[i]->SetActive(i == _selectedIndex);
	}
}

void RadioGroup::MouseButtonUp(s32 x, s32 y, MouseButton::Id button)
{
	for(u32 i = 0, count = _radioButtons.GetSize(); i < count; ++i)
	{
		_radioButtons[i]->MouseButtonUp(x, y, button);
	}
}

void RadioGroup::MouseMove(s32 x, s32 y)
{
	for(u32 i = 0, count = _radioButtons.GetSize(); i < count; ++i)
	{
		_radioButtons[i]->MouseMove(x, y);
	}
}

void RadioGroup::MouseMoveNC(s32 x, s32 y)
{
	for(u32 i = 0, count = _radioButtons.GetSize(); i < count; ++i)
	{
		_radioButtons[i]->MouseMoveNC(x, y);
	}
}

void RadioGroup::MouseScroll(s32 x, s32 y, s32 scroll)
{
	for(u32 i = 0, count = _radioButtons.GetSize(); i < count; ++i)
	{
		_radioButtons[i]->MouseScroll(x, y, scroll);
	}
}

void RadioGroup::Draw(NVGcontext* nvgContext)
{
	for(u32 i = 0, count = _radioButtons.GetSize(); i < count; ++i)
	{
		_radioButtons[i]->Draw(nvgContext);
	}
}
