#include "Core.hpp"

bool string_builder_alloc_page (String_Builder *builder)
{
	assert (builder->curr_page->next == null);

	String_Builder_Page *page = mem_alloc_uninit (String_Builder_Page, 1, builder->allocator);
	if (!page)
		return false;

	page->prev = builder->curr_page;
	page->next = null;
	page->count = 0;

	builder->curr_page->next = page;
	builder->curr_page = page;

	return true;
}

void string_builder_init (String_Builder *builder, Allocator allocator)
{
	builder->allocator = allocator;
	builder->count = 0;
	builder->curr_page = &builder->base_page;
	builder->base_page.prev = null;
	builder->base_page.next = null;
	builder->base_page.count = 0;
}

void string_builder_free (String_Builder *builder)
{
	while (builder->curr_page != &builder->base_page)
	{
		String_Builder_Page *prev = builder->curr_page->prev;

		mem_free (builder->curr_page, builder->allocator);

		builder->curr_page = prev;
	}

	builder->count = 0;
	builder->base_page.count = 0;
	builder->base_page.prev = null;
	builder->base_page.next = null;
}

void string_builder_clear (String_Builder *builder)
{
	while (builder->curr_page != &builder->base_page)
	{
		builder->curr_page->count = 0;
		builder->curr_page = builder->curr_page->prev;
	}

	builder->base_page.count = 0;
	builder->count = 0;
}

String string_builder_build (String_Builder *builder, Allocator allocator, bool free_builder)
{
	String result;
	result.count = builder->count;
	result.data = mem_alloc_uninit (char, result.count, allocator);

	s64 offset = 0;
	auto page = &builder->base_page;
	while (page && page->count > 0)
	{
		memcpy (result.data + offset, page->buff, page->count);
		offset += page->count;
		page = page->next;
	}

	assert (offset == result.count);

	if (free_builder)
		string_builder_free (builder);

	return result;
}

char *string_builder_build_cstr (String_Builder *builder, Allocator allocator, bool free_builder)
{
	char *result = mem_alloc_uninit (char, builder->count + 1, allocator);

	s64 offset = 0;
	auto page = &builder->base_page;
	while (page && page->count > 0)
	{
		memcpy (result + offset, page->buff, page->count);
		offset += page->count;
		page = page->next;
	}

	result[builder->count] = 0;

	if (free_builder)
		string_builder_free (builder);

	return result;
}

s64 string_builder_append_byte (String_Builder *builder, u8 byte)
{
	if (byte == 0)	// Don't append null terminators
		return 0;

	if (builder->curr_page->count >= STRING_BUILDER_PAGE_SIZE)
	{
		if (builder->curr_page->next)
		{
			builder->curr_page = builder->curr_page->next;
			assert (builder->curr_page->count == 0, "Moved to the next string builder page, but it was not empty.");
		}
		else
		{
			bool page_ok = string_builder_alloc_page (builder);
			assert (page_ok, "Could not allocate new page for string builder.");
		}
	}

	builder->curr_page->buff[builder->curr_page->count] = byte;
	builder->curr_page->count += 1;
	builder->count += 1;

	return 1;
}

s64 string_builder_append_string (String_Builder *builder, const String &str)
{
	for_array (i, str)
		string_builder_append_byte (builder, cast (u8) str[i]);

	return str.count;
}

s64 string_builder_append_string (String_Builder *builder, const char *cstr)
{
	s64 i = 0;

	while (cstr[i])
	{
		string_builder_append_byte (builder, cast (u8) cstr[i]);
		i += 1;
	}

	return i;
}

s64 string_builder_append_string (String_Builder *builder, s64 len, const char *str)
{
	s64 i = 0;

	while (i < len)
	{
		string_builder_append_byte (builder, cast (u8) str[i]);
		i += 1;
	}

	return i;
}

s64 string_builder_append_s64 (String_Builder *builder, s64 value, const char *base)
{
	s64 appended = 0;
	u64 unsigned_value;
	if (value < 0)
	{
		appended += string_builder_append_byte (builder, '-');
		unsigned_value = -value;
	}
	else
	{
		unsigned_value = value;
	}

	appended += string_builder_append_u64 (builder, unsigned_value, base);

	return appended;
}

s64 string_builder_append_u64 (String_Builder *builder, u64 value, const char *base)
{
	int base_len = strlen (base);
	assert (base_len >= 2, "Invalid base, should be at least 2 characters.");

	if (value == 0)
		return string_builder_append_byte (builder, cast (u8) base[0]);

	u64 tmp_value = value;
	int number_len = 0;
	while (tmp_value)
	{
		number_len += 1;
		tmp_value /= base_len;
	}

	char buff[64];

	int i = 0;
	while (value)
	{
		char digit = base[value % base_len];
		buff[number_len - i - 1] = digit;

		i += 1;
		value /= base_len;
	}

	return string_builder_append_string (builder, i, buff);
}

s64 string_builder_append_int (String_Builder *builder, int value, const char *base)
{
	return string_builder_append_s64 (builder, cast (s64) value, base);
}

char *string_builder_stbsp_callback (const char *buff, void *user, int len)
{
	String_Builder *builder = cast (String_Builder *) user;
	assert (STRING_BUILDER_PAGE_SIZE - builder->curr_page->count >= len,
		"String builder does not have enough space for stbsprintf. This shouldn't happen.");

	builder->curr_page->count += len;
	builder->count += len;

	if (STRING_BUILDER_PAGE_SIZE - builder->curr_page->count < STB_SPRINTF_MIN)
	{
		bool page_ok = string_builder_alloc_page (builder);
		assert (page_ok, "Could not allocate new page for string builder.");
	}

	return cast (char *) builder->curr_page->buff + builder->curr_page->count;
}

s64 string_builder_appendv (String_Builder *builder, const char *fmt_str, va_list va)
{
	if (STRING_BUILDER_PAGE_SIZE - builder->curr_page->count < STB_SPRINTF_MIN)
		string_builder_alloc_page (builder);

	return stbsp_vsprintfcb (string_builder_stbsp_callback, builder, cast (char *) builder->curr_page->buff + builder->curr_page->count, fmt_str, va);
}

s64 string_builder_append (String_Builder *builder, const char *fmt_str, ...)
{
	va_list va;

	va_start (va, fmt_str);

	s64 appended = string_builder_appendv (builder, fmt_str, va);

	va_end (va);

	return appended;
}

s64 string_builder_append_line (String_Builder *builder, const char *fmt_str, ...)
{
	va_list va;

	va_start (va, fmt_str);

	s64 appended = string_builder_appendv (builder, fmt_str, va);

	va_end (va);

	appended += string_builder_append_byte (builder, '\n');

	return appended;
}
