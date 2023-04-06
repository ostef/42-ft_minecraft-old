#pragma once

#include "Core.hpp"
#include "Linalg.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

// We need to define IM_ASSERT otherwise ImGui will overwrite
// our own with assert from ucrt/assert.h
#define IM_ASSERT assert
#include "ImGui.hpp"

extern Arena     frame_arena;
extern Allocator frame_allocator;

f64 perlin_noise (f64 x, f64 y, f64 z);
void show_perlin_test_window (bool *opened = null);
