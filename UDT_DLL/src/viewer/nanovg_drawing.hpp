#pragma once


#include "uberdemotools.h"
#include "nanovg/nanovg.h"


extern NVGcolor nvgGrey(unsigned char c);
extern NVGcolor nvgGreyA(unsigned char c, unsigned char a);
extern void DrawProgressBar(NVGcontext* nvgContext, f32 x, f32 y, f32 w, f32 h, f32 r, f32 progress);
extern void DrawPlayer(NVGcontext* nvgContext, f32 x, f32 y, f32 r, f32 a, bool firing);
extern void DrawGrenade(NVGcontext* nvgContext, f32 x, f32 y, f32 r);
extern void DrawRailBeam(NVGcontext* nvgContext, f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1, f32 alpha);
extern void DrawShaftBeam(NVGcontext* nvgContext, f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1);
extern void DrawImpact(NVGcontext* nvgContext, f32 x, f32 y, f32 r);
