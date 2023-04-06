#pragma once

template<typename T>
Quaternion<T> add (const Quaternion<T> &a, const Quaternion<T> &b)
{
	return add (a.as_vec4, b.as_vec4);
}

template<typename T>
Quaternion<T> sub (const Quaternion<T> &a, const Quaternion<T> &b)
{
	return sub (a.as_vec4, b.as_vec4);
}

template<typename T>
Quaternion<T> neg (const Quaternion<T> &q)
{
	return neg (q.as_vec4);
}

template<typename T>
Quaternion<T> mul (const Quaternion<T> &a, const Quaternion<T> &b)
{
	auto w1 = a.w;
	auto x1 = a.x;
	auto y1 = a.y;
	auto z1 = a.z;

	auto w2 = b.w;
	auto x2 = b.x;
	auto y2 = b.y;
	auto z2 = b.z;

	return {w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2,
		w1 * y2 - x1 * z2 + w2 * y1 + z1 * x2,
		w1 * z2 + x1 * y2 - y1 * x2 + w2 * z1,
		w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2};
}

template<typename T>
Quaternion<T> div (const Quaternion<T> &a, T b)
{
	return div (a.as_vec4, b);
}

template<typename T>
T dot (const Quaternion<T> &a, const Quaternion<T> &b)
{
	return dot (a.as_vec4, b.as_vec4);
}

template<typename T, typename E>
bool approx_zero (const Quaternion<T> &q, E epsilon = 0.00001)
{
	return approx_zero (q.as_vec4, epsilon);
}

template<typename T, typename E>
bool approx_equals (const Quaternion<T> &a, const Quaternion<T> &b, E epsilon = 0.00001)
{
	return approx_equals (a.as_vec4, b.as_vec4, epsilon);
}

template<typename T>
bool equals (const Quaternion<T> &a, const Quaternion<T> &b)
{
	return equals (a.as_vec4, b.as_vec4);
}

template<typename T>
Quaternion<T> operator+ (const Quaternion<T> &a, const Quaternion<T> &b)
{
	return add (a, b);
}

template<typename T>
Quaternion<T> operator- (const Quaternion<T> &a, const Quaternion<T> &b)
{
	return sub (a, b);
}

template<typename T>
Quaternion<T> operator- (const Quaternion<T> &q)
{
	return neg (q);
}

template<typename T>
Quaternion<T> operator* (const Quaternion<T> &a, T b)
{
	return mul (a, b);
}

template<typename T>
Quaternion<T> operator/ (const Quaternion<T> &a, T b)
{
	return div (a, b);
}

template<typename T>
bool operator== (const Quaternion<T> &a, const Quaternion<T> &b)
{
	return equals (a, b);
}

template<typename T>
bool operator!= (const Quaternion<T> &a, const Quaternion<T> &b)
{
	return !equals (a, b);
}

template<typename T>
T sqrd_length (const Quaternion<T> &q)
{
	return sqrd_length (q.as_vec4);
}

template<typename T>
T length (const Quaternion<T> &q)
{
	return length (a.as_vec4, b.as_vec4);
}

template<typename T>
Quaternion<T> normalized (const Quaternion<T> &q, const Quaternion<T> &fallback = {})
{
	return normalized (q.as_vec4, fallback.as_vec4);
}

template<typename T>
Quaternion<T> inverse (const Quaternion<T> &q, T epsilon = 0.00001)
{
	if (approx_zero (q, epsilon))
		return q;

	auto sq = dot (q, q);

	return {-q.x / sq, -q.y / sq, -q.z / sq, q.w / sq};
}

template<typename T>
Quaternion<T> conjugate (const Quaternion<T> &q)
{
	return {-q.x, -q.y, -q.z, q.w};
}

template<typename T>
T angle (const Quaternion<T> &q)
{
	return acos (q.w) * 2;
}

template<typename T>
T angle (const Quaternion<T> &a, const Quaternion<T> &b)
{
	return angle (b * inverse (a));
}

template<typename T>
Quaternion<T> nlerp (const Quaternion<T> &a, const Quaternion<T> &b, T t)
{
	return normalized (lerp (a.as_vec4, b.as_vec4, t));
}

template<typename T>
Quaternion<T> nlerp_shortest (const Quaternion<T> &a, const Quaternion<T> &b, T t)
{

	return nlerp (a, (dot (a, b) < 0 ? -b : b), t);
}

template<typename T>
Vector<T, 3> rotate (const Vector<T, 3> &v, const Quaternion<T> &q)
{
	return (q * {vec.x, vec.y, vec.z, 0} * inverse (q)).vec;
}
