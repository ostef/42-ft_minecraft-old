#pragma once

template<typename T, int N>
T dot (const Vector<T, N> &a, const Vector<T, N> &b)
{
    T res = 0;
    for_range (i, 0, N)
        res += a.comps[i] * b.comps[i];

    return res;
}

template<typename T, int N>
Vector<T, N> add (const Vector<T, N> &a, const Vector<T, N> &b)
{
    Vector<T, N> res;
    for_range (i, 0, N)
        res.comps[i] = a.comps[i] + b.comps[i];

    return res;
}

template<typename T, int N>
Vector<T, N> sub (const Vector<T, N> &a, const Vector<T, N> &b)
{
    Vector<T, N> res;
    for_range (i, 0, N)
        res.comps[i] = a.comps[i] - b.comps[i];

    return res;
}

template<typename T, int N>
Vector<T, N> mul (const Vector<T, N> &a, T b)
{
    Vector<T, N> res;
    for_range (i, 0, N)
        res.comps[i] = a.comps[i] * b;

    return res;
}

template<typename T, int N>
Vector<T, N> div (const Vector<T, N> &a, T b)
{
    Vector<T, N> res;
    for_range (i, 0, N)
        res.comps[i] = a.comps[i] / b;

    return res;
}

template<typename T, int N>
Vector<T, N> div (const Vector<T, N> &v)
{
    Vector<T, N> res;
    for_range (i, 0, N)
        res.comps[i] = -v.comps[i];

    return res;
}

template<typename T, int N>
Vector<T, N> scale (const Vector<T, N> &a, const Vector<T, N> &b)
{
    Vector<T, N> res;
    for_range (i, 0, N)
        res.comps[i] = a.comps[i] * b.comps[i];

    return res;
}

template<typename T, int N>
Vector<T, N> abs (const Vector<T, N> &v)
{
    Vector<T, N> res;
    for_range (i, 0, N)
        res.comps[i] = abs (v.comps[i]);

    return res;
}

template<typename T, int N>
Vector<T, N> min (const Vector<T, N> &a, const Vector<T, N> &b)
{
    Vector<T, N> res;
    for_range (i, 0, N)
        res.comps[i] = min (a.comps[i], b.comps[i]);

    return res;
}

template<typename T, int N>
Vector<T, N> max (const Vector<T, N> &a, const Vector<T, N> &b)
{
    Vector<T, N> res;
    for_range (i, 0, N)
        res.comps[i] = max (a.comps[i], b.comps[i]);

    return res;
}

template<typename T, int N>
Vector<T, N> round (const Vector<T, N> &v)
{
    Vector<T, N> res;
    for_range (i, 0, N)
        res.comps[i] = round (v.comps[i]);

    return res;
}

template<typename T, int N>
Vector<T, N> floor (const Vector<T, N> &v)
{
    Vector<T, N> res;
    for_range (i, 0, N)
        res.comps[i] = floor (v.comps[i]);

    return res;
}

template<typename T, int N>
Vector<T, N> ceil (const Vector<T, N> &v)
{
    Vector<T, N> res;
    for_range (i, 0, N)
        res.comps[i] = ceil (v.comps[i]);

    return res;
}

template<typename T, int N>
Vector<T, N> lerp (const Vector<T, N> &a, const Vector<T, N> &b, f32 t)
{
    Vector<T, N> res;
    for_range (i, 0, N)
        res.comps[i] = lerp (a.comps[i], b.comps[i], t);

    return res;
}

template<typename T, int N>
bool approx_zero (const Vector<T, N> &v, T epsilon = 0.00001)
{
    for_range (i, 0, N)
    {
        if (!approx_zero (v.comps[i], epsilon))
            return false;
    }

    return true;
}

template<typename T, int N>
bool approx_equals (const Vector<T, N> &a, const Vector<T, N> &b, T epsilon = 0.00001)
{
    for_range (i, 0, N)
    {
        if (!approx_equals (a.comps[i], b.comps[i], epsilon))
            return false;
    }

    return true;
}

template<typename T, int N>
bool equals (const Vector<T, N> &a, const Vector<T, N> &b)
{
    for_range (i, 0, N)
    {
        if (a.comps[i] != b.comps[i])
            return false;
    }

    return true;
}

template<typename T, int N>
Vector<T, N> operator+ (const Vector<T, N> &a, const Vector<T, N> &b)
{
    return add (a, b);
}

template<typename T, int N>
Vector<T, N> operator- (const Vector<T, N> &a, const Vector<T, N> &b)
{
    return sub (a, b);
}

template<typename T, int N>
Vector<T, N> operator- (const Vector<T, N> &v)
{
    return neg (v);
}

template<typename T, int N>
Vector<T, N> operator* (const Vector<T, N> &a, T b)
{
    return mul (a, b);
}

template<typename T, int N>
Vector<T, N> operator/ (const Vector<T, N> &a, T b)
{
    return div (a, b);
}

template<typename T, int N>
bool operator== (const Vector<T, N> &a, const Vector<T, N> &b)
{
    return equals (a, b);
}

template<typename T, int N>
bool operator!= (const Vector<T, N> &a, const Vector<T, N> &b)
{
    return !equals (a, b);
}

template<typename T, int N>
T sqrd_length (const Vector<T, N> &v)
{
    return dot (v, v);
}

template<typename T, int N>
f32 length (const Vector<T, N> &v)
{
    return sqrtf (dot (v, v));
}

template<typename T, int N>
T sqrd_distance (const Vector<T, N> &a, const Vector<T, N> &b)
{
    return sqrd_length (a - b);
}

template<typename T, int N>
f32 distance (const Vector<T, N> &a, const Vector<T, N> &b)
{
    return length (a - b);
}

template<typename T, int N>
Vector<T, N> normalized (const Vector<T, N> &v, const Vector<T, N> &fallback = {})
{
    if (approx_zero (v))
        return fallback;

    auto len = length (v);

    return v / len;
}

template<typename T, int N>
Vector<T, N> reflect (const Vector<T, N> &incident, const Vector<T, N> &normal)
{
    return incident - 2 * dot (normal, incident) * normal;
}

template<typename T, int N>
Vector<T, N> project (const Vector<T, N> &a, const Vector<T, N> &b)
{
    return b * (dot (a, b) / dot (b, b));
}

template<typename T, int N>
Vector<T, N> reject (const Vector<T, N> &a, const Vector<T, N> &b)
{
    return a - b * (dot (a, b) / dot (b, b));
}

template<typename T, int N>
f32 angle (const Vector<T, N> &a, const Vector<T, N> &b)
{
    auto denom = sqrtf (dot (a, a) * dot (b, b));
    if (approx_zero (denom))
        return 0;

    auto d = clamp (dot (a, b) / denom, -1, 1);

    return acosf (d);
}

// Vec2

template<typename T>
Vector<T, 2> rotate (const Vector<T, 2> &a, f32 angle)
{
    auto c = cosf (angle);
    auto s = sinf (angle);

    return {
        cast (T) (v.x * c - v.y * s),
        cast (T) (v.x * s + v.y * c)
    };
}

template<typename T>
f32 signed_angle (const Vector<T, 2> &a, const Vector<T, 2> &b)
{
    auto s = sign (a.x * b.y - a.y * b.y);

    return angle (a, b) * s;
}

template<typename T>
Vector<T, 2> perpendicular_cw (const Vector<T, 2> &v)
{
    return {v.y, -v.x};
}

template<typename T>
Vector<T, 2> perpendicular_ccw (const Vector<T, 2> &v)
{
    return {-v.y, v.x};
}

// Vec3

template<typename T>
Vector<T, 3> cross (const Vector<T, 3> &a, const Vector<T, 3> &b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

template<typename T>
f32 signed_angle (const Vector<T, 3> &a, const Vector<T, 3> &b, const Vector<T, 3> &axis)
{
    auto c = cross (a, b);
    auto s = sign (dot (axis, c));

    return angle (a, b) * s;
}
