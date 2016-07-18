#include "math.hpp"
#include "utils.hpp"

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
	void Copy(f32* dest, const f32* src)
	{
		dest[0] = src[0];
		dest[1] = src[1];
		dest[2] = src[2];
	}

	f32 Dot(const f32* a, const f32* b)
	{
		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	}

	f32 SquaredDist(const f32* a, const f32* b)
	{
		f32 dir[3];
		Sub(dir, a, b);

		return SquaredLength(dir);
	}

	f32 Dist(const f32* a, const f32* b)
	{
		f32 dir[3];
		Sub(dir, a, b);

		return sqrtf(SquaredLength(dir));
	}

	f32 SquaredLength(const f32* a)
	{
		return a[0]*a[0] + a[1]*a[1] + a[2]*a[2];
	}

	f32 Length(const f32* a)
	{
		return sqrtf(SquaredLength(a));
	}

	void Add(f32* result, const f32* a, const f32* b)
	{
		result[0] = a[0] + b[0];
		result[1] = a[1] + b[1];
		result[2] = a[2] + b[2];
	}

	void Sub(f32* result, const f32* a, const f32* b)
	{
		result[0] = a[0] - b[0];
		result[1] = a[1] - b[1];
		result[2] = a[2] - b[2];
	}

	void Direction(f32* result, const f32* start, const f32* end)
	{
		f32 dir[3];
		Sub(dir, end, start);
		Normalize(result, dir);
	}

	void Normalize(f32* result, const f32* a)
	{
		const f32 length = Length(a);
		result[0] = a[0] / length;
		result[1] = a[1] / length;
		result[2] = a[2] / length;
	}

	void Mad(f32* result, const f32* a, const f32* b, f32 s)
	{
		result[0] = a[0] + b[0] * s;
		result[1] = a[1] + b[1] * s;
		result[2] = a[2] + b[2] * s;
	}

	void Lerp(f32* result, const f32* a, const f32* b, f32 t)
	{
		const f32 s = 1.0f - t;
		result[0] = s*a[0] + t*b[0];
		result[1] = s*a[1] + t*b[1];
		result[2] = s*a[2] + t*b[2];
	}

	void Zero(f32* result)
	{
		result[0] = 0.0f;
		result[1] = 0.0f;
		result[2] = 0.0f;
	}

	void Increment(f32* result, const float* a)
	{
		result[0] += a[0];
		result[1] += a[1];
		result[2] += a[2];
	}

	void EulerAnglesToAxisVector(f32* result, const f32* angles)
	{
		const f32 angle1 = DegToRad(angles[YAW]);
		const f32 sy = sinf(angle1);
		const f32 cy = cosf(angle1);
		const f32 angle2 = DegToRad(angles[PITCH]);
		const f32 sp = sinf(angle2);
		const f32 cp = cosf(angle2);

		result[0] = cp*cy;
		result[1] = cp*sy;
		result[2] = -sp;
	}
}

namespace Quat
{
	void FromEulerAnglesDeg(f32* result, const f32* angles)
	{
		f32 anglesRad[3];
		anglesRad[0] = DegToRad(angles[0]);
		anglesRad[1] = DegToRad(angles[1]);
		anglesRad[2] = DegToRad(angles[2]);
		FromEulerAngles(result, anglesRad);
	}

	void FromEulerAngles(f32* result, const f32* angles)
	{
		const f32 c1 = cosf(angles[0] / 2.0f);
		const f32 s1 = sinf(angles[0] / 2.0f);
		const f32 c2 = cosf(angles[1] / 2.0f);
		const f32 s2 = sinf(angles[1] / 2.0f);
		const f32 c3 = cosf(angles[2] / 2.0f);
		const f32 s3 = sinf(angles[2] / 2.0f);
		const f32 c1c2 = c1*c2;
		const f32 s1s2 = s1*s2;
		result[0] = c1c2*c3 - s1s2*s3;
		result[1] = c1c2*s3 + s1s2*c3;
		result[2] = s1*c2*c3 + c1*s2*s3;
		result[3] = c1*s2*c3 - s1*c2*s3;
	}

	void Normalize(f32* result, const f32* in)
	{
		const f32 norm = sqrtf(Dot(in, in));
		const f32 n = norm != 0.0f ? norm : 1.0f;
		result[0] = in[0] / n;
		result[1] = in[1] / n;
		result[2] = in[2] / n;
		result[3] = in[3] / n;
	}

	f32 Dot(const f32* a, const f32* b)
	{
		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
	}

	void Invert(f32* result, const f32* in)
	{
		// Inverse(q) = Conjugate(q) / (q^2) = Conjugate(q) / DotProduct(q, q)
		const f32 dp = Dot(in, in);
		result[0] = in[0] / dp;
		result[1] = -in[1] / dp;
		result[2] = -in[2] / dp;
		result[3] = -in[3] / dp;
	}

	void Multiply(f32* result, const f32* a, const f32* b)
	{
		result[0] = a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3];
		result[1] = a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2];
		result[2] = a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1];
		result[3] = a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0];
	}

	f32 Angle(const f32* a, const f32* b)
	{
		f32 aInv[4];
		f32 result[4];
		Invert(aInv, a);
		Multiply(result, b, aInv);

		return acosf(result[0]) * 2.0f; // Element 0 is w.
	}

	f32 AngleDiff(const f32* a, const f32* b)
	{
		const f32 angle = Quat::Angle(a, b);
		const f32 angleDiff = fabsf((angle > UDT_PI) ? (2.0f * UDT_PI - angle) : angle);

		return angleDiff;
	}
}
