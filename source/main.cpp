#include "Minecraft.hpp"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

Arena     frame_arena;
Allocator frame_allocator;

int main (int argc, const char **args)
{
	platform_init ();
	crash_handler_init ();

	arena_init (&frame_arena, 4096, heap_allocator ());

	while (true)
	{
		arena_reset (&frame_arena);
		sleep_milliseconds (10);
	}

	return 0;
}
