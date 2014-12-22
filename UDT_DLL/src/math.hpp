#pragma once


#include "types.hpp"


#define UDT_PI 3.14159265358979323846f


extern f32 DegToRad(f32 angleDeg);
extern f32 RadToDeg(f32 angleRad);

namespace Float3
{
	extern f32 Dot(const f32* a, const f32* b);
	extern f32 Dist(const f32* a, const f32* b);
}
