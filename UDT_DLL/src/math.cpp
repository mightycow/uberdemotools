#include "math.hpp"


f32 DegToRad(f32 angleDeg)
{
	return (angleDeg / 180.0f) * UDT_PI;
}

f32 RadToDeg(f32 angleRad)
{
	return (angleRad / UDT_PI) * 180.0f;
}
