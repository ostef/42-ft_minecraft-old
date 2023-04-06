#pragma once

// Vec2

template<typename T>
T dot (const Vector<T, 2> &a, const Vector<T, 2> &b)
{
	return a.comps[0] * b.comps[0] + a.comps[1] * b.comps[1];
}

template<typename T>
Vector<T, 2> add (const Vector<T, 2> &a, const Vector<T, 2> &b)
{
	return {a.comps[0] + b.comps[0], a.comps[1] + b.comps[1]};
}

template<typename T>
Vector<T, 2> sub (const Vector<T, 2> &a, const Vector<T, 2> &b)
{
	return {a.comps[0] - b.comps[0], a.comps[1] - b.comps[1]};
}

template<typename T>
Vector<T, 2> mul (const Vector<T, 2> &a, T b)
{
	return {a.comps[0] * b, a.comps[1] * b};
}

template<typename T>
Vector<T, 2> div (const Vector<T, 2> &a, T b)
{
	return {a.comps[0] / b, a.comps[1] / b};
}

template<typename T>
Vector<T, 2> scale (const Vector<T, 2> &a, const Vector<T, 2> &b)
{
	return {a.comps[0] * b.comps[0], a.comps[1] * b.comps[1]};
}

template<typename T>
Vector<T, 2> neg (const Vector<T, 2> &v)
{
	return {-v.comps[0], -v.comps[1]};
}

template<typename T>
Vector<T, 2> abs (const Vector<T, 2> &v)
{
	return {abs (v.comps[0]), abs (v.comps[1])};
}

template<typename T>
Vector<T, 2> min (const Vector<T, 2> &a, const Vector<T, 2> &b)
{
	return {min (a.comps[0], b.comps[0]), min (a.comps[1], b.comps[1])};
}

template<typename T>
Vector<T, 2> max (const Vector<T, 2> &a, const Vector<T, 2> &b)
{
	return {max (a.comps[0], b.comps[0]), max (a.comps[1], b.comps[1])};
}

template<typename T>
Vector<T, 2> clamp (const Vector<T, 2> &v, const Vector<T, 2> &a, const Vector<T, 2> &b)
{
	return {clamp (v.comps[0], a.comps[0], b.comps[0]), clamp (v.comps[1], a.comps[1], b.comps[1])};
}

template<typename T>
Vector<T, 2> round (const Vector<T, 2> &v)
{
	return {round (v.comps[0]), round (v.comps[1])};
}

template<typename T>
Vector<T, 2> floor (const Vector<T, 2> &v)
{
	return {floor (v.comps[0]), floor (v.comps[1])};
}

template<typename T>
Vector<T, 2> ceil (const Vector<T, 2> &v)
{
	return {ceil (v.comps[0]), ceil (v.comps[1])};
}

template<typename T>
Vector<T, 2> lerp (const Vector<T, 2> &a, const Vector<T, 2> &b, f32 t)
{
	return {lerp (a.comps[0], b.comps[0], t), lerp (a.comps[1], b.comps[1], t)};
}

template<typename T, typename E>
bool approx_zero (const Vector<T, 2> &v, E epsilon = 0.00001)
{
	return approx_zero (v.comps[0], epsilon) && approx_zero (v.comps[1], epsilon);
}

template<typename T, typename E>
bool approx_equals (const Vector<T, 2> &a, const Vector<T, 2> &b, E epsilon = 0.00001)
{
	return approx_equals (a.comps[0], b.comps[0], epsilon) && approx_equals (a.comps[1], b.comps[1], epsilon);
}

template<typename T>
bool equals (const Vector<T, 2> &a, const Vector<T, 2> &b)
{
	return a.comps[0] == b.comps[0] && a.comps[1] == b.comps[1];
}

// Vec3

template<typename T>
T dot (const Vector<T, 3> &a, const Vector<T, 3> &b)
{
	return a.comps[0] * b.comps[0] + a.comps[1] * b.comps[1] + a.comps[2] * b.comps[2];
}

template<typename T>
Vector<T, 3> add (const Vector<T, 3> &a, const Vector<T, 3> &b)
{
	return {a.comps[0] + b.comps[0], a.comps[1] + b.comps[1], a.comps[2] + b.comps[2]};
}

template<typename T>
Vector<T, 3> sub (const Vector<T, 3> &a, const Vector<T, 3> &b)
{
	return {a.comps[0] - b.comps[0], a.comps[1] - b.comps[1], a.comps[2] - b.comps[2]};
}

template<typename T>
Vector<T, 3> mul (const Vector<T, 3> &a, T b)
{
	return {a.comps[0] * b, a.comps[1] * b, a.comps[2] * b};
}

template<typename T>
Vector<T, 3> div (const Vector<T, 3> &a, T b)
{
	return {a.comps[0] / b, a.comps[1] / b, a.comps[2] / b};
}

template<typename T>
Vector<T, 3> scale (const Vector<T, 3> &a, const Vector<T, 3> &b)
{
	return {a.comps[0] * b.comps[0], a.comps[1] * b.comps[1], a.comps[2] * b.comps[2]};
}

template<typename T>
Vector<T, 3> neg (const Vector<T, 3> &v)
{
	return {-v.comps[0], -v.comps[1], -v.comps[2]};
}

template<typename T>
Vector<T, 3> abs (const Vector<T, 3> &v)
{
	return {abs (v.comps[0]), abs (v.comps[1]), abs (v.comps[2])};
}

template<typename T>
Vector<T, 3> min (const Vector<T, 3> &a, const Vector<T, 3> &b)
{
	return {min (a.comps[0], b.comps[0]), min (a.comps[1], b.comps[1]), min (a.comps[2], b.comps[2])};
}

template<typename T>
Vector<T, 3> max (const Vector<T, 3> &a, const Vector<T, 3> &b)
{
	return {max (a.comps[0], b.comps[0]), max (a.comps[1], b.comps[1]), max (a.comps[2], b.comps[2])};
}

template<typename T>
Vector<T, 3> clamp (const Vector<T, 3> &v, const Vector<T, 3> &a, const Vector<T, 3> &b)
{
	return {clamp (v.comps[0], a.comps[0], b.comps[0]), clamp (v.comps[1], a.comps[1], b.comps[1]), clamp (v.comps[2], a.comps[2], b.comps[2])};
}

template<typename T>
Vector<T, 3> round (const Vector<T, 3> &v)
{
	return {round (v.comps[0]), round (v.comps[1]), round (v.comps[2])};
}

template<typename T>
Vector<T, 3> floor (const Vector<T, 3> &v)
{
	return {floor (v.comps[0]), floor (v.comps[1]), floor (v.comps[2])};
}

template<typename T>
Vector<T, 3> ceil (const Vector<T, 3> &v)
{
	return {ceil (v.comps[0]), ceil (v.comps[1]), ceil (v.comps[2])};
}

template<typename T>
Vector<T, 3> lerp (const Vector<T, 3> &a, const Vector<T, 3> &b, f32 t)
{
	return {lerp (a.comps[0], b.comps[0], t), lerp (a.comps[1], b.comps[1], t), lerp (a.comps[2], b.comps[2], t)};
}

template<typename T, typename E>
bool approx_zero (const Vector<T, 3> &v, E epsilon = 0.00001)
{
	return approx_zero (v.comps[0], epsilon) && approx_zero (v.comps[1], epsilon) && approx_zero (v.comps[2], epsilon);
}

template<typename T, typename E>
bool approx_equals (const Vector<T, 3> &a, const Vector<T, 3> &b, E epsilon = 0.00001)
{
	return approx_equals (a.comps[0], b.comps[0], epsilon) && approx_equals (a.comps[1], b.comps[1], epsilon) && approx_equals (a.comps[2], b.comps[2], epsilon);
}

template<typename T>
bool equals (const Vector<T, 3> &a, const Vector<T, 3> &b)
{
	return a.comps[0] == b.comps[0] && a.comps[1] == b.comps[1] && a.comps[2] == b.comps[2];
}

// Vec4

template<typename T>
T dot (const Vector<T, 4> &a, const Vector<T, 4> &b)
{
	return a.comps[0] * b.comps[0] + a.comps[1] * b.comps[1] + a.comps[2] * b.comps[2] + a.comps[3] * b.comps[3];
}

template<typename T>
Vector<T, 4> add (const Vector<T, 4> &a, const Vector<T, 4> &b)
{
	return {a.comps[0] + b.comps[0], a.comps[1] + b.comps[1], a.comps[2] + b.comps[2], a.comps[3] + b.comps[3]};
}

template<typename T>
Vector<T, 4> sub (const Vector<T, 4> &a, const Vector<T, 4> &b)
{
	return {a.comps[0] - b.comps[0], a.comps[1] - b.comps[1], a.comps[2] - b.comps[2], a.comps[3] - b.comps[3]};
}

template<typename T>
Vector<T, 4> mul (const Vector<T, 4> &a, T b)
{
	return {a.comps[0] * b, a.comps[1] * b, a.comps[2] * b, a.comps[3] * b};
}

template<typename T>
Vector<T, 4> div (const Vector<T, 4> &a, T b)
{
	return {a.comps[0] / b, a.comps[1] / b, a.comps[2] / b, a.comps[3] / b};
}

template<typename T>
Vector<T, 4> scale (const Vector<T, 4> &a, const Vector<T, 4> &b)
{
	return {a.comps[0] * b.comps[0], a.comps[1] * b.comps[1], a.comps[2] * b.comps[2], a.comps[3] * b.comps[3]};
}

template<typename T>
Vector<T, 4> neg (const Vector<T, 4> &v)
{
	return {-v.comps[0], -v.comps[1], -v.comps[2], -v.comps[3]};
}

template<typename T>
Vector<T, 4> abs (const Vector<T, 4> &v)
{
	return {abs (v.comps[0]), abs (v.comps[1]), abs (v.comps[2]), abs (v.comps[3])};
}

template<typename T>
Vector<T, 4> min (const Vector<T, 4> &a, const Vector<T, 4> &b)
{
	return {min (a.comps[0], b.comps[0]), min (a.comps[1], b.comps[1]), min (a.comps[2], b.comps[2]), min (a.comps[3], b.comps[3])};
}

template<typename T>
Vector<T, 4> max (const Vector<T, 4> &a, const Vector<T, 4> &b)
{
	return {max (a.comps[0], b.comps[0]), max (a.comps[1], b.comps[1]), max (a.comps[2], b.comps[2]), max (a.comps[3], b.comps[3])};
}

template<typename T>
Vector<T, 4> clamp (const Vector<T, 4> &v, const Vector<T, 4> &a, const Vector<T, 4> &b)
{
	return {clamp (v.comps[0], a.comps[0], b.comps[0]), clamp (v.comps[1], a.comps[1], b.comps[1]), clamp (v.comps[2], a.comps[2], b.comps[2]), clamp (v.comps[3], a.comps[3], b.comps[3])};
}

template<typename T>
Vector<T, 4> round (const Vector<T, 4> &v)
{
	return {round (v.comps[0]), round (v.comps[1]), round (v.comps[2]), round (v.comps[3])};
}

template<typename T>
Vector<T, 4> floor (const Vector<T, 4> &v)
{
	return {floor (v.comps[0]), floor (v.comps[1]), floor (v.comps[2]), floor (v.comps[3])};
}

template<typename T>
Vector<T, 4> ceil (const Vector<T, 4> &v)
{
	return {ceil (v.comps[0]), ceil (v.comps[1]), ceil (v.comps[2]), ceil (v.comps[3])};
}

template<typename T>
Vector<T, 4> lerp (const Vector<T, 4> &a, const Vector<T, 4> &b, f32 t)
{
	return {lerp (a.comps[0], b.comps[0], t), lerp (a.comps[1], b.comps[1], t), lerp (a.comps[2], b.comps[2], t), lerp (a.comps[3], b.comps[3], t)};
}

template<typename T, typename E>
bool approx_zero (const Vector<T, 4> &v, E epsilon = 0.00001)
{
	return approx_zero (v.comps[0], epsilon) && approx_zero (v.comps[1], epsilon) && approx_zero (v.comps[2], epsilon) && approx_zero (v.comps[3], epsilon);
}

template<typename T, typename E>
bool approx_equals (const Vector<T, 4> &a, const Vector<T, 4> &b, E epsilon = 0.00001)
{
	return approx_equals (a.comps[0], b.comps[0], epsilon) && approx_equals (a.comps[1], b.comps[1], epsilon) && approx_equals (a.comps[2], b.comps[2], epsilon) && approx_equals (a.comps[3], b.comps[3], epsilon);
}

template<typename T>
bool equals (const Vector<T, 4> &a, const Vector<T, 4> &b)
{
	return a.comps[0] == b.comps[0] && a.comps[1] == b.comps[1] && a.comps[2] == b.comps[2] && a.comps[3] == b.comps[3];
}

// Mat3

template<typename T>
Matrix<T, 3, 3> add (const Matrix<T, 3, 3> &a, const Matrix<T, 3, 3> &b)
{
	return {a.comps[0] + b.comps[0], a.comps[1] + b.comps[1], a.comps[2] + b.comps[2], a.comps[3] + b.comps[3], a.comps[4] + b.comps[4], a.comps[5] + b.comps[5], a.comps[6] + b.comps[6], a.comps[7] + b.comps[7], a.comps[8] + b.comps[8]};
}

template<typename T>
Matrix<T, 3, 3> sub (const Matrix<T, 3, 3> &a, const Matrix<T, 3, 3> &b)
{
	return {a.comps[0] - b.comps[0], a.comps[1] - b.comps[1], a.comps[2] - b.comps[2], a.comps[3] - b.comps[3], a.comps[4] - b.comps[4], a.comps[5] - b.comps[5], a.comps[6] - b.comps[6], a.comps[7] - b.comps[7], a.comps[8] - b.comps[8]};
}

template<typename T>
Matrix<T, 3, 3> mul (const Matrix<T, 3, 3> &a, T b)
{
	return {a.comps[0] * b, a.comps[1] * b, a.comps[2] * b, a.comps[3] * b, a.comps[4] * b, a.comps[5] * b, a.comps[6] * b, a.comps[7] * b, a.comps[8] * b};
}

template<typename T>
Vector<T, 3> mul (const Matrix<T, 3, 3> &a, const Vector<T, 3> &b)
{
	return {a.comps[0 * 3 + 0] * b.comps[0] + a.comps[0 * 3 + 1] * b.comps[1] + a.comps[0 * 3 + 2] * b.comps[2],
		a.comps[1 * 3 + 0] * b.comps[0] + a.comps[1 * 3 + 1] * b.comps[1] + a.comps[1 * 3 + 2] * b.comps[2],
		a.comps[2 * 3 + 0] * b.comps[0] + a.comps[2 * 3 + 1] * b.comps[1] + a.comps[2 * 3 + 2] * b.comps[2]};
}

template<typename T>
Vector<T, 3> mul (const Vector<T, 3> &a, const Matrix<T, 3, 3> &b)
{
	return {a.comps[0] * b.comps[0 * 3 + 0] + a.comps[1] * b.comps[1 * 3 + 0] + a.comps[2] * b.comps[2 * 3 + 0],
		a.comps[0] * b.comps[0 * 3 + 1] + a.comps[1] * b.comps[1 * 3 + 1] + a.comps[2] * b.comps[2 * 3 + 1],
		a.comps[0] * b.comps[0 * 3 + 2] + a.comps[1] * b.comps[1 * 3 + 2] + a.comps[2] * b.comps[2 * 3 + 2]};
}

template<typename T>
Matrix<T, 3, 3> mul (const Matrix<T, 3, 3> &a, const Matrix<T, 3, 3> &b)
{
	return {a.comps[0 * 3 + 0] * b.comps[0 * 3 + 0] + a.comps[0 * 3 + 1] * b.comps[1 * 3 + 0] + a.comps[0 * 3 + 2] * b.comps[2 * 3 + 0], a.comps[0 * 3 + 0] * b.comps[0 * 3 + 1] + a.comps[0 * 3 + 1] * b.comps[1 * 3 + 1] + a.comps[0 * 3 + 2] * b.comps[2 * 3 + 1], a.comps[0 * 3 + 0] * b.comps[0 * 3 + 2] + a.comps[0 * 3 + 1] * b.comps[1 * 3 + 2] + a.comps[0 * 3 + 2] * b.comps[2 * 3 + 2], 
		a.comps[1 * 3 + 0] * b.comps[0 * 3 + 0] + a.comps[1 * 3 + 1] * b.comps[1 * 3 + 0] + a.comps[1 * 3 + 2] * b.comps[2 * 3 + 0], a.comps[1 * 3 + 0] * b.comps[0 * 3 + 1] + a.comps[1 * 3 + 1] * b.comps[1 * 3 + 1] + a.comps[1 * 3 + 2] * b.comps[2 * 3 + 1], a.comps[1 * 3 + 0] * b.comps[0 * 3 + 2] + a.comps[1 * 3 + 1] * b.comps[1 * 3 + 2] + a.comps[1 * 3 + 2] * b.comps[2 * 3 + 2], 
		a.comps[2 * 3 + 0] * b.comps[0 * 3 + 0] + a.comps[2 * 3 + 1] * b.comps[1 * 3 + 0] + a.comps[2 * 3 + 2] * b.comps[2 * 3 + 0], a.comps[2 * 3 + 0] * b.comps[0 * 3 + 1] + a.comps[2 * 3 + 1] * b.comps[1 * 3 + 1] + a.comps[2 * 3 + 2] * b.comps[2 * 3 + 1], a.comps[2 * 3 + 0] * b.comps[0 * 3 + 2] + a.comps[2 * 3 + 1] * b.comps[1 * 3 + 2] + a.comps[2 * 3 + 2] * b.comps[2 * 3 + 2]};
}

template<typename T>
Matrix<T, 3, 3> div (const Matrix<T, 3, 3> &a, T b)
{
	return {a.comps[0] / b, a.comps[1] / b, a.comps[2] / b, a.comps[3] / b, a.comps[4] / b, a.comps[5] / b, a.comps[6] / b, a.comps[7] / b, a.comps[8] / b};
}

template<typename T>
Matrix<T, 3, 3> transposed (const Matrix<T, 3, 3> &m)
{
	return {m.comps[0], m.comps[3], m.comps[6], m.comps[1], m.comps[4], m.comps[7], m.comps[2], m.comps[5], m.comps[8]};
}

// Mat3x4

template<typename T>
Matrix<T, 3, 4> add (const Matrix<T, 3, 4> &a, const Matrix<T, 3, 4> &b)
{
	return {a.comps[0] + b.comps[0], a.comps[1] + b.comps[1], a.comps[2] + b.comps[2], a.comps[3] + b.comps[3], a.comps[4] + b.comps[4], a.comps[5] + b.comps[5], a.comps[6] + b.comps[6], a.comps[7] + b.comps[7], a.comps[8] + b.comps[8], a.comps[9] + b.comps[9], a.comps[10] + b.comps[10], a.comps[11] + b.comps[11]};
}

template<typename T>
Matrix<T, 3, 4> sub (const Matrix<T, 3, 4> &a, const Matrix<T, 3, 4> &b)
{
	return {a.comps[0] - b.comps[0], a.comps[1] - b.comps[1], a.comps[2] - b.comps[2], a.comps[3] - b.comps[3], a.comps[4] - b.comps[4], a.comps[5] - b.comps[5], a.comps[6] - b.comps[6], a.comps[7] - b.comps[7], a.comps[8] - b.comps[8], a.comps[9] - b.comps[9], a.comps[10] - b.comps[10], a.comps[11] - b.comps[11]};
}

template<typename T>
Matrix<T, 3, 4> mul (const Matrix<T, 3, 4> &a, T b)
{
	return {a.comps[0] * b, a.comps[1] * b, a.comps[2] * b, a.comps[3] * b, a.comps[4] * b, a.comps[5] * b, a.comps[6] * b, a.comps[7] * b, a.comps[8] * b, a.comps[9] * b, a.comps[10] * b, a.comps[11] * b};
}

template<typename T>
Vector<T, 3> mul (const Matrix<T, 3, 4> &a, const Vector<T, 4> &b)
{
	return {a.comps[0 * 4 + 0] * b.comps[0] + a.comps[0 * 4 + 1] * b.comps[1] + a.comps[0 * 4 + 2] * b.comps[2] + a.comps[0 * 4 + 3] * b.comps[3],
		a.comps[1 * 4 + 0] * b.comps[0] + a.comps[1 * 4 + 1] * b.comps[1] + a.comps[1 * 4 + 2] * b.comps[2] + a.comps[1 * 4 + 3] * b.comps[3],
		a.comps[2 * 4 + 0] * b.comps[0] + a.comps[2 * 4 + 1] * b.comps[1] + a.comps[2 * 4 + 2] * b.comps[2] + a.comps[2 * 4 + 3] * b.comps[3]};
}

template<typename T>
Vector<T, 4> mul (const Vector<T, 3> &a, const Matrix<T, 3, 4> &b)
{
	return {a.comps[0] * b.comps[0 * 4 + 0] + a.comps[1] * b.comps[1 * 4 + 0] + a.comps[2] * b.comps[2 * 4 + 0],
		a.comps[0] * b.comps[0 * 4 + 1] + a.comps[1] * b.comps[1 * 4 + 1] + a.comps[2] * b.comps[2 * 4 + 1],
		a.comps[0] * b.comps[0 * 4 + 2] + a.comps[1] * b.comps[1 * 4 + 2] + a.comps[2] * b.comps[2 * 4 + 2],
		a.comps[0] * b.comps[0 * 4 + 3] + a.comps[1] * b.comps[1 * 4 + 3] + a.comps[2] * b.comps[2 * 4 + 3]};
}

template<typename T>
Matrix<T, 3, 3> mul (const Matrix<T, 3, 4> &a, const Matrix<T, 4, 3> &b)
{
	return {a.comps[0 * 4 + 0] * b.comps[0 * 3 + 0] + a.comps[0 * 4 + 1] * b.comps[1 * 3 + 0] + a.comps[0 * 4 + 2] * b.comps[2 * 3 + 0] + a.comps[0 * 4 + 3] * b.comps[3 * 3 + 0], a.comps[0 * 4 + 0] * b.comps[0 * 3 + 1] + a.comps[0 * 4 + 1] * b.comps[1 * 3 + 1] + a.comps[0 * 4 + 2] * b.comps[2 * 3 + 1] + a.comps[0 * 4 + 3] * b.comps[3 * 3 + 1], a.comps[0 * 4 + 0] * b.comps[0 * 3 + 2] + a.comps[0 * 4 + 1] * b.comps[1 * 3 + 2] + a.comps[0 * 4 + 2] * b.comps[2 * 3 + 2] + a.comps[0 * 4 + 3] * b.comps[3 * 3 + 2], 
		a.comps[1 * 4 + 0] * b.comps[0 * 3 + 0] + a.comps[1 * 4 + 1] * b.comps[1 * 3 + 0] + a.comps[1 * 4 + 2] * b.comps[2 * 3 + 0] + a.comps[1 * 4 + 3] * b.comps[3 * 3 + 0], a.comps[1 * 4 + 0] * b.comps[0 * 3 + 1] + a.comps[1 * 4 + 1] * b.comps[1 * 3 + 1] + a.comps[1 * 4 + 2] * b.comps[2 * 3 + 1] + a.comps[1 * 4 + 3] * b.comps[3 * 3 + 1], a.comps[1 * 4 + 0] * b.comps[0 * 3 + 2] + a.comps[1 * 4 + 1] * b.comps[1 * 3 + 2] + a.comps[1 * 4 + 2] * b.comps[2 * 3 + 2] + a.comps[1 * 4 + 3] * b.comps[3 * 3 + 2], 
		a.comps[2 * 4 + 0] * b.comps[0 * 3 + 0] + a.comps[2 * 4 + 1] * b.comps[1 * 3 + 0] + a.comps[2 * 4 + 2] * b.comps[2 * 3 + 0] + a.comps[2 * 4 + 3] * b.comps[3 * 3 + 0], a.comps[2 * 4 + 0] * b.comps[0 * 3 + 1] + a.comps[2 * 4 + 1] * b.comps[1 * 3 + 1] + a.comps[2 * 4 + 2] * b.comps[2 * 3 + 1] + a.comps[2 * 4 + 3] * b.comps[3 * 3 + 1], a.comps[2 * 4 + 0] * b.comps[0 * 3 + 2] + a.comps[2 * 4 + 1] * b.comps[1 * 3 + 2] + a.comps[2 * 4 + 2] * b.comps[2 * 3 + 2] + a.comps[2 * 4 + 3] * b.comps[3 * 3 + 2]};
}

template<typename T>
Matrix<T, 3, 4> mul (const Matrix<T, 3, 4> &a, const Matrix<T, 4, 4> &b)
{
	return {a.comps[0 * 4 + 0] * b.comps[0 * 4 + 0] + a.comps[0 * 4 + 1] * b.comps[1 * 4 + 0] + a.comps[0 * 4 + 2] * b.comps[2 * 4 + 0] + a.comps[0 * 4 + 3] * b.comps[3 * 4 + 0], a.comps[0 * 4 + 0] * b.comps[0 * 4 + 1] + a.comps[0 * 4 + 1] * b.comps[1 * 4 + 1] + a.comps[0 * 4 + 2] * b.comps[2 * 4 + 1] + a.comps[0 * 4 + 3] * b.comps[3 * 4 + 1], a.comps[0 * 4 + 0] * b.comps[0 * 4 + 2] + a.comps[0 * 4 + 1] * b.comps[1 * 4 + 2] + a.comps[0 * 4 + 2] * b.comps[2 * 4 + 2] + a.comps[0 * 4 + 3] * b.comps[3 * 4 + 2], a.comps[0 * 4 + 0] * b.comps[0 * 4 + 3] + a.comps[0 * 4 + 1] * b.comps[1 * 4 + 3] + a.comps[0 * 4 + 2] * b.comps[2 * 4 + 3] + a.comps[0 * 4 + 3] * b.comps[3 * 4 + 3], 
		a.comps[1 * 4 + 0] * b.comps[0 * 4 + 0] + a.comps[1 * 4 + 1] * b.comps[1 * 4 + 0] + a.comps[1 * 4 + 2] * b.comps[2 * 4 + 0] + a.comps[1 * 4 + 3] * b.comps[3 * 4 + 0], a.comps[1 * 4 + 0] * b.comps[0 * 4 + 1] + a.comps[1 * 4 + 1] * b.comps[1 * 4 + 1] + a.comps[1 * 4 + 2] * b.comps[2 * 4 + 1] + a.comps[1 * 4 + 3] * b.comps[3 * 4 + 1], a.comps[1 * 4 + 0] * b.comps[0 * 4 + 2] + a.comps[1 * 4 + 1] * b.comps[1 * 4 + 2] + a.comps[1 * 4 + 2] * b.comps[2 * 4 + 2] + a.comps[1 * 4 + 3] * b.comps[3 * 4 + 2], a.comps[1 * 4 + 0] * b.comps[0 * 4 + 3] + a.comps[1 * 4 + 1] * b.comps[1 * 4 + 3] + a.comps[1 * 4 + 2] * b.comps[2 * 4 + 3] + a.comps[1 * 4 + 3] * b.comps[3 * 4 + 3], 
		a.comps[2 * 4 + 0] * b.comps[0 * 4 + 0] + a.comps[2 * 4 + 1] * b.comps[1 * 4 + 0] + a.comps[2 * 4 + 2] * b.comps[2 * 4 + 0] + a.comps[2 * 4 + 3] * b.comps[3 * 4 + 0], a.comps[2 * 4 + 0] * b.comps[0 * 4 + 1] + a.comps[2 * 4 + 1] * b.comps[1 * 4 + 1] + a.comps[2 * 4 + 2] * b.comps[2 * 4 + 1] + a.comps[2 * 4 + 3] * b.comps[3 * 4 + 1], a.comps[2 * 4 + 0] * b.comps[0 * 4 + 2] + a.comps[2 * 4 + 1] * b.comps[1 * 4 + 2] + a.comps[2 * 4 + 2] * b.comps[2 * 4 + 2] + a.comps[2 * 4 + 3] * b.comps[3 * 4 + 2], a.comps[2 * 4 + 0] * b.comps[0 * 4 + 3] + a.comps[2 * 4 + 1] * b.comps[1 * 4 + 3] + a.comps[2 * 4 + 2] * b.comps[2 * 4 + 3] + a.comps[2 * 4 + 3] * b.comps[3 * 4 + 3]};
}

template<typename T>
Matrix<T, 3, 4> div (const Matrix<T, 3, 4> &a, T b)
{
	return {a.comps[0] / b, a.comps[1] / b, a.comps[2] / b, a.comps[3] / b, a.comps[4] / b, a.comps[5] / b, a.comps[6] / b, a.comps[7] / b, a.comps[8] / b, a.comps[9] / b, a.comps[10] / b, a.comps[11] / b};
}

// Mat4

template<typename T>
Matrix<T, 4, 4> add (const Matrix<T, 4, 4> &a, const Matrix<T, 4, 4> &b)
{
	return {a.comps[0] + b.comps[0], a.comps[1] + b.comps[1], a.comps[2] + b.comps[2], a.comps[3] + b.comps[3], a.comps[4] + b.comps[4], a.comps[5] + b.comps[5], a.comps[6] + b.comps[6], a.comps[7] + b.comps[7], a.comps[8] + b.comps[8], a.comps[9] + b.comps[9], a.comps[10] + b.comps[10], a.comps[11] + b.comps[11], a.comps[12] + b.comps[12], a.comps[13] + b.comps[13], a.comps[14] + b.comps[14], a.comps[15] + b.comps[15]};
}

template<typename T>
Matrix<T, 4, 4> sub (const Matrix<T, 4, 4> &a, const Matrix<T, 4, 4> &b)
{
	return {a.comps[0] - b.comps[0], a.comps[1] - b.comps[1], a.comps[2] - b.comps[2], a.comps[3] - b.comps[3], a.comps[4] - b.comps[4], a.comps[5] - b.comps[5], a.comps[6] - b.comps[6], a.comps[7] - b.comps[7], a.comps[8] - b.comps[8], a.comps[9] - b.comps[9], a.comps[10] - b.comps[10], a.comps[11] - b.comps[11], a.comps[12] - b.comps[12], a.comps[13] - b.comps[13], a.comps[14] - b.comps[14], a.comps[15] - b.comps[15]};
}

template<typename T>
Matrix<T, 4, 4> mul (const Matrix<T, 4, 4> &a, T b)
{
	return {a.comps[0] * b, a.comps[1] * b, a.comps[2] * b, a.comps[3] * b, a.comps[4] * b, a.comps[5] * b, a.comps[6] * b, a.comps[7] * b, a.comps[8] * b, a.comps[9] * b, a.comps[10] * b, a.comps[11] * b, a.comps[12] * b, a.comps[13] * b, a.comps[14] * b, a.comps[15] * b};
}

template<typename T>
Vector<T, 4> mul (const Matrix<T, 4, 4> &a, const Vector<T, 4> &b)
{
	return {a.comps[0 * 4 + 0] * b.comps[0] + a.comps[0 * 4 + 1] * b.comps[1] + a.comps[0 * 4 + 2] * b.comps[2] + a.comps[0 * 4 + 3] * b.comps[3],
		a.comps[1 * 4 + 0] * b.comps[0] + a.comps[1 * 4 + 1] * b.comps[1] + a.comps[1 * 4 + 2] * b.comps[2] + a.comps[1 * 4 + 3] * b.comps[3],
		a.comps[2 * 4 + 0] * b.comps[0] + a.comps[2 * 4 + 1] * b.comps[1] + a.comps[2 * 4 + 2] * b.comps[2] + a.comps[2 * 4 + 3] * b.comps[3],
		a.comps[3 * 4 + 0] * b.comps[0] + a.comps[3 * 4 + 1] * b.comps[1] + a.comps[3 * 4 + 2] * b.comps[2] + a.comps[3 * 4 + 3] * b.comps[3]};
}

template<typename T>
Vector<T, 4> mul (const Vector<T, 4> &a, const Matrix<T, 4, 4> &b)
{
	return {a.comps[0] * b.comps[0 * 4 + 0] + a.comps[1] * b.comps[1 * 4 + 0] + a.comps[2] * b.comps[2 * 4 + 0] + a.comps[3] * b.comps[3 * 4 + 0],
		a.comps[0] * b.comps[0 * 4 + 1] + a.comps[1] * b.comps[1 * 4 + 1] + a.comps[2] * b.comps[2 * 4 + 1] + a.comps[3] * b.comps[3 * 4 + 1],
		a.comps[0] * b.comps[0 * 4 + 2] + a.comps[1] * b.comps[1 * 4 + 2] + a.comps[2] * b.comps[2 * 4 + 2] + a.comps[3] * b.comps[3 * 4 + 2],
		a.comps[0] * b.comps[0 * 4 + 3] + a.comps[1] * b.comps[1 * 4 + 3] + a.comps[2] * b.comps[2 * 4 + 3] + a.comps[3] * b.comps[3 * 4 + 3]};
}

template<typename T>
Matrix<T, 4, 4> mul (const Matrix<T, 4, 4> &a, const Matrix<T, 4, 4> &b)
{
	return {a.comps[0 * 4 + 0] * b.comps[0 * 4 + 0] + a.comps[0 * 4 + 1] * b.comps[1 * 4 + 0] + a.comps[0 * 4 + 2] * b.comps[2 * 4 + 0] + a.comps[0 * 4 + 3] * b.comps[3 * 4 + 0], a.comps[0 * 4 + 0] * b.comps[0 * 4 + 1] + a.comps[0 * 4 + 1] * b.comps[1 * 4 + 1] + a.comps[0 * 4 + 2] * b.comps[2 * 4 + 1] + a.comps[0 * 4 + 3] * b.comps[3 * 4 + 1], a.comps[0 * 4 + 0] * b.comps[0 * 4 + 2] + a.comps[0 * 4 + 1] * b.comps[1 * 4 + 2] + a.comps[0 * 4 + 2] * b.comps[2 * 4 + 2] + a.comps[0 * 4 + 3] * b.comps[3 * 4 + 2], a.comps[0 * 4 + 0] * b.comps[0 * 4 + 3] + a.comps[0 * 4 + 1] * b.comps[1 * 4 + 3] + a.comps[0 * 4 + 2] * b.comps[2 * 4 + 3] + a.comps[0 * 4 + 3] * b.comps[3 * 4 + 3], 
		a.comps[1 * 4 + 0] * b.comps[0 * 4 + 0] + a.comps[1 * 4 + 1] * b.comps[1 * 4 + 0] + a.comps[1 * 4 + 2] * b.comps[2 * 4 + 0] + a.comps[1 * 4 + 3] * b.comps[3 * 4 + 0], a.comps[1 * 4 + 0] * b.comps[0 * 4 + 1] + a.comps[1 * 4 + 1] * b.comps[1 * 4 + 1] + a.comps[1 * 4 + 2] * b.comps[2 * 4 + 1] + a.comps[1 * 4 + 3] * b.comps[3 * 4 + 1], a.comps[1 * 4 + 0] * b.comps[0 * 4 + 2] + a.comps[1 * 4 + 1] * b.comps[1 * 4 + 2] + a.comps[1 * 4 + 2] * b.comps[2 * 4 + 2] + a.comps[1 * 4 + 3] * b.comps[3 * 4 + 2], a.comps[1 * 4 + 0] * b.comps[0 * 4 + 3] + a.comps[1 * 4 + 1] * b.comps[1 * 4 + 3] + a.comps[1 * 4 + 2] * b.comps[2 * 4 + 3] + a.comps[1 * 4 + 3] * b.comps[3 * 4 + 3], 
		a.comps[2 * 4 + 0] * b.comps[0 * 4 + 0] + a.comps[2 * 4 + 1] * b.comps[1 * 4 + 0] + a.comps[2 * 4 + 2] * b.comps[2 * 4 + 0] + a.comps[2 * 4 + 3] * b.comps[3 * 4 + 0], a.comps[2 * 4 + 0] * b.comps[0 * 4 + 1] + a.comps[2 * 4 + 1] * b.comps[1 * 4 + 1] + a.comps[2 * 4 + 2] * b.comps[2 * 4 + 1] + a.comps[2 * 4 + 3] * b.comps[3 * 4 + 1], a.comps[2 * 4 + 0] * b.comps[0 * 4 + 2] + a.comps[2 * 4 + 1] * b.comps[1 * 4 + 2] + a.comps[2 * 4 + 2] * b.comps[2 * 4 + 2] + a.comps[2 * 4 + 3] * b.comps[3 * 4 + 2], a.comps[2 * 4 + 0] * b.comps[0 * 4 + 3] + a.comps[2 * 4 + 1] * b.comps[1 * 4 + 3] + a.comps[2 * 4 + 2] * b.comps[2 * 4 + 3] + a.comps[2 * 4 + 3] * b.comps[3 * 4 + 3], 
		a.comps[3 * 4 + 0] * b.comps[0 * 4 + 0] + a.comps[3 * 4 + 1] * b.comps[1 * 4 + 0] + a.comps[3 * 4 + 2] * b.comps[2 * 4 + 0] + a.comps[3 * 4 + 3] * b.comps[3 * 4 + 0], a.comps[3 * 4 + 0] * b.comps[0 * 4 + 1] + a.comps[3 * 4 + 1] * b.comps[1 * 4 + 1] + a.comps[3 * 4 + 2] * b.comps[2 * 4 + 1] + a.comps[3 * 4 + 3] * b.comps[3 * 4 + 1], a.comps[3 * 4 + 0] * b.comps[0 * 4 + 2] + a.comps[3 * 4 + 1] * b.comps[1 * 4 + 2] + a.comps[3 * 4 + 2] * b.comps[2 * 4 + 2] + a.comps[3 * 4 + 3] * b.comps[3 * 4 + 2], a.comps[3 * 4 + 0] * b.comps[0 * 4 + 3] + a.comps[3 * 4 + 1] * b.comps[1 * 4 + 3] + a.comps[3 * 4 + 2] * b.comps[2 * 4 + 3] + a.comps[3 * 4 + 3] * b.comps[3 * 4 + 3]};
}

template<typename T>
Matrix<T, 4, 4> div (const Matrix<T, 4, 4> &a, T b)
{
	return {a.comps[0] / b, a.comps[1] / b, a.comps[2] / b, a.comps[3] / b, a.comps[4] / b, a.comps[5] / b, a.comps[6] / b, a.comps[7] / b, a.comps[8] / b, a.comps[9] / b, a.comps[10] / b, a.comps[11] / b, a.comps[12] / b, a.comps[13] / b, a.comps[14] / b, a.comps[15] / b};
}

template<typename T>
Matrix<T, 4, 4> transposed (const Matrix<T, 4, 4> &m)
{
	return {m.comps[0], m.comps[4], m.comps[8], m.comps[12], m.comps[1], m.comps[5], m.comps[9], m.comps[13], m.comps[2], m.comps[6], m.comps[10], m.comps[14], m.comps[3], m.comps[7], m.comps[11], m.comps[15]};
}
