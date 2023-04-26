#pragma once

#define IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS

// We do our own stb sprintf wrapper because ImGui includes
// the implementation of stb sprintf if we define IMGUI_USE_STB_SPRINTF
// We already implement stb sprintf ourselves in Core.cpp so this causes problems

#include <stb_sprintf.h>

inline
static int ImFormatString(char* buf, size_t buf_size, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int w = stbsp_vsnprintf(buf, (int)buf_size, fmt, args);

    va_end(args);
    if (buf == NULL)
        return w;
    if (w == -1 || w >= (int)buf_size)
        w = (int)buf_size - 1;
    buf[w] = 0;
    return w;
}

inline
static int ImFormatStringV(char* buf, size_t buf_size, const char* fmt, va_list args)
{
    int w = stbsp_vsnprintf(buf, (int)buf_size, fmt, args);

    if (buf == NULL)
        return w;
    if (w == -1 || w >= (int)buf_size)
        w = (int)buf_size - 1;
    buf[w] = 0;
    return w;
}

#define IMGUI_DEFINE_MATH_OPERATORS
#include <misc/single_file/imgui_single_file.h>

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include <glad/gl.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

namespace ImGuiExt
{
    bool IsItemDoubleClicked (ImGuiMouseButton mouse_button = 0);

    float HermiteCubicCalculate (float x0, float y0, float der0, float x1, float y1, float der1, float t);

    void AddHermiteCubic (ImDrawList *draw_list,
        const ImVec2 &offset, const ImVec2 &scale,
        float x0, float y0, float der0, float x1, float y1, float der1,
        ImU32 color, float thickness = 1.0f, int num_segments = 64);

    #define ImGuiExt_BezierSpline_PointCountFromCurveCount(n) (3 * (n) + 1)
    #define ImGuiExt_BezierSpline_CurveCountFromPointCount(n) (((n) - 1) / 3)

    bool BezierSplineEditor (const char *str_id, const ImVec2 &size,
        int max_control_points, int *control_point_count, ImVec2 *control_points);
}
