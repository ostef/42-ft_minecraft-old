#include "Core.hpp"

int decimal_length (u64 n)
{
	if (n == 0)
		return 1;

	int i = 0;
	while (n)
	{
		n /= 10;
		i += 1;
	}

	return i;
}

u32 hash_string (const String &str)
{
	static const u32 OFFSET_BASIS = 0x811c9dc5;
	static const u32 PRIME        = 0x01000193;

	u32 hash = OFFSET_BASIS;
	for_array (i, str)
	{
		hash ^= str.data[i];
		hash *= PRIME;
	}

	return hash;
}