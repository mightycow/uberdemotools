#include "uberdemotools.h"
#include "macros.hpp"


struct Float2
{
	Float2()
	{
	}

	Float2(f32 x, f32 y)
	{
		Set(x, y);
	}

	void Set(f32 x, f32 y)
	{
		v[0] = x;
		v[1] = y;
	}

	static Float2 Zero()
	{
		return Float2(0.0f, 0.0f);
	}

	static Float2 One()
	{
		return Float2(1.0f, 1.0f);
	}

	f32 operator[](int i) const
	{
		return v[i];
	}

	f32& operator[](int i)
	{
		return v[i];
	}

	operator f32*()
	{
		return v;
	}

	operator const f32*() const
	{
		return v;
	}

private:
	f32 v[2];
};

UDT_INLINE Float2 operator*(const Float2& a, const Float2& b)
{
	return Float2(a[0] * b[0], a[1] * b[1]);
}

UDT_INLINE Float2 operator/(const Float2& a, const Float2& b)
{
	return Float2(a[0] / b[0], a[1] / b[1]);
}

UDT_INLINE Float2 operator+(const Float2& a, const Float2& b)
{
	return Float2(a[0] + b[0], a[1] + b[1]);
}

UDT_INLINE Float2 operator-(const Float2& a, const Float2& b)
{
	return Float2(a[0] - b[0], a[1] - b[1]);
}

UDT_INLINE Float2 mad(const Float2& in, const Float2& scale, const Float2& bias)
{
	return Float2(in[0] * scale[0] + bias[0], in[1] * scale[1] + bias[1]);
}

struct Rectangle
{
	Rectangle() 
	{
	}

	Rectangle(f32 x, f32 y, f32 w, f32 h)
	{
		Set(x, y, w, h);
	}

	void Set(f32 x, f32 y, f32 w, f32 h)
	{
		Pos[0] = x;
		Pos[1] = y;
		Dim[0] = w;
		Dim[1] = h;
	}

	f32 X() const
	{
		return Pos[0];
	}

	f32 Y() const
	{
		return Pos[1];
	}

	f32 Left() const
	{
		return Pos[0];
	}

	f32 Top() const
	{
		return Pos[1];
	}

	f32 Right() const
	{
		return Pos[0] + Dim[0];
	}

	f32 Bottom() const
	{
		return Pos[1] + Dim[1];
	}

	f32 Width() const
	{
		return Dim[0];
	}

	f32 Height() const
	{
		return Dim[1];
	}

	f32 CenterX() const
	{
		return Pos[0] + Dim[0] / 2.0f;
	}

	f32 CenterY() const
	{
		return Pos[1] + Dim[1] / 2.0f;
	}

	Float2 TopLeft() const
	{
		return Pos;
	}

	Float2 BottomLeft() const
	{
		return Float2(Left(), Bottom());
	}

	Float2 TopRight() const
	{
		return Float2(Right(), Top());
	}

	Float2 BottomRight() const
	{
		return Float2(Right(), Bottom());
	}

	Float2 Size() const
	{
		return Pos;
	}

private:
	Float2 Pos;
	Float2 Dim;
};
