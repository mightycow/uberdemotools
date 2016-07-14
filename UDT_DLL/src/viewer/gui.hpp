#pragma once


#include "uberdemotools.h"
#include "shared.hpp"
#include "timer.hpp"
#include "nanovg/nanovg.h"
#include "blendish/blendish.h"


struct Widget
{
	Widget();
	virtual ~Widget();

	void SetRect(f32 x, f32 y, f32 w, f32 h);
	void GetRect(f32& x, f32& y, f32& w, f32& h);

	f32 GetX() const { return Pos[0]; }
	f32 GetY() const { return Pos[1]; }
	f32 GetWidth() const { return Dim[0]; }
	f32 GetHeight() const { return Dim[1]; }

	virtual void MouseButtonDown(s32 x, s32 y, MouseButton::Id button);
	virtual void MouseButtonUp(s32 x, s32 y, MouseButton::Id button);
	virtual void MouseMove(s32 x, s32 y);
	virtual void MouseMoveNC(s32 x, s32 y);
	virtual void MouseScroll(s32 x, s32 y, s32 scroll);
	virtual void Draw(NVGcontext* nvgContext);

	bool IsHovered() const;

protected:
	bool IsPointInside(f32 x, f32 y);

	f32 Pos[2];
	f32 Dim[2];
	bool Hovered;
};

struct WidgetGroup : public Widget
{
	WidgetGroup();
	~WidgetGroup();

	void AddWidget(Widget* widget);
	void RemoveWidget(Widget* widget);

	void MouseButtonDown(s32 x, s32 y, MouseButton::Id button) override;
	void MouseButtonUp(s32 x, s32 y, MouseButton::Id button) override;
	void MouseMove(s32 x, s32 y) override;
	void MouseMoveNC(s32 x, s32 y) override;
	void MouseScroll(s32 x, s32 y, s32 scroll) override;
	void Draw(NVGcontext* nvgContext) override;

private:
	udtVMArray<Widget*> _widgets { "WidgetGroup::WidgetsArray" };
};

struct DemoProgressSlider : public Widget
{
	DemoProgressSlider();
	~DemoProgressSlider();

	void SetRadius(f32 r);
	void SetProgress(f32 progress);
	bool HasProgressChanged(f32& progress);
	bool HasDragJustStarted();
	bool HasDragJustEnded();
	f32  GetProgress() const;

	void MouseButtonDown(s32 x, s32 y, MouseButton::Id button) override;
	void MouseButtonUp(s32 x, s32 y, MouseButton::Id button) override;
	void MouseMove(s32 x, s32 y) override;
	void MouseMoveNC(s32 x, s32 y) override;
	void Draw(NVGcontext* nvgContext) override;

private:
	void ChangeProgress(s32 x, s32 y);
	void SetDragging(bool dragging);

	f32 _radius;
	f32 _progress;
	bool _progressChanged;
	bool _draggingSlider;
	bool _dragStarted;
	bool _dragEnded;
};

struct Slider : public Widget
{
	typedef void (*ValueFormatter)(char* buffer, f32 value, f32 min, f32 max);

	Slider();
	~Slider();

	void SetFormatter(ValueFormatter formatter);
	void SetRange(f32 min, f32 max);
	void SetValuePtr(f32* value);
	void SetText(const char* text);

	void MouseButtonDown(s32 x, s32 y, MouseButton::Id button) override;
	void MouseButtonUp(s32 x, s32 y, MouseButton::Id button) override;
	void MouseMove(s32 x, s32 y) override;
	void MouseMoveNC(s32 x, s32 y) override;
	void MouseScroll(s32 x, s32 y, s32 scroll) override;
	void Draw(NVGcontext* nvgContext) override;

private:
	void ChangeProgress(s32 x, s32 y);

	ValueFormatter _formatter;
	const char* _text;
	f32* _value;
	f32 _min;
	f32 _max;
	bool _draggingSlider;
};

struct Button : public Widget
{
	Button();
	~Button();

	bool WasClicked();

	void MouseButtonDown(s32 x, s32 y, MouseButton::Id button) override;
	void MouseButtonUp(s32 x, s32 y, MouseButton::Id button) override;
	void MouseMove(s32 x, s32 y) override;
	void MouseMoveNC(s32 x, s32 y) override;

protected:
	BNDwidgetState GetState() const;

	bool Pressed;
	bool Clicked;
};

struct PlayPauseButton : public Button
{
	PlayPauseButton();
	~PlayPauseButton();

	void SetTimerPtr(const udtTimer* timer);

	void Draw(NVGcontext* nvgContext) override;

private:
	const udtTimer* _timer;
};

struct StopButton : public Button
{
	void Draw(NVGcontext* nvgContext) override;
};

struct ReverseButton : public Button
{
	ReverseButton();
	~ReverseButton();

	void SetReversedPtr(const bool* reversed);

	void Draw(NVGcontext* nvgContext) override;

private:
	const bool* _reversed;
};

struct TextButton : public Button
{
	TextButton();
	~TextButton();

	void SetRect(NVGcontext* nvgContext, f32 x, f32 y);
	void SetText(const char* text);

	void Draw(NVGcontext* nvgContext) override;

private:
	const char* _text;
};

struct CheckBox : public Widget
{
	CheckBox();
	~CheckBox();

	void SetRect(NVGcontext* nvgContext, f32 x, f32 y);
	void SetText(const char* text);
	void SetActivePtr(bool* active);
	bool WasClicked();

	void MouseButtonDown(s32 x, s32 y, MouseButton::Id button) override;
	void MouseButtonUp(s32 x, s32 y, MouseButton::Id button) override;
	void MouseMove(s32 x, s32 y) override;
	void MouseMoveNC(s32 x, s32 y) override;
	void Draw(NVGcontext* nvgContext) override;

private:
	const char* _text;
	bool* _active;
	bool _clicked;
};

struct RadioButton : public Widget
{
	RadioButton();
	~RadioButton();

	void SetCornerFlags(int flags);
	void SetText(const char* text);
	void SetActive(bool active);
	bool IsActive() const;
	bool WasClicked();

	void MouseButtonDown(s32 x, s32 y, MouseButton::Id button) override;
	void MouseButtonUp(s32 x, s32 y, MouseButton::Id button) override;
	void MouseMove(s32 x, s32 y) override;
	void MouseMoveNC(s32 x, s32 y) override;
	void Draw(NVGcontext* nvgContext) override;

private:
	const char* _text;
	int _cornerFlags;
	bool _active;
	bool _clicked;
};

struct RadioGroup : public Widget
{
	RadioGroup();
	~RadioGroup();

	bool HasSelectionChanged();
	u32  GetSelectedIndex() const;

	void AddRadioButton(RadioButton* radioButton);
	void RemoveRadioButton(RadioButton* radioButton);
	void RemoveAllRadioButtons();

	void MouseButtonDown(s32 x, s32 y, MouseButton::Id button) override;
	void MouseButtonUp(s32 x, s32 y, MouseButton::Id button) override;
	void MouseMove(s32 x, s32 y) override;
	void MouseMoveNC(s32 x, s32 y) override;
	void MouseScroll(s32 x, s32 y, s32 scroll) override;
	void Draw(NVGcontext* nvgContext) override;

private:
	udtVMArray<RadioButton*> _radioButtons { "RadioGroup::RadioButtonsArray" };
	u32 _selectedIndex;
	bool _selectionChanged;
};

struct Label : public Widget
{
	Label();
	~Label();

	void SetText(const char* text);
	void SetRect(NVGcontext* nvgContext, f32 x, f32 y);

	void Draw(NVGcontext* nvgContext) override;

private:
	const char* _text;
};
