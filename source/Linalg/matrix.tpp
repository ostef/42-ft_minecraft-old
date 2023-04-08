#pragma once

template<typename T, int N, int M>
Matrix<T, N, M> add (const Matrix<T, N, M> &a, const Matrix<T, N, M> &b)
{
    Matrix<T, N, M> res;
    for_range (i, 0, N * M)
        res.comps[i] = a.comps[i] + b.comps[i];

    return res;
}

template<typename T, int N, int M>
Matrix<T, N, M> sub (const Matrix<T, N, M> &a, const Matrix<T, N, M> &b)
{
    Matrix<T, N, M> res;
    for_range (i, 0, N * M)
        res.comps[i] = a.comps[i] - b.comps[i];

    return res;
}

template<typename T, int N, int M>
Matrix<T, N, M> neg (const Matrix<T, N, M> &m)
{
    Matrix<T, N, M> res;
    for_range (i, 0, N * M)
        res.comps[i] = -m.comps[i];

    return res;
}

template<typename T, int N, int M>
Matrix<T, N, M> mul (const Matrix<T, N, M> &a, T b)
{
    Matrix<T, N, M> res;
    for_range (i, 0, N * M)
        res.comps[i] = a.comps[i] * b;

    return res;
}

template<typename T, int N, int M>
Vector<T, N> mul (const Matrix<T, N, M> &a, const Vector<T, M> &b)
{
    Vector<T, N> res;
    for_range (i, 0, N)
    {
        res.comps[i] = 0;
        for_range (k, 0, M)
        {
            res.comps[i] += a.comps[i * M + k] * b.comps[k];
        }
    }

    return res;
}

template<typename T, int N, int M>
Vector<T, M> mul (const Vector<T, N> &b, const Matrix<T, N, M> &a)
{
    Vector<T, M> res;
    for_range (j, 0, M)
    {
        res.comps[j] = 0;
        for_range (k, 0, N)
        {
            res.comps[j] += b.comps[k] * a.comps[k * M + j];
        }
    }

    return res;
}

template<typename T, int N, int M, int P>
Matrix<T, N, P> mul (const Matrix<T, N, M> &a, const Matrix<T, M, P> &b)
{
    Matrix<T, N, P> res;
    for_range (i, 0, N)
    {
        for_range (j, 0, P)
        {
            int index = i * P + j;
            res.comps[index] = 0;
            for_range (k, 0, M)
            {
                res.comps[index] += a.comps[i * M + k] * b.comps[k * P + j];
            }
        }
    }

    return res;
}

template<typename T, int N, int M>
Matrix<T, N, M> div (const Matrix<T, N, M> &a, T b)
{
    Matrix<T, N, M> res;
    for_range (i, 0, N * M)
        res.comps[i] = a.comps[i] / b;

    return res;
}

template<typename T, int N>
Matrix<T, N, N> transposed (const Matrix<T, N, N> &m)
{
    Matrix<T, N, N> res;
    for_range (i, 0, N)
    {
        for_range (j, 0, N)
        {
            res.comps[i * N + j] = m.comps[j * N + j];
        }
    }

    return res;
}

template<typename T, int N, int M>
Matrix<T, N, M> operator+ (const Matrix<T, N, M> &a, const Matrix<T, N, M> &b)
{
    return add (a, b);
}

template<typename T, int N, int M>
Matrix<T, N, M> operator- (const Matrix<T, N, M> &a, const Matrix<T, N, M> &b)
{
    return sub (a, b);
}

template<typename T, int N, int M>
Matrix<T, N, M> operator* (const Matrix<T, N, M> &a, T b)
{
    return mul (a, b);
}

template<typename T, int N, int M>
Vector<T, N> operator* (const Matrix<T, N, M> &a, const Vector<T, M> &b)
{
    return mul (a, b);
}

template<typename T, int N, int M>
Vector<T, M> operator* (const Vector<T, N> &a, const Matrix<T, N, M> &b)
{
    return mul (a, b);
}

template<typename T, int N, int M, int P>
Matrix<T, N, P> operator* (const Matrix<T, N, M> &a, const Matrix<T, M, P> &b)
{
    return mul (a, b);
}

template<typename T, int N, int M>
Matrix<T, N, M> operator/ (const Matrix<T, N, M> &a, T b)
{
    return div (a, b);
}

// Mat3

template<typename T>
Vector<T, 3> right_vector (const Matrix<T, 3, 3> &m)
{
    return {m.r00, m.r10, m.r20};
}

template<typename T>
Vector<T, 3> forward_vector (const Matrix<T, 3, 3> &m)
{
    return {m.r01, m.r11, m.r21};
}

template<typename T>
Vector<T, 3> up_vector (const Matrix<T, 3, 3> &m)
{
    return {m.r02, m.r12, m.r22};
}

template<typename T>
Matrix<T, 3, 3> inverse (const Matrix<T, 3, 3> &m)
{
    Vector<T, 3> a = {m.r00, m.r10, m.r20};
    Vector<T, 3> b = {m.r01, m.r11, m.r21};
    Vector<T, 3> c = {m.r02, m.r12, m.r22};

    auto r0 = cross (b, c);
    auto r1 = cross (c, a);
    auto r2 = cross (a, b);

    auto det = dot (r2, c);
    auto inv_det = 1 / det;

    Matrix<T, 3, 3> result;
    result.r0 = r0 * inv_det;
    result.r1 = r1 * inv_det;
    result.r2 = r2 * inv_det;

    return result;
}

template<typename T>
Matrix<T, 3, 3> mat3_from_quat (const Quaternion<T> &q)
{
    T x_x = q.x * q.x;
    T y_y = q.y * q.y;
    T z_z = q.z * q.z;
    T x_y = q.x * q.y;
    T x_z = q.x * q.z;
    T y_z = q.y * q.z;
    T w_x = q.w * q.x;
    T w_y = q.w * q.y;
    T w_z = q.w * q.z;

    Matrix<T, 3, 3> result;
    result.r00 = cast (T) (1 - 2 * (y_y + z_z));
    result.r01 = cast (T)     (2 * (x_y - w_z));
    result.r02 = cast (T)     (2 * (x_z + w_y));

    result.r10 = cast (T)     (2 * (x_y + w_z));
    result.r11 = cast (T) (1 - 2 * (x_x + z_z));
    result.r12 = cast (T)     (2 * (y_z - w_x));

    result.r20 = cast (T)     (2 * (x_z - w_y));
    result.r21 = cast (T)     (2 * (y_z + w_x));
    result.r22 = cast (T) (1 - 2 * (x_x + y_y));

    return result;
}

// Mat3x4

template<typename T>
Vector<T, 3> right_vector (const Matrix<T, 3, 4> &m)
{
    return {m.r00, m.r10, m.r20};
}

template<typename T>
Vector<T, 3> forward_vector (const Matrix<T, 3, 4> &m)
{
    return {m.r01, m.r11, m.r21};
}

template<typename T>
Vector<T, 3> up_vector (const Matrix<T, 3, 4> &m)
{
    return {m.r02, m.r12, m.r22};
}

template<typename T>
Vector<T, 3> translation_vector (const Matrix<T, 3, 4> &m)
{
    return {m.r03, m.r13, m.r23};
}

// Mat4

template<typename T>
Vector<T, 3> right_vector (const Matrix<T, 4, 4> &m)
{
    return {m.r00, m.r10, m.r20};
}

template<typename T>
Vector<T, 3> forward_vector (const Matrix<T, 4, 4> &m)
{
    return {m.r01, m.r11, m.r21};
}

template<typename T>
Vector<T, 3> up_vector (const Matrix<T, 4, 4> &m)
{
    return {m.r02, m.r12, m.r22};
}

template<typename T>
Vector<T, 3> translation_vector (const Matrix<T, 4, 4> &m)
{
    return {m.r03, m.r13, m.r23};
}

template<typename T>
Matrix<T, 4, 4> mat4_translate (const Vector<T, 3> &translation)
{
    return {
        1, 0, 0, translation.x,
        0, 1, 0, translation.y,
        0, 0, 1, translation.z,
        0, 0, 0, 1,
    };
}

template<typename T>
Matrix<T, 4, 4> mat4_scale (const Vector<T, 3> &scale)
{
    return {
        scale.x, 0, 0, 0,
        0, scale.y, 0, 0,
        0, 0, scale.z, 0,
        0, 0, 0, 1
    };
}

template<typename T>
Matrix<T, 4, 4> mat4_from_quat (const Quaternion<T> &q)
{
    T x_x = q.x * q.x;
    T y_y = q.y * q.y;
    T z_z = q.z * q.z;
    T x_y = q.x * q.y;
    T x_z = q.x * q.z;
    T y_z = q.y * q.z;
    T w_x = q.w * q.x;
    T w_y = q.w * q.y;
    T w_z = q.w * q.z;

    Matrix<T, 4, 4> result;
    result.r00 = cast (T) (1 - 2 * (y_y + z_z));
    result.r01 = cast (T)     (2 * (x_y - w_z));
    result.r02 = cast (T)     (2 * (x_z + w_y));
    result.r03 = 0;

    result.r10 = cast (T)     (2 * (x_y + w_z));
    result.r11 = cast (T) (1 - 2 * (x_x + z_z));
    result.r12 = cast (T)     (2 * (y_z - w_x));
    result.r13 = 0;

    result.r20 = cast (T)     (2 * (x_z - w_y));
    result.r21 = cast (T)     (2 * (y_z + w_x));
    result.r22 = cast (T) (1 - 2 * (x_x + y_y));
    result.r23 = 0;

    result.r30 = 0;
    result.r31 = 0;
    result.r32 = 0;
    result.r33 = 1;

    return result;
}

template<typename T>
Matrix<T, 4, 4> mat4_transform (const Vector<T, 3> &translation, const Quaternion<T> &rotation, const Vector<T, 3> &scale = {1, 1, 1})
{
    return mat4_translate (translation) * mat4_from_quat (rotation) * mat4_scale (scale);
}

template<typename T>
Matrix<T, 4, 4> mat4_perspective_projection (T fov_in_degrees, T aspect, T n, T f, bool left_handed = true)
{
    T view_z = left_handed ? 1 : -1;
    T t = tan (to_rads (fov_in_degrees) * 0.5) * n;
    T b = -t;
    T r = aspect * t;
    T l = -r;

    return {
        2 * n / (r - l),               0, -view_z * (r + l) / (r - l),      0,
                      0, 2 * n / (t - b), -view_z * (t + b) / (t - b),      0,
                      0,               0,                      view_z, -2 * n,
                      0,               0,                      view_z,      0
    };
}

// See: 1.7.5 Inverses of Small Matrices in 'Foundations of Game Development: Mathematics' by Eric Lengyel.
template<typename T>
Matrix<T, 4, 4> inverse (const Matrix<T, 4, 4> &m)
{
    Vector<T, 3> a = {m.r00, m.r10, m.r20};
    Vector<T, 3> b = {m.r01, m.r11, m.r21};
    Vector<T, 3> c = {m.r02, m.r12, m.r22};
    Vector<T, 3> d = {m.r03, m.r13, m.r23};

    auto x = m.r30;
    auto y = m.r31;
    auto z = m.r32;
    auto w = m.r33;

    auto s = cross (a, b);
    auto t = cross (c, d);
    auto u = a * y - b * x;
    auto v = c * w - d * z;

    auto det = dot (s, v) + dot (t, u);
    s /= det;
    t /= det;
    u /= det;
    v /= det;

    auto r0 = cross (b, v) + t * y;
    auto r1 = cross (v, a) - t * x;
    auto r2 = cross (d, u) + s * w;
    auto r3 = cross (u, c) - s * z;

    Matrix<T, 4, 4> result;
    result.r0 = {r0.x, r0.y, r0.z, -dot (b, t)};
    result.r1 = {r1.x, r1.y, r1.z,  dot (a, t)};
    result.r2 = {r2.x, r2.y, r2.z, -dot (d, s)};
    result.r3 = {r3.x, r3.y, r3.z,  dot (c, s)};

    return result;
}
