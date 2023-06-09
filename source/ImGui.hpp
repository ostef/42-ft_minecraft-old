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

    struct PanZoomViewParams
    {
        ImVec2 XOffsetRange;
        ImVec2 YOffsetRange;
        ImVec2 ScaleRange;
        float GridCellSize;
    };

    static const PanZoomViewParams PanZoomViewParams_Default = PanZoomViewParams{
        {-1.0f, 1.0f},
        {-1.0f, 1.0f},
        {0.01f, 500.0f},
        0.1f,
    };

    typedef int PanZoomViewFlags;

    enum
    {
        PanZoomViewFlags_None = 0,
        PanZoomViewFlags_NoGrid = 0x01,
        PanZoomViewFlags_PanOnSpace = 0x02,
        PanZoomViewFlags_PanOnLeftMouse = 0x04,
        PanZoomViewFlags_PanOnMiddleMouse = 0x08,
        PanZoomViewFlags_PanOnRightMouse = 0x10,

        PanZoomViewFlags_Default = PanZoomViewFlags_PanOnMiddleMouse,
    };

    bool BeginPanZoomView (const char *str_id, const ImVec2 &size, ImVec2 *offset, float *scale,
        bool border = false, PanZoomViewFlags flags = PanZoomViewFlags_Default, ImGuiWindowFlags window_flags = 0,
        const PanZoomViewParams &extra = PanZoomViewParams_Default);

    void EndPanZoomView ();

    void AddDottedLine (ImDrawList *draw_list, const ImVec2 &a, const ImVec2 &b, float spacing, ImU32 color, float thickness = 1.0f);
    void AddDashedLine (ImDrawList *draw_list, const ImVec2 &a, const ImVec2 &b, float line_len, float spacing, ImU32 color, float thickness = 1.0f);

    float HermiteCubicCalculate (float x0, float y0, float der0, float x1, float y1, float der1, float t);

    void AddHermiteCubic (ImDrawList *draw_list,
        float x0, float y0, float der0, float x1, float y1, float der1,
        ImU32 color, float thickness = 1.0f, int num_segments = 64);

    void PanZoomToWindow (const ImVec2 &offset, float scale, ImVec2 *inout_pos, ImVec2 *inout_size);
    void WindowToPanZoom (const ImVec2 &offset, float scale, ImVec2 *inout_pos, ImVec2 *inout_size);

    struct HermiteSplineParams
    {
        PanZoomViewParams ViewParams;
        ImVec2 XRange;
        ImVec2 YRange;
    };

    static const HermiteSplineParams HermiteSplineParams_Default = HermiteSplineParams{
        PanZoomViewParams_Default,
        {0.0f, 1.0f},
        {0.0f, 1.0f},
    };

    typedef int HermiteSplineFlags;

    enum
    {
        HermiteSplineFlags_None = 0,
        HermiteSplineFlags_NoGrid = 0x01,
        HermiteSplineFlags_NoPanNoZoom = 0x02,
        HermiteSplineFlags_NoMetricsOnXAxis = 0x04,
        HermiteSplineFlags_NoMetricsOnYAxis = 0x08,
    };

    bool BeginHermiteSpline (const char *str_id, ImVec2 *offset, float *scale, const ImVec2 &size = {},
        bool border = true, HermiteSplineFlags flags = 0, const HermiteSplineParams &extra = HermiteSplineParams_Default);

    void EndHermiteSpline ();

    typedef int HermiteSplinePointFlags;

    enum
    {
        HermiteSplinePointFlags_None = 0,
        HermiteSplinePointFlags_LockLocation = 0x01,
        HermiteSplinePointFlags_LockValue = 0x02,
        HermiteSplinePointFlags_LockDerivative = 0x04,
    };

    struct HermiteSplinePointValues
    {
        float *location;
        float *value;
        float *derivative;
    };

    bool HermiteSplinePoint (ImGuiID id, const ImVec2 &offset, float scale, bool selected,
        HermiteSplinePointValues &point, const HermiteSplinePointValues &next, HermiteSplinePointFlags flags = 0);
}
