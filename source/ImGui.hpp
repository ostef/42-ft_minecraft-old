#pragma once

#define IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS

// We do our own stb sprintf wrapper because ImGui includes
// the implementation of stb sprintf if we define IMGUI_USE_STB_SPRINTF
// We already implement stb sprintf ourselves in Core.cpp so this causes problems

#include <stb_sprintf.h>

inline
int ImFormatString(char* buf, size_t buf_size, const char* fmt, ...)
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
int ImFormatStringV(char* buf, size_t buf_size, const char* fmt, va_list args)
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
