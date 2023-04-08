#include "Core.hpp"

#ifdef PLATFORM_WINDOWS
# include "Core/platform_windows.cpp"
# include "Core/crash_handler_windows.cpp"
#endif

#include "Core/memory.cpp"
#include "Core/string.cpp"
#include "Core/string_builder.cpp"

#define STB_SPRINTF_IMPLEMENTATION
#include <stb_sprintf.h>
