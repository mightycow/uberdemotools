#include "math.hpp"

#include <math.h>


f32 DegToRad(f32 angleDeg)
{
	return (angleDeg / 180.0f) * UDT_PI;
}

f32 RadToDeg(f32 angleRad)
{
	return (angleRad / UDT_PI) * 180.0f;
}

namespace Float3
{
	void Copy(f32* a, const f32* b)
	{
		a[0] = b[0];
		a[1] = b[1];
		a[2] = b[2];
	}

	f32 Dot(const f32* a, const f32* b)
	{
		const f32 x = a[0] - b[0];
		const f32 y = a[1] - b[1];
		const f32 z = a[2] - b[2];

		return x*x + y*y + z*z;
	}

	f32 Dist(const f32* a, const f32* b)
	{
		return sqrtf(Dot(a, b));
	}

	f32 SquaredLength(const f32* a)
	{
		return a[0]*a[0] + a[1]*a[1] + a[2]*a[2];
	}

	f32 Length(const f32* a)
	{
		return sqrtf(SquaredLength(a));
	}

	void Mad(f32* result, const f32* a, const f32* b, f32 s)
	{
		result[0] = a[0] + b[0] * s;
		result[1] = a[1] + b[1] * s;
		result[2] = a[2] + b[2] * s;
	}
}
