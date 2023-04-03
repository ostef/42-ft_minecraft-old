#include "Core.hpp"

String string_make (const char *cstr)
{
	String result;
	// We bypass const because it's too inconvenient to have another String struct for const strings.
	// In my programming style we don't really care about constness, apart for string literals I don't
	// really use const that much. Writing to a string literal would produce a bus error or segfault
	// anyway, so const-correctness in our case isn't that big of a deal, since we don't rely on it really.
	result.data = cast (char *) cstr;
	result.count = strlen (cstr);

	return result;
}

String string_make (s64 count, char *data)
{
	String result;
	result.count = count;
	result.data = data;

	return result;
}

String string_clone (const String &str, Allocator allocator)
{
	String result;
	result.count = str.count;
	result.data = mem_alloc_uninit (char, str.count, allocator);
	memcpy (result.data, str.data, str.count);

	return result;
}

String string_clone (const char *str, Allocator allocator)
{
	return string_clone (string_make (str), allocator);
}

char *string_clone_to_cstring (const String &str, Allocator allocator)
{
	char *result = mem_alloc_uninit (char, str.count + 1, allocator);
	memcpy (result, str.data, str.count);
	result[str.count] = 0;

	return result;
}

int string_compare (const String &a, const String &b)
{
	if (a.count < b.count)
		return -b.data[a.count];
	if (a.count > b.count)
		return a.data[b.count];

	return strncmp (a.data, b.data, a.count);
}

int string_compare (const String &a, const char *b)
{
	return strncmp (a.data, b, a.count);
}

bool string_equals (const String &a, const String &b)
{
	return string_compare (a, b) == 0;
}

bool string_equals (const String &a, const char *b)
{
	return string_compare (a, b) == 0;
}

bool is_alpha (char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_digit (char c)
{
	return c >= '0' && c <= '9';
}

bool is_alpha_numeric (char c)
{
	return c == '_' || is_alpha (c) || is_digit (c);
}

bool is_whitespace (char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

int fprint_string (FILE *file, String str)
{
	fwrite (str.data, 1, cast (u64) str.count, file);

	return cast (int) str.count;
}

int fprint_string (FILE *file, const char *cstr)
{
	return fprint_string (file, string_make (cstr));
}

int print_string (String str)
{
	return fprint_string (stdout, str);
}

int print_string (const char *cstr)
{
	return fprint_string (stdout, cstr);
}

int fprint_u64 (FILE *file, u64 n, const char *base)
{
	if (n == 0)
	{
		fprint_string (file, string_make (1, (char *)base));
		return 1;
	}

	int base_len = strlen (base);

	int len = 0;
	u64 temp = n;
	while (temp)
	{
		len += 1;
		temp /= base_len;
	}

	char buffer[64];
	int i = 0;
	while (i < len)
	{
		int digit = n % base_len;
		n /= base_len;
		buffer[len - 1 - i] = base[digit];
		i += 1;
	}

	return fprint_string (file, string_make (len, buffer));
}

int print_u64 (u64 n, const char *base)
{
	return fprint_u64 (stdout, n, base);
}

int fprint_s64 (FILE *file, s64 n, const char *base)
{
	int i = 0;

	u64 u;
	if (n < 0)
	{
		i += fprint_string (file, "-");
		u = -n;
	}
	else
	{
		u = n;
	}

	return i + fprint_u64 (file, u, base);
}

int print_s64 (s64 n, const char *base)
{
	return fprint_s64 (stdout, n, base);
}

char *print_stbsp_callback (const char *buff, void *user, int len)
{
	FILE *file = cast (FILE *) user;

	fwrite (buff, 1, len, file);

	return cast (char *) buff;	// Reuse the same buffer
}

int vprint (const char *fmt_str, va_list va)
{
	char buff[STB_SPRINTF_MIN];
	FILE *file = stdout;

	return stbsp_vsprintfcb (print_stbsp_callback, file, buff, fmt_str, va);
}

int print (const char *fmt_str, ...)
{
	va_list va;

	va_start (va, fmt_str);
	int printed = vprint (fmt_str, va);
	va_end (va);

	return printed;
}

int println (const char *fmt_str, ...)
{
	va_list va;

	va_start (va, fmt_str);
	int printed = vprint (fmt_str, va);
	va_end (va);

	return printed + print_string ("\n");
}

int vfprint (FILE *file, const char *fmt_str, va_list va)
{
	char buff[STB_SPRINTF_MIN];

	return stbsp_vsprintfcb (print_stbsp_callback, file, buff, fmt_str, va);
}

int fprint (FILE *file, const char *fmt_str, ...)
{
	va_list va;

	va_start (va, fmt_str);
	int printed = vfprint (file, fmt_str, va);
	va_end (va);

	return printed;
}

int fprintln (FILE *file, const char *fmt_str, ...)
{
	va_list va;

	va_start (va, fmt_str);
	int printed = vfprint (file, fmt_str, va);
	va_end (va);

	return printed + fprint_string (file, "\n");
}

String vfstring (Allocator allocator, const char *fmt_str, va_list va)
{
	String_Builder builder;
	string_builder_init (&builder, allocator);

	string_builder_appendv (&builder, fmt_str, va);

	return string_builder_build (&builder, allocator);
}

String fstring (Allocator allocator, const char *fmt_str, ...)
{
	va_list va;

	va_start (va, fmt_str);

	String_Builder builder;
	string_builder_init (&builder, allocator);

	string_builder_appendv (&builder, fmt_str, va);

	va_end (va);

	return string_builder_build (&builder, allocator);
}

String filename_get_parent_dir (String filename)
{
	if (!filename.count)
		return {};

	s64 offset = filename.count - 1;
	while (offset > 0)
	{
		if (filename[offset] == '\\' || filename[offset] == '/')
			break;

		offset -= 1;
	}

	return string_make (offset, filename.data);
}
