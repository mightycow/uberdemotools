#pragma once


#include "types.hpp"


#define UDT_PI 3.14159265358979323846f


extern f32 DegToRad(f32 angleDeg);
extern f32 RadToDeg(f32 angleRad);

namespace Float3
{
	extern void Copy(f32* dest, const f32* src);
	extern f32  Dot(const f32* a, const f32* b);
	extern f32  Dist(const f32* a, const f32* b);
	extern f32  SquaredLength(const f32* a);
	extern f32  Length(const f32* a);
	extern void Mad(f32* result, const f32* a, const f32* b, f32 s);
	extern void Zero(f32* result);
	extern void Increment(f32* result, const float* a);
}

namespace Quat
{
	// Quaternion elements order: 0=w 1=x 2=y 3=z
	extern void FromEulerAnglesDeg(f32* result, const f32* angles);
	extern void FromEulerAngles(f32* result, const f32* angles);
	extern void Normalize(f32* result, const f32* in);
	extern f32  Dot(const f32* a, const f32* b);
	extern void Invert(f32* result, const f32* in);
	extern void Multiply(f32* result, const f32* a, const f32* b);
	extern f32  Angle(const f32* a, const f32* b); // Expects normalized input.
}
