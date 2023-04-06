#include "Core.hpp"

void *mem_alloc (s64 size, Allocator allocator)
{
	if (!allocator.proc)
		return null;

	if (size <= 0)
		return null;

	return allocator.proc (Allocator_Op_Allocate, size, null, allocator.data);
}

void mem_free (void *ptr, Allocator allocator)
{
	if (ptr && allocator.proc)
		allocator.proc (Allocator_Op_Free, 0, ptr, allocator.data);
}

void *heap_allocator_proc (Allocator_Op op, s64 size, void *old_ptr, void *data)
{
	(void)data;

	if (op == Allocator_Op_Allocate)
		return malloc (size);
	else if (op == Allocator_Op_Free)
		free (old_ptr);
	else
		panic ("Invalid allocator operation");

	return null;
}

Allocator heap_allocator ()
{
	Allocator result;
	result.data = null;
	result.proc = heap_allocator_proc;

	return result;
}

bool arena_add_page (Arena *arena, s64 min_size)
{
	s64 size = max (min_size, arena->default_page_size);

	Arena_Page *page = cast (Arena_Page *) mem_alloc (size + sizeof (Arena_Page), arena->page_allocator);
	if (!page)
		return false;

	page->prev = arena->page;
	page->size = size;
	page->used = 0;

	arena->page = page;
	arena->total_size += page->size;

	return true;
}

bool arena_init (Arena *arena, s64 default_page_size, Allocator allocator)
{
	arena->page = null;
	arena->total_size = 0;
	arena->page_allocator = allocator;
	arena->default_page_size = default_page_size;

	return arena_add_page (arena, default_page_size);
}

void arena_free_pages (Arena *arena, Arena_Page *until)
{
	while (arena->page && arena->page != until)
	{
		arena->total_size -= arena->page->size;
		arena->total_used -= arena->page->used;

		Arena_Page *prev = arena->page->prev;
		mem_free (arena->page, arena->page_allocator);
		arena->page = prev;
	}
}

void arena_reset (Arena *arena)
{
	arena_free_pages (arena, null);
}

Arena_State arena_get_state (Arena *arena)
{
	Arena_State state;
	state.page = arena->page;
	if (arena->page)
		state.mark = arena->page->used;
	else
		state.mark = 0;

	return state;
}

void arena_set_state (Arena *arena, Arena_State state)
{
	arena_free_pages (arena, state.page);

	assert (arena->page == state.page, "Invalid arena state given to arena_set_state.");

	if (state.page)
	{
		s64 diff = state.page->used - state.mark;
		state.page->used -= diff;
		arena->total_used -= diff;
	}
}

void *arena_allocator_proc (Allocator_Op op, s64 size, void *old_ptr, void *data)
{
	(void)old_ptr;

	Arena *arena = (Arena *)data;

	if (op == Allocator_Op_Allocate)
	{
		if (size <= 0)
			return null;

		s64 aligned_size = (size + 7) & ~7;

		if (!arena->page || arena->page->used + aligned_size > arena->page->size)
		{
			if (!arena_add_page (arena, aligned_size))
			{
				panic ("Could not allocate memory for arena.");
				return null;
			}
		}

		void *ptr = cast (u8 *) (arena->page + 1) + arena->page->used;
		ptr = cast (void *) ((cast (s64) ptr + 7) & ~7);

		arena->page->used += aligned_size;
		arena->total_used += aligned_size;

		return ptr;
	}
	else if (op != Allocator_Op_Free)
		panic ("Unreachable code.");

	return null;
}

Allocator arena_allocator (Arena *arena)
{
	Allocator result;
	result.data = arena;
	result.proc = arena_allocator_proc;

	return result;
}
