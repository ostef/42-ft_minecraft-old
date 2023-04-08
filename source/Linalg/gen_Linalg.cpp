// This program generates code for Vector, Matrix functions
// Linalg should work fine without generating these functions, as they
// are specialization of functions that are already written. What we
// generate here is code that is optimized for certain template parameters
// (mostly loop unrolled versions of other functions).

#include "Core.cpp"

#define ap string_builder_append
#define ln string_builder_append_line

void unroll (String_Builder *b, int N, const char *sep, const char *fmt_str)
{
    for_range (i, 0, N)
    {
        if (i != 0)
            ap (b, sep);
        ap (b, fmt_str, i, i, i, i, i, i, i, i, i, i);
    }
}

void generate_vector_dot (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "T dot (const Vector<T, %d> &a, const Vector<T, %d> &b)\n{", N, N);
    ap (b, "    return ");
    unroll (b, N, " + ", "a.comps[%d] * b.comps[%d]");
    ln (b, ";");
    ln (b, "}");
}

void generate_vector_add (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> add (const Vector<T, %d> &a, const Vector<T, %d> &b)\n{", N, N, N);
    ap (b, "    return {");
    unroll (b, N, ", ", "a.comps[%d] + b.comps[%d]");
    ln (b, "};");
    ln (b, "}");
}

void generate_vector_sub (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> sub (const Vector<T, %d> &a, const Vector<T, %d> &b)\n{", N, N, N);
    ap (b, "    return {");
    unroll (b, N, ", ", "a.comps[%d] - b.comps[%d]");
    ln (b, "};");
    ln (b, "}");
}

void generate_vector_mul (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> mul (const Vector<T, %d> &a, T b)\n{", N, N, N);
    ap (b, "    return {");
    unroll (b, N, ", ", "a.comps[%d] * b");
    ln (b, "};");
    ln (b, "}");
}

void generate_vector_div (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> div (const Vector<T, %d> &a, T b)\n{", N, N, N);
    ap (b, "    return {");
    unroll (b, N, ", ", "a.comps[%d] / b");
    ln (b, "};");
    ln (b, "}");
}

void generate_vector_neg (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> neg (const Vector<T, %d> &v)\n{", N, N, N);
    ap (b, "    return {");
    unroll (b, N, ", ", "-v.comps[%d]");
    ln (b, "};");
    ln (b, "}");
}

void generate_vector_scale (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> scale (const Vector<T, %d> &a, const Vector<T, %d> &b)\n{", N, N, N);
    ap (b, "    return {");
    unroll (b, N, ", ", "a.comps[%d] * b.comps[%d]");
    ln (b, "};");
    ln (b, "}");
}

void generate_vector_abs (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> abs (const Vector<T, %d> &v)\n{", N, N, N);
    ap (b, "    return {");
    unroll (b, N, ", ", "abs (v.comps[%d])");
    ln (b, "};");
    ln (b, "}");
}

void generate_vector_min (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> min (const Vector<T, %d> &a, const Vector<T, %d> &b)\n{", N, N, N);
    ap (b, "    return {");
    unroll (b, N, ", ", "min (a.comps[%d], b.comps[%d])");
    ln (b, "};");
    ln (b, "}");
}

void generate_vector_max (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> max (const Vector<T, %d> &a, const Vector<T, %d> &b)\n{", N, N, N);
    ap (b, "    return {");
    unroll (b, N, ", ", "max (a.comps[%d], b.comps[%d])");
    ln (b, "};");
    ln (b, "}");
}

void generate_vector_round (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> round (const Vector<T, %d> &v)\n{", N, N, N);
    ap (b, "    return {");
    unroll (b, N, ", ", "round (v.comps[%d])");
    ln (b, "};");
    ln (b, "}");
}

void generate_vector_floor (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> floor (const Vector<T, %d> &v)\n{", N, N, N);
    ap (b, "    return {");
    unroll (b, N, ", ", "floor (v.comps[%d])");
    ln (b, "};");
    ln (b, "}");
}

void generate_vector_ceil (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> ceil (const Vector<T, %d> &v)\n{", N, N, N);
    ap (b, "    return {");
    unroll (b, N, ", ", "ceil (v.comps[%d])");
    ln (b, "};");
    ln (b, "}");
}

void generate_vector_clamp (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> clamp (const Vector<T, %d> &v, const Vector<T, %d> &a, const Vector<T, %d> &b)\n{", N, N, N, N);
    ap (b, "    return {");
    unroll (b, N, ", ", "clamp (v.comps[%d], a.comps[%d], b.comps[%d])");
    ln (b, "};");
    ln (b, "}");
}

void generate_vector_lerp (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> lerp (const Vector<T, %d> &a, const Vector<T, %d> &b, f32 t)\n{", N, N, N, N);
    ap (b, "    return {");
    unroll (b, N, ", ", "lerp (a.comps[%d], b.comps[%d], t)");
    ln (b, "};");
    ln (b, "}");
}

void generate_vector_approx_zero (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "bool approx_zero (const Vector<T, %d> &v, T epsilon = 0.00001)\n{", N, N, N, N);
    ap (b, "    return ");
    unroll (b, N, " && ", "approx_zero (v.comps[%d], epsilon)");
    ln (b, ";");
    ln (b, "}");
}

void generate_vector_approx_equals (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "bool approx_equals (const Vector<T, %d> &a, const Vector<T, %d> &b, T epsilon = 0.00001)\n{", N, N, N, N);
    ap (b, "    return ");
    unroll (b, N, " && ", "approx_equals (a.comps[%d], b.comps[%d], epsilon)");
    ln (b, ";");
    ln (b, "}");
}

void generate_vector_equals (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "bool equals (const Vector<T, %d> &a, const Vector<T, %d> &b)\n{", N, N, N, N);
    ap (b, "    return ");
    unroll (b, N, " && ", "a.comps[%d] == b.comps[%d]");
    ln (b, ";");
    ln (b, "}");
}

void generate_vector_functions (String_Builder *b, int N)
{
    generate_vector_dot (b, N);

    ap (b, "\n");
    generate_vector_add (b, N);

    ap (b, "\n");
    generate_vector_sub (b, N);

    ap (b, "\n");
    generate_vector_mul (b, N);

    ap (b, "\n");
    generate_vector_div (b, N);

    ap (b, "\n");
    generate_vector_scale (b, N);

    ap (b, "\n");
    generate_vector_neg (b, N);

    ap (b, "\n");
    generate_vector_abs (b, N);

    ap (b, "\n");
    generate_vector_min (b, N);

    ap (b, "\n");
    generate_vector_max (b, N);

    ap (b, "\n");
    generate_vector_clamp (b, N);

    ap (b, "\n");
    generate_vector_round (b, N);

    ap (b, "\n");
    generate_vector_floor (b, N);

    ap (b, "\n");
    generate_vector_ceil (b, N);

    ap (b, "\n");
    generate_vector_lerp (b, N);

    ap (b, "\n");
    generate_vector_approx_zero (b, N);

    ap (b, "\n");
    generate_vector_approx_equals (b, N);

    ap (b, "\n");
    generate_vector_equals (b, N);
}

void unroll_double (String_Builder *b, int N, int M, const char *sep, const char *fmt_str)
{
    for_range (i, 0, N)
    {
        for_range (j, 0, M)
        {
            if (i != 0 || j != 0)
                ap (b, sep);

            int index = i * M + j;
            ap (b, fmt_str, index, index, index, index, index, index, index, index);
        }
    }
}

void generate_matrix_add (String_Builder *b, int N, int M)
{
    ln (b, "template<typename T>");
    ln (b, "Matrix<T, %d, %d> add (const Matrix<T, %d, %d> &a, const Matrix<T, %d, %d> &b)\n{", N, M, N, M, N, M);
    ap (b, "    return {");
    unroll_double (b, N, M, ", ", "a.comps[%d] + b.comps[%d]");
    ln (b, "};");
    ln (b, "}");
}

void generate_matrix_sub (String_Builder *b, int N, int M)
{
    ln (b, "template<typename T>");
    ln (b, "Matrix<T, %d, %d> sub (const Matrix<T, %d, %d> &a, const Matrix<T, %d, %d> &b)\n{", N, M, N, M, N, M);
    ap (b, "    return {");
    unroll_double (b, N, M, ", ", "a.comps[%d] - b.comps[%d]");
    ln (b, "};");
    ln (b, "}");
}

void generate_matrix_mulf (String_Builder *b, int N, int M)
{
    ln (b, "template<typename T>");
    ln (b, "Matrix<T, %d, %d> mul (const Matrix<T, %d, %d> &a, T b)\n{", N, M, N, M, N, M);
    ap (b, "    return {");
    unroll_double (b, N, M, ", ", "a.comps[%d] * b");
    ln (b, "};");
    ln (b, "}");
}

void generate_matrix_mulv (String_Builder *b, int N, int M)
{
    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> mul (const Matrix<T, %d, %d> &a, const Vector<T, %d> &b)\n{", N, N, M, M);
    ap (b, "    return {");

    for_range (i, 0, N)
    {
        if (i != 0)
            ap (b, ",\n        ");

        for_range (k, 0, M)
        {
            if (k != 0)
                ap (b, " + ");
            ap (b, "a.comps[%d * %d + %d] * b.comps[%d]", i, M, k, k);
        }
    }

    ln (b, "};");
    ln (b, "}\n");

    ln (b, "template<typename T>");
    ln (b, "Vector<T, %d> mul (const Vector<T, %d> &a, const Matrix<T, %d, %d> &b)\n{", M, N, N, M);
    ap (b, "    return {");

    for_range (j, 0, M)
    {
        if (j != 0)
            ap (b, ",\n        ");

        for_range (k, 0, N)
        {
            if (k != 0)
                ap (b, " + ");
            ap (b, "a.comps[%d] * b.comps[%d * %d + %d]", k, k, M, j);
        }
    }

    ln (b, "};");
    ln (b, "}");
}

void generate_matrix_mul (String_Builder *b, int N, int M, int P)
{
    ln (b, "template<typename T>");
    ln (b, "Matrix<T, %d, %d> mul (const Matrix<T, %d, %d> &a, const Matrix<T, %d, %d> &b)\n{", N, P, N, M, M, P);
    ap (b, "    return {");

    for_range (i, 0, N)
    {
        if (i != 0)
            ap (b, "\n        ");

        for_range (j, 0, P)
        {

            int index = i * P + j;
            for_range (k, 0, M)
            {
                if (k != 0)
                    ap (b, " + ");
                ap (b, "a.comps[%d * %d + %d] * b.comps[%d * %d + %d]", i, M, k, k, P, j);
            }

            if (i != N - 1 || j != P - 1)
                ap (b, ", ");
        }
    }

    ln (b, "};");
    ln (b, "}");
}

void generate_matrix_divf (String_Builder *b, int N, int M)
{
    ln (b, "template<typename T>");
    ln (b, "Matrix<T, %d, %d> div (const Matrix<T, %d, %d> &a, T b)\n{", N, M, N, M, N, M);
    ap (b, "    return {");
    unroll_double (b, N, M, ", ", "a.comps[%d] / b");
    ln (b, "};");
    ln (b, "}");
}

void generate_matrix_transposed (String_Builder *b, int N)
{
    ln (b, "template<typename T>");
    ln (b, "Matrix<T, %d, %d> transposed (const Matrix<T, %d, %d> &m)\n{", N, N, N, N);
    ap (b, "    return {");

    for_range (i, 0, N)
    {
        for_range (j, 0, N)
        {
            if (i != 0 || j != 0)
                ap (b, ", ");
            ap (b, "m.comps[%d]", j * N + i);
        }
    }

    ln (b, "};");
    ln (b, "}");
}

void generate_matrix_functions (String_Builder *b, int N, int M)
{
    generate_matrix_add (b, N, M);

    ap (b, "\n");
    generate_matrix_sub (b, N, M);

    ap (b, "\n");
    generate_matrix_mulf (b, N, M);

    ap (b, "\n");
    generate_matrix_mulv (b, N, M);

    ap (b, "\n");
    generate_matrix_mul (b, N, M, N);

    if (M != N)
    {
        ap (b, "\n");
        generate_matrix_mul (b, N, M, M);
    }

    ap (b, "\n");
    generate_matrix_divf (b, N, M);

    if (N == M)
    {
        ap (b, "\n");
        generate_matrix_transposed (b, N);
    }
}

int main ()
{
    platform_init ();
    crash_handler_init ();

    String_Builder builder;
    string_builder_init (&builder, heap_allocator ());

    ln (&builder, "#pragma once");

    ln (&builder, "\n// Vec2\n");
    generate_vector_functions (&builder, 2);

    ln (&builder, "\n// Vec3\n");
    generate_vector_functions (&builder, 3);

    ln (&builder, "\n// Vec4\n");
    generate_vector_functions (&builder, 4);

    ln (&builder, "\n// Mat3\n");
    generate_matrix_functions (&builder, 3, 3);

    ln (&builder, "\n// Mat3x4\n");
    generate_matrix_functions (&builder, 3, 4);

    ln (&builder, "\n// Mat4\n");
    generate_matrix_functions (&builder, 4, 4);

    auto result = string_builder_build_cstr (&builder, heap_allocator ());
    print (result);

    return 0;
}
