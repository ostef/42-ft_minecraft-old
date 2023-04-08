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
