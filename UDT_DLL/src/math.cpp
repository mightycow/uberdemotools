#include "math.hpp"


float DegToRad(float angleDeg)
{
	return (angleDeg / 180.0f) * UDT_PI;
}

float RadToDeg(float angleRad)
{
	return (angleRad / UDT_PI) * 180.0f;
}