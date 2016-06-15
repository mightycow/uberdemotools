#include "nanovg_drawing.hpp"
#include "math.hpp"

#include <math.h>


NVGcolor nvgGrey(unsigned char c)
{
	return nvgRGB(c, c, c);
}

NVGcolor nvgGreyA(unsigned char c, unsigned char a)
{
	return nvgRGBA(c, c, c, a);
}

void DrawProgressBar(NVGcontext* nvgContext, f32 x, f32 y, f32 w, f32 h, f32 r, f32 progress)
{
	const f32 progressDiskCenterX = x + r + (w - 2.0f*r) * progress;

	nvgBeginPath(nvgContext);
	nvgRoundedRect(nvgContext, x, y + 2.0f, w, h, r);
	nvgStrokeColor(nvgContext, nvgGrey(144));
	nvgStrokeWidth(nvgContext, 2.0f);
	nvgStroke(nvgContext);
	nvgClosePath(nvgContext);

	nvgBeginPath(nvgContext);
	nvgRoundedRect(nvgContext, x, y, w, h, r);
	nvgFillPaint(nvgContext, nvgLinearGradient(nvgContext, x, y, x, y + h, nvgGrey(82), nvgGrey(115)));
	nvgFill(nvgContext);
	nvgStrokeColor(nvgContext, nvgGrey(73));
	nvgStrokeWidth(nvgContext, 2.0f);
	nvgStroke(nvgContext);
	nvgClosePath(nvgContext);

	nvgBeginPath(nvgContext);
	nvgCircle(nvgContext, progressDiskCenterX, y + h / 2.0f, r);
	nvgFillPaint(nvgContext, nvgLinearGradient(nvgContext, progressDiskCenterX, y, progressDiskCenterX, y + h, nvgGrey(180), nvgGrey(133)));
	nvgFill(nvgContext);
	nvgStrokeColor(nvgContext, nvgGrey(58));
	nvgStrokeWidth(nvgContext, 1.0f);
	nvgStroke(nvgContext);
	nvgClosePath(nvgContext);
}

void DrawPlayer(NVGcontext* nvgContext, f32 x, f32 y, f32 r, f32 a, bool firing)
{
	const f32 fovRadius = 6.0f * r;
	const f32 x0 = x + fovRadius * cosf(a - UDT_PI / 6.0f);
	const f32 y0 = y + fovRadius * sinf(a - UDT_PI / 6.0f);
	const f32 x1 = x + fovRadius * cosf(a + UDT_PI / 6.0f);
	const f32 y1 = y + fovRadius * sinf(a + UDT_PI / 6.0f);

	nvgBeginPath(nvgContext);
	nvgMoveTo(nvgContext, x, y);
	nvgLineTo(nvgContext, x0, y0);
	nvgLineTo(nvgContext, x1, y1);
	nvgFillPaint(nvgContext, nvgRadialGradient(nvgContext, x, y, 0.0f, fovRadius, nvgGreyA(255, 127), nvgGreyA(255, 0)));
	nvgFill(nvgContext);
	nvgClosePath(nvgContext);

	const NVGcolor inside = firing ? nvgRGB(255, 191, 191) : nvgRGB(255, 127, 127);
	const NVGcolor outside = firing ? nvgRGB(255, 63, 63) : nvgRGB(255, 0, 0);
	nvgBeginPath(nvgContext);
	nvgCircle(nvgContext, x, y, r);
	nvgFillPaint(nvgContext, nvgRadialGradient(nvgContext, x, y, 0.0f, r, inside, outside));
	nvgFill(nvgContext);
	nvgStrokeColor(nvgContext, nvgGrey(0));
	nvgStrokeWidth(nvgContext, 1.5f);
	nvgStroke(nvgContext);
	nvgClosePath(nvgContext);
}

void DrawGrenade(NVGcontext* nvgContext, f32 x, f32 y, f32 r)
{
	nvgBeginPath(nvgContext);
	nvgCircle(nvgContext, x, y, r);
	nvgFillPaint(nvgContext, nvgRadialGradient(nvgContext, x, y, 0.0f, r, nvgRGB(127, 255, 127), nvgRGB(0, 255, 0)));
	nvgFill(nvgContext);
	nvgStrokeColor(nvgContext, nvgGrey(0));
	nvgStrokeWidth(nvgContext, 1.0f);
	nvgStroke(nvgContext);
	nvgClosePath(nvgContext);
}

void DrawRailBeam(NVGcontext* nvgContext, f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1, f32 alpha)
{
	const f32 dx = x1 - x0;
	const f32 dy = y1 - y0;
	const f32 x = (x0 + x1) / 2.0f;
	const f32 y = (y0 + y1) / 2.0f;
	const f32 length = sqrtf(dx*dx + dy*dy);
	const f32 angle = 0.5f * UDT_PI - atan2f(dx, dy);
	const f32 xa = x + cosf(angle - 0.5f * UDT_PI);
	const f32 ya = y + sinf(angle - 0.5f * UDT_PI);
	const f32 xb = x - cosf(angle - 0.5f * UDT_PI);
	const f32 yb = y - sinf(angle - 0.5f * UDT_PI);

	nvgResetTransform(nvgContext);
	nvgTranslate(nvgContext, xa, ya);
	nvgRotate(nvgContext, angle);
	nvgScale(nvgContext, length, 0.75f);
	nvgBeginPath(nvgContext);
	nvgMoveTo(nvgContext, -0.5f, -0.5f * z0);
	nvgLineTo(nvgContext, -0.5f, 0.5f * z0);
	nvgLineTo(nvgContext, 0.5f, 0.5f * z1);
	nvgLineTo(nvgContext, 0.5f, -0.5f * z1);
	nvgLineTo(nvgContext, -0.5f, -0.5f * z0);
	nvgFillColor(nvgContext, nvgRGBAf(1.0f, 0.0f, 0.0f, alpha));
	nvgFill(nvgContext);
	nvgClosePath(nvgContext);
	nvgResetTransform(nvgContext);

	nvgResetTransform(nvgContext);
	nvgTranslate(nvgContext, xb, yb);
	nvgRotate(nvgContext, angle);
	nvgScale(nvgContext, length, 0.75f);
	nvgBeginPath(nvgContext);
	nvgMoveTo(nvgContext, -0.5f, -0.5f * z0);
	nvgLineTo(nvgContext, -0.5f, 0.5f * z0);
	nvgLineTo(nvgContext, 0.5f, 0.5f * z1);
	nvgLineTo(nvgContext, 0.5f, -0.5f * z1);
	nvgLineTo(nvgContext, -0.5f, -0.5f * z0);
	nvgFillColor(nvgContext, nvgRGBAf(1.0f, 0.0f, 0.0f, alpha));
	nvgFill(nvgContext);
	nvgClosePath(nvgContext);
	nvgResetTransform(nvgContext);

	nvgResetTransform(nvgContext);
	nvgTranslate(nvgContext, x, y);
	nvgRotate(nvgContext, angle);
	nvgScale(nvgContext, length, 0.5f);
	nvgBeginPath(nvgContext);
	nvgMoveTo(nvgContext, -0.5f, -0.5f * z0);
	nvgLineTo(nvgContext, -0.5f, 0.5f * z0);
	nvgLineTo(nvgContext, 0.5f, 0.5f * z1);
	nvgLineTo(nvgContext, 0.5f, -0.5f * z1);
	nvgLineTo(nvgContext, -0.5f, -0.5f * z0);
	nvgFillColor(nvgContext, nvgRGBAf(1.0f, 1.0f, 1.0f, alpha));
	nvgFill(nvgContext);
	nvgClosePath(nvgContext);
	nvgResetTransform(nvgContext);
}

void DrawShaftBeam(NVGcontext* nvgContext, f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1)
{
	const f32 dx = x1 - x0;
	const f32 dy = y1 - y0;
	const f32 x = (x0 + x1) / 2.0f;
	const f32 y = (y0 + y1) / 2.0f;
	const f32 length = sqrtf(dx*dx + dy*dy);
	const f32 angle = 0.5f * UDT_PI - atan2f(dx, dy);
	const f32 xa = x + cosf(angle - 0.5f * UDT_PI);
	const f32 ya = y + sinf(angle - 0.5f * UDT_PI);
	const f32 xb = x - cosf(angle - 0.5f * UDT_PI);
	const f32 yb = y - sinf(angle - 0.5f * UDT_PI);

	nvgResetTransform(nvgContext);
	nvgTranslate(nvgContext, xa, ya);
	nvgRotate(nvgContext, angle);
	nvgScale(nvgContext, length, 1.5f);
	nvgBeginPath(nvgContext);
	nvgMoveTo(nvgContext, -0.5f, -0.5f * z0);
	nvgLineTo(nvgContext, -0.5f, 0.5f * z0);
	nvgLineTo(nvgContext, 0.5f, 0.5f * z1);
	nvgLineTo(nvgContext, 0.5f, -0.5f * z1);
	nvgLineTo(nvgContext, -0.5f, -0.5f * z0);
	nvgFillColor(nvgContext, nvgRGB(0, 0, 255));
	nvgFill(nvgContext);
	nvgClosePath(nvgContext);
	nvgResetTransform(nvgContext);

	nvgResetTransform(nvgContext);
	nvgTranslate(nvgContext, xb, yb);
	nvgRotate(nvgContext, angle);
	nvgScale(nvgContext, length, 1.5f);
	nvgBeginPath(nvgContext);
	nvgMoveTo(nvgContext, -0.5f, -0.5f * z0);
	nvgLineTo(nvgContext, -0.5f, 0.5f * z0);
	nvgLineTo(nvgContext, 0.5f, 0.5f * z1);
	nvgLineTo(nvgContext, 0.5f, -0.5f * z1);
	nvgLineTo(nvgContext, -0.5f, -0.5f * z0);
	nvgFillColor(nvgContext, nvgRGB(0, 0, 255));
	nvgFill(nvgContext);
	nvgClosePath(nvgContext);
	nvgResetTransform(nvgContext);

	nvgResetTransform(nvgContext);
	nvgTranslate(nvgContext, x, y);
	nvgRotate(nvgContext, angle);
	nvgScale(nvgContext, length, 1.5f);
	nvgBeginPath(nvgContext);
	nvgMoveTo(nvgContext, -0.5f, -0.5f * z0);
	nvgLineTo(nvgContext, -0.5f, 0.5f * z0);
	nvgLineTo(nvgContext, 0.5f, 0.5f * z1);
	nvgLineTo(nvgContext, 0.5f, -0.5f * z1);
	nvgLineTo(nvgContext, -0.5f, -0.5f * z0);
	nvgFillColor(nvgContext, nvgGrey(255));
	nvgFill(nvgContext);
	nvgClosePath(nvgContext);
	nvgResetTransform(nvgContext);
}

void DrawImpact(NVGcontext* nvgContext, f32 x, f32 y, f32 r)
{
	nvgBeginPath(nvgContext);
	nvgCircle(nvgContext, x, y, r);
	nvgFillColor(nvgContext, nvgGrey(255));
	nvgFill(nvgContext);
	nvgStrokeColor(nvgContext, nvgGrey(0));
	nvgStrokeWidth(nvgContext, 1.0f);
	nvgStroke(nvgContext);
	nvgClosePath(nvgContext);
}
