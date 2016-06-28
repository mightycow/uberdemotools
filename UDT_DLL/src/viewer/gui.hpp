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

	f32 _radius;
	f32 _progress;
	bool _progressChanged;
	bool _draggingSlider;
	bool _dragStarted;
	bool _dragEnded;
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
