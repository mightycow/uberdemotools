#pragma once


#include "uberdemotools.h"
#include "shared.hpp"
#include "timer.hpp"
#include "nanovg/nanovg.h"


struct Widget
{
	Widget();
	virtual ~Widget();

	void SetRect(f32 x, f32 y, f32 w, f32 h);
	void GetRect(f32& x, f32& y, f32& w, f32& h);

	virtual void MouseButtonDown(s32 x, s32 y, MouseButton::Id button);
	virtual void MouseButtonUp(s32 x, s32 y, MouseButton::Id button);
	virtual void MouseMove(s32 x, s32 y);
	virtual void MouseMoveNC(s32 x, s32 y);
	virtual void MouseScroll(s32 x, s32 y, s32 scroll);
	virtual void Draw(NVGcontext* nvgContext);

protected:
	bool IsPointInside(f32 x, f32 y);

	f32 Pos[2];
	f32 Dim[2];
	bool Hovered;
	bool Enabled;
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
	udtVMArray<Widget*> _widgets;
};

struct DemoProgressBar : public Widget
{
	DemoProgressBar();
	~DemoProgressBar();

	void SetRadius(f32 r);
	void SetProgress(f32 progress);
	bool HasProgressChanged(f32& progress);
	bool HasDragJustStarted();
	bool HasDragJustEnded();

	void MouseButtonDown(s32 x, s32 y, MouseButton::Id button) override;
	void MouseButtonUp(s32 x, s32 y, MouseButton::Id button) override;
	void MouseMove(s32 x, s32 y) override;
	void MouseMoveNC(s32 x, s32 y) override;
	void Draw(NVGcontext* nvgContext) override;

private:
	void ChangeProgress(s32 x, s32 y);
	void SetDragging(bool dragging);

	f32 Radius;
	f32 Progress;
	u8 OrientationAxis;
	bool ProgressChanged;
	bool DraggingSlider;
	bool DragStarted;
	bool DragEnded;
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

private:
	bool Pressed;
	bool Clicked;
};

struct PlayPauseButton : public Button
{
	PlayPauseButton();
	~PlayPauseButton();

	void SetPlaying(bool playing);

	void Draw(NVGcontext* nvgContext) override;

private:
	bool Playing;
};

struct StopButton : public Button
{
	void Draw(NVGcontext* nvgContext) override;
};

struct ReverseButton : public Button
{
	ReverseButton();
	~ReverseButton();

	void SetReversed(bool reversed);

	void Draw(NVGcontext* nvgContext) override;

private:
	bool Reversed;
};

struct ButtonBar : public Widget
{
	ButtonBar();
	~ButtonBar();

	void AddButton(Button* button);
	bool WasClicked(Button*& button);
	void DoLayout(f32 x, f32 y, f32 h, f32 r);

	void MouseButtonDown(s32 x, s32 y, MouseButton::Id button) override;
	void MouseButtonUp(s32 x, s32 y, MouseButton::Id button) override;
	void MouseMove(s32 x, s32 y) override;
	void MouseMoveNC(s32 x, s32 y) override;
	void Draw(NVGcontext* nvgContext) override;

private:
	udtVMArray<Button*> _buttons;
	f32 _radius;
};

struct TitleBar : public Widget
{
	TitleBar();
	~TitleBar();

	bool IsBeingDragged(s32& x, s32& y);

	void MouseButtonDown(s32 x, s32 y, MouseButton::Id button) override;
	void MouseButtonUp(s32 x, s32 y, MouseButton::Id button) override;
	void MouseMove(s32 x, s32 y) override;
	void MouseMoveNC(s32 x, s32 y) override;
	void Draw(NVGcontext* nvgContext) override;

private:
	bool Dragging;
	s32 OffsetX;
	s32 OffsetY;
	s32 CursorX;
	s32 CursorY;
};
