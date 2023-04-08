#pragma once

#include "Core.hpp"

#ifndef NO_BOUNDS_CHECKING
# define CHECK_BOUNDS(N) assert (i >= 0 && i < N, "Bounds check failed (got %lld, expected [0; %lld]).", i, N - 1)
#else
# define CHECK_BOUNDS(N)
#endif

#define DEFINE_ACCESS_OPERATOR(T, N, n)\
    inline\
    T &operator[] (s64 i)\
    {\
        CHECK_BOUNDS (N);\
\
        return n[i];\
    }\
\
    inline\
    const T &operator[] (s64 i) const\
    {\
        CHECK_BOUNDS (N);\
\
        return n[i];\
    }

// Vector

#define DEFINE_VECTOR_OPERATORS(T, N)\
    Vector<T, N> &operator+= (const Vector<T, N> &b) { *this = *this + b; return *this; }\
    Vector<T, N> &operator-= (const Vector<T, N> &b) { *this = *this - b; return *this; }\
    Vector<T, N> &operator*= (T b) { *this = *this * b; return *this; }\
    Vector<T, N> &operator/= (T b) { *this = *this / b; return *this; }

template<typename T, int N>
union Vector
{
    T comps[N] = {};

    DEFINE_ACCESS_OPERATOR (T, N, comps);
    DEFINE_VECTOR_OPERATORS (T, N);
};

template<typename T>
union Vector<T, 2>
{
    struct
    {
        T x, y;
    };

    T comps[2] = {};

    DEFINE_ACCESS_OPERATOR (T, 2, comps);
    DEFINE_VECTOR_OPERATORS (T, 2);
};

template<typename T>
union Vector<T, 3>
{
    struct
    {
        T x, y, z;
    };

    struct
    {
        T r, g, b;
    };

    struct
    {
        T yaw, pitch, roll;
    };

    Vector<T, 2> xy;

    T comps[3] = {};

    DEFINE_ACCESS_OPERATOR (T, 3, comps);
    DEFINE_VECTOR_OPERATORS (T, 3);
};

template<typename T>
union Vector<T, 4>
{
    struct
    {
        T x, y, z, w;
    };

    struct
    {
        T r, g, b, a;
    };

    Vector<T, 3> xyz;
    Vector<T, 3> rgb;

    T comps[4] = {};

    DEFINE_ACCESS_OPERATOR (T, 4, comps);
    DEFINE_VECTOR_OPERATORS (T, 4);
};

#undef DEFINE_VECTOR_OPERATORS

typedef Vector<s32, 2> Vec2i;
typedef Vector<s64, 2> Vec2l;
typedef Vector<f32, 2> Vec2f;
typedef Vector<f64, 2> Vec2d;

typedef Vector<s32, 3> Vec3i;
typedef Vector<s64, 3> Vec3l;
typedef Vector<f32, 3> Vec3f;
typedef Vector<f64, 3> Vec3d;

typedef Vector<s32, 4> Vec4i;
typedef Vector<s64, 4> Vec4l;
typedef Vector<f32, 4> Vec4f;
typedef Vector<f64, 4> Vec4d;

// Matrix

#define DEFINE_MATRIX_OPERATORS(T, N, M)\
    Matrix<T, N, M> &operator+= (const Matrix<T, N, M> &b) { *this = *this + b; return *this; }\
    Matrix<T, N, M> &operator-= (const Matrix<T, N, M> &b) { *this = *this - b; return *this; }\
    Matrix<T, N, M> &operator*= (T b) { *this = *this * b; return *this; }\
    Matrix<T, N, M> &operator/= (T b) { *this = *this / b; return *this; }

template<typename T, int N, int M>
union Matrix
{
    typedef Vector<T, M> Row;

    Row rows[N];

    T comps[N * M] = {};

    DEFINE_ACCESS_OPERATOR (Row, N, rows);
    DEFINE_MATRIX_OPERATORS (T, N, M);
};

template<typename T>
union Matrix<T, 3, 3>
{
    typedef Vector<T, 3> Row;

    struct
    {
        T r00, r01, r02;
        T r10, r11, r12;
        T r20, r21, r22;
    };

    struct
    {
        Row r0;
        Row r1;
        Row r2;
    };

    Row rows[3];

    f32 comps[3 * 3] = {
        1,0,0,
        0,1,0,
        0,0,1
    };

    DEFINE_ACCESS_OPERATOR (Row, 3, rows);
    DEFINE_MATRIX_OPERATORS (T, 3, 3);
};

template<typename T>
union Matrix<T, 3, 4>
{
    typedef Vector<T, 4> Row;

    struct
    {
        T r00, r01, r02, r03;
        T r10, r11, r12, r13;
        T r20, r21, r22, r23;
    };

    struct
    {
        Row r0;
        Row r1;
        Row r2;
    };

    Row rows[3];

    f32 comps[3 * 4] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0
    };

    DEFINE_ACCESS_OPERATOR (Row, 3, rows);
    DEFINE_MATRIX_OPERATORS (T, 3, 4);
};

template<typename T>
union Matrix<T, 4, 4>
{
    typedef Vector<T, 4> Row;

    struct
    {
        T r00, r01, r02, r03;
        T r10, r11, r12, r13;
        T r20, r21, r22, r23;
        T r30, r31, r32, r33;
    };

    struct
    {
        Row r0;
        Row r1;
        Row r2;
        Row r3;
    };

    Row rows[4];

    T comps[4 * 4] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };

    DEFINE_ACCESS_OPERATOR (Row, 4, rows);
    DEFINE_MATRIX_OPERATORS (T, 4, 4);
};

#undef DEFINE_MATRIX_OPERATORS

typedef Matrix<f32, 3, 3> Mat3f;
typedef Matrix<f64, 3, 3> Mat3d;

typedef Matrix<f32, 3, 4> Mat3x4f;
typedef Matrix<f64, 3, 4> Mat3x4d;

typedef Matrix<f32, 4, 4> Mat4f;
typedef Matrix<f64, 4, 4> Mat4d;

// Quaternion

template<typename T>
union Quaternion
{
    struct
    {
        T x, y, z, w;
    };

    Vector<T, 3> vec;
    Vector<T, 4> as_vec4;

    T comps[4] = {0,0,0,1};

    DEFINE_ACCESS_OPERATOR (T, 4, comps);

    Quaternion<T> &operator+= (const Quaternion<T> &b)
    {
        *this = *this + b;
        return *this;
    }

    Quaternion<T> &operator-= (const Quaternion<T> &b)
    {
        *this = *this - b;
        return *this;
    }

    Quaternion<T> &operator*= (T b)
    {
        *this = *this * b;
        return *this;
    }

    Quaternion<T> &operator*= (const Quaternion<T> &b)
    {
        *this = *this * b;
        return *this;
    }

    Quaternion<T> &operator/= (T b)
    {
        *this = *this / b;
        return *this;
    }
};

typedef Quaternion<f32> Quatf;
typedef Quaternion<f64> Quatd;


#undef DEFINE_ACCESS_OPERATOR
#undef CHECK_BOUNDS

#include "Linalg/vector.tpp"
#include "Linalg/matrix.tpp"
#include "Linalg/quaternion.tpp"
#include "Linalg/generated.tpp"
