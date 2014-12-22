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
	f32 Dot(const f32* a, const f32* b)
	{
		const f32 x = a[0] - b[0];
		const f32 y = a[1] - b[1];
		const f32 z = a[2] - b[2];

		return  x*x + y*y + z*z;
	}

	f32 Dist(const f32* a, const f32* b)
	{
		return sqrtf(Dot(a, b));
	}
}
