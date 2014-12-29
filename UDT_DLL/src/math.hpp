#pragma once


#include "types.hpp"


#define UDT_PI 3.14159265358979323846f


extern f32 DegToRad(f32 angleDeg);
extern f32 RadToDeg(f32 angleRad);

namespace Float3
{
	extern void Copy(f32* a, const f32* b);
	extern f32  Dot(const f32* a, const f32* b);
	extern f32  Dist(const f32* a, const f32* b);
	extern f32  SquaredLength(const f32* a);
	extern f32  Length(const f32* a);
	extern void Mad(f32* result, const f32* a, const f32* b, f32 s);
}
