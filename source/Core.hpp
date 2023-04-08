#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <math.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
# define PLATFORM_WINDOWS
# ifdef _WIN64
# define PLATFORM_64BIT
#else
# define PLATFORM_32BIT
# endif
#elif __linux__
# define PLATFORM_LINUX
#else
# error "Unsupported compiler"
#endif

#if __GNUC__
# if __x86_64__ || __ppc64__
#  define PLATFORM_64BIT
# else
#  define PLATFORM_32BUT
# endif
#endif

#if defined(PLATFORM_32BIT) && !defined(ALLOW_32BIT)
# error "Unsupported platform, only 64-bit platforms are supported"
#endif

#ifdef STRING_BUILDER_PAGE_SIZE
# define STB_SPRINTF_MIN (STRING_BUILDER_PAGE_SIZE / 2)
#endif

#ifdef PLATFORM_WINDOWS
#pragma warning (disable: 4312)	// 'type cast': conversion from 'A' to 'B' of greater size
#endif

#include <stb_sprintf.h>

#define null nullptr

typedef  uint8_t  u8;
typedef   int8_t  s8;
typedef uint16_t u16;
typedef  int16_t s16;
typedef uint32_t u32;
typedef  int32_t s32;
typedef uint64_t u64;
typedef  int64_t s64;

typedef float  f32;
typedef double f64;

#if defined (PLATFORM_WINDOWS)

# define debug_break() __debugbreak ()

#elif defined (PLATFORM_LINUX)

# include <signal.h>
# define debug_break() raise (SIGTRAP)

#endif

#ifndef REPORT_FATAL_COLOR
# define REPORT_FATAL_COLOR "1;35"	// Bold magenta
#endif

#ifndef REPORT_ERROR_COLOR
# define REPORT_ERROR_COLOR "1;31"	// Bold red
#endif

#ifndef REPORT_INFO_COLOR
# define REPORT_INFO_COLOR "1;32"	// Bold green
#endif

#define panic(...) do {\
    print ("\n\033[" REPORT_FATAL_COLOR "mPanic\033[0m at file " __FILE__ ":%d\n", __LINE__);\
    print ("\t" __VA_ARGS__);\
    print ("\n");\
    debug_break ();\
} while (0)

#define assert(expr, ...) do {\
    if (!(expr)) {\
        print ("\n\033[" REPORT_FATAL_COLOR "mAssertion failed (" #expr ")\033[0m at file " __FILE__ ":%d\n", __LINE__);\
        print ("\t" __VA_ARGS__); print ("\n");\
        debug_break ();\
    }\
} while (0)

#define cast(T) (T)

#define array_size(a) (sizeof (a) / sizeof (*(a)))

#define for_range(index, low, high) for (s64 index = low; index < high; index += 1)
#define for_list(it, first) for (auto it = first; it != null; it = it->next)
#define for_array(index, arr) for (s64 index = 0; index < (arr).count; index += 1)

// Defer
// https://www.gingerbill.org/article/2015/08/19/defer-in-cpp/

template <typename F>
struct privDefer
{
    F f;

    privDefer (F f) : f (f) {}
    ~privDefer () { f (); }
};

template <typename F>
privDefer<F> defer_func (F f)
{
    return privDefer<F> (f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1 (x, y)
#define DEFER_3(x)    DEFER_2 (x, __COUNTER__)
#define defer(code)   auto DEFER_3 (_defer_) = defer_func ([&](){code;})

// Math

#define PI 3.14159265358979323846

inline
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

template<typename T>
T abs (T x)
{
    return x > 0 ? x : -x;
}

template<typename T>
T min (T a, T b)
{
    return a < b ? a : b;
}

template<typename T>
T max (T a, T b)
{
    return a > b ? a : b;
}

template<typename T>
T clamp (T x, T a, T b)
{
    return x < a ? a : (x > b ? b : x);
}

template<typename T>
T lerp (T a, T b, T t)
{
    return a + t * (b - a);
}

inline
bool approx_zero (f32 x, f32 epsilon = 0.00001f)
{
    return abs (x) <= epsilon;
}

inline
bool approx_equals (f32 a, f32 b, f32 epsilon = 0.00001f)
{
    return abs (a - b) <= epsilon;
}

template<typename T>
T to_rads (T angle)
{
    return angle * PI / 180.0;
}

// Memory

enum Allocator_Op
{
    Allocator_Op_Allocate = 0,
    Allocator_Op_Free,
};

typedef void *(*Allocator_Proc) (Allocator_Op op, s64 size, void *old_ptr, void *data);

struct Allocator
{
    void          *data;
    Allocator_Proc proc;
};

void *mem_alloc (s64 size, Allocator allocator);
void mem_free (void *ptr, Allocator allocator);

#define mem_alloc_typed(T, count, allocator) cast (T *) memset (mem_alloc (sizeof (T) * (count), (allocator)), 0, sizeof (T) * (count))
#define mem_alloc_uninit(T, count, allocator) cast (T *) mem_alloc (sizeof (T) * (count), (allocator))

Allocator heap_allocator ();

struct Arena_Page
{
    struct Arena_Page *prev;
    s64 used;
    s64 size;
};

struct Arena
{
    Arena_Page *page;
    Allocator page_allocator;
    s64 total_size;
    s64 total_used;
    s64 default_page_size;
};

struct Arena_State
{
    Arena_Page *page;
    s64 mark;
};

bool arena_init (Arena *arena, s64 default_page_size, Allocator allocator);
void arena_reset (Arena *arena);
Arena_State arena_get_state (Arena *arena);
void arena_set_state (Arena *arena, Arena_State state);
Allocator arena_allocator (Arena *arena);

// Print

int vprint (const char *fmt_str, va_list va);
int print (const char *fmt_str, ...);
int println (const char *fmt_str, ...);

int vfprint (FILE *file, const char *fmt_str, va_list va);
int fprint (FILE *file, const char *fmt_str, ...);
int fprintln (FILE *file, const char *fmt_str, ...);

// String

struct String
{
    s64 count;
    char *data;

    inline
    char &operator[] (s64 i)
    {
        #ifndef NO_BOUNDS_CHECKING
            assert (i >= 0 && i < count, "Bounds check failed (got %lld, expected [0; %lld]).", i, count - 1);
        #endif

        return data[i];
    }

    inline
    const char &operator[] (s64 i) const
    {
        #ifndef NO_BOUNDS_CHECKING
            assert (i >= 0 && i < count, "Bounds check failed (got %lld, expected [0; %lld]).", i, count - 1);
        #endif

        return data[i];
    }
};

#define fstr(s) cast (int) (s).count, (s).data

String string_make (const char *cstr);
String string_make (s64 count, char *data);
String string_clone (const String &str, Allocator allocator);
String string_clone (const char *str, Allocator allocator);
char *string_clone_to_cstring (const String &str, Allocator allocator);
int string_compare (const String &a, const String &b);
int string_compare (const String &a, const char *b);
bool string_equals (const String &a, const String &b);
bool string_equals (const String &a, const char *b);

bool is_alpha (char c);
bool is_digit (char c);
bool is_whitespace (char c);
bool is_alpha_numeric (char c);

String vfstring (Allocator allocator, const char *fmt_str, va_list va);
String fstring (Allocator allocator, const char *fmt_str, ...);
char *vfcstring (Allocator allocator, const char *fmt_str, va_list va);
char *fcstring (Allocator allocator, const char *fmt_str, ...);

// String Builder

#ifndef STRING_BUILDER_PAGE_SIZE
# define STRING_BUILDER_PAGE_SIZE (STB_SPRINTF_MIN * 2)
#endif

struct String_Builder_Page
{
    struct String_Builder_Page *prev;
    struct String_Builder_Page *next;
    s64 count;
    u8 buff[STRING_BUILDER_PAGE_SIZE];
};

struct String_Builder
{
    Allocator allocator;
    s64 count;
    String_Builder_Page *curr_page;
    String_Builder_Page base_page;
};

#define      BINARY_BASE "01"
#define       OCTAL_BASE "01234567"
#define     DECIMAL_BASE "0123456789"
#define HEXADECIMAL_BASE "0123456789abcdef"

void string_builder_init (String_Builder *builder, Allocator allocator);
void string_builder_free (String_Builder *builder);
void string_builder_clear (String_Builder *builder);
String string_builder_build (String_Builder *builder, Allocator allocator, bool free_builder = true);
char *string_builder_build_cstr (String_Builder *builder, Allocator allocator, bool free_builder = true);
s64 string_builder_append_byte (String_Builder *builder, u8 byte);
s64 string_builder_append_string (String_Builder *builder, const String &str);
s64 string_builder_append_string (String_Builder *builder, const char *cstr);
s64 string_builder_append_string (String_Builder *builder, s64 len, const char *str);
s64 string_builder_append_s64 (String_Builder *builder, s64 value, const char *base = DECIMAL_BASE);
s64 string_builder_append_u64 (String_Builder *builder, u64 value, const char *base = DECIMAL_BASE);
s64 string_builder_append_int (String_Builder *builder, int value, const char *base = DECIMAL_BASE);
s64 string_builder_appendv (String_Builder *builder, const char *fmt_str, va_list va);
s64 string_builder_append (String_Builder *builder, const char *fmt_str, ...);
s64 string_builder_append_line (String_Builder *builder, const char *fmt_str, ...);

// Print

int fprint_string (FILE *file, String str);
int fprint_string (FILE *file, const char *cstr);
int print_string (String str);
int print_string (const char *cstr);
int fprint_u64 (FILE *file, u64 n, const char *base = DECIMAL_BASE);
int print_u64 (u64 n, const char *base = DECIMAL_BASE);
int fprint_s64 (FILE *file, s64 n, const char *base = DECIMAL_BASE);
int print_s64 (s64 n, const char *base = DECIMAL_BASE);

#ifdef PLATFORM_WINDOWS

String wide_to_utf8 (wchar_t *data, Allocator allocator);
wchar_t *utf8_to_wide (String str, s64 *out_length, Allocator allocator);

#endif

String filename_get_full (String filename, Allocator allocator);
String filename_get_parent_dir (String filename);

// Array

template <typename T>
struct Slice
{
    s64 count;
    T *data;

    inline
    T &operator[] (s64 i)
    {
        #ifndef NO_BOUNDS_CHECKING
            assert (i >= 0 && i < count, "Bounds check failed (got %lld, expected [0; %lld]).", i, count - 1);
        #endif

        return data[i];
    }

    inline
    const T &operator[] (s64 i) const
    {
        #ifndef NO_BOUNDS_CHECKING
            assert (i >= 0 && i < count, "Bounds check failed (got %lld, expected [0; %lld]).", i, count - 1);
        #endif

        return data[i];
    }
};

template<typename T>
Slice<T> slice_make (s64 count, T *data)
{
    Slice<T> result;
    result.count = count;
    result.data = data;

    return result;
}

template<typename T>
Slice<T> slice_alloc (s64 count, Allocator allocator)
{
    Slice<T> result;

    result.data = mem_alloc_uninit (T, count, allocator);
    if (!result.data)
    {
        result.count = 0;

        return result;
    }

    result.count = count;

    return result;
}

template <typename T>
struct Array : public Slice<T>
{
    s64 capacity;
    Allocator allocator;
};

template<typename T>
Slice<T> slice_from_array (const Array<T> &arr)
{
    Slice<T> result;
    result.count = arr.count;
    result.data = arr.data;

    return result;
}

template<typename T>
void array_init (Array<T> *arr, Allocator allocator, s64 base_capacity = 0)
{
    arr->count = 0;
    arr->data = null;
    arr->capacity = 0;
    arr->allocator = allocator;
    if (base_capacity > 0)
        array_reserve (arr, base_capacity);
}

template<typename T>
void array_free (Array<T> *arr)
{
    mem_free (arr->data, arr->allocator);
    arr->data = null;
    arr->capacity = 0;
    arr->count = 0;
}

template<typename T>
void array_clear (Array<T> *arr)
{
    arr->count = 0;
}

template<typename T>
void array_reserve (Array<T> *arr, s64 capacity)
{
    if (capacity < arr->capacity)
        return;

    T *new_data = mem_alloc_uninit (T, capacity, arr->allocator);
    assert (new_data != null, "Array reserve failed. Requested %lld elements.", capacity);
    memcpy (new_data, arr->data, sizeof (T) * arr->count);

    mem_free (arr->data, arr->allocator);

    arr->data = new_data;
    arr->capacity = capacity;
}

template<typename T>
T *array_push (Array<T> *arr)
{
    if (arr->count >= arr->capacity)
        array_reserve (arr, arr->capacity * 2 + 8);

    T *result = &arr->data[arr->count];
    arr->count += 1;

    return result;
}

template<typename T>
T *array_push (Array<T> *arr, const T &elem)
{
    T *ptr = array_push (arr);
    *ptr = elem;

    return ptr;
}

template<typename T>
void array_pop (Array<T> *arr)
{
    assert (arr->count > 0, "Cannot pop element from empty array.");
    arr->count -= 1;
}

// Hash map

#define HASH_NEVER_OCCUPIED 0
#define HASH_REMOVED 1
#define HASH_FIRST_OCCUPIED 2
#define HASH_MAP_MIN_CAPACITY 32
#define HASH_MAP_LOAD_FACTOR 70

template<typename Key, typename Value>
struct Hash_Map
{
    typedef u32 (*hash_func) (const Key &);
    typedef bool (*compare_func) (const Key &, const Key &);

    struct Entry
    {
        u32 hash;
        Key key;
        Value value;
    };

    s64 count;
    s64 capacity;
    Allocator allocator;
    Entry *entries;
    hash_func hash_function;
    compare_func compare_function;
};

#define for_hash_map(it, hash_map) for (auto it = hash_map_first (&(hash_map)); it.key; it = hash_map_next (&(hash_map), it))

template<typename Key, typename Value>
bool hash_map_init (
    Hash_Map<Key, Value> *map,
    typename Hash_Map<Key, Value>::hash_func hash_function,
    typename Hash_Map<Key, Value>::compare_func compare_function,
    Allocator allocator,
    s64 base_capacity = 0
)
{
    assert (base_capacity >= 0);
    assert (hash_function != null);
    assert (compare_function != null);

    memset (map, 0, sizeof (*map));

    map->allocator = allocator;
    map->count = 0;
    map->hash_function = hash_function;
    map->compare_function = compare_function;

    if (base_capacity > 0)
    {
        map->capacity = max (base_capacity, HASH_MAP_MIN_CAPACITY);

        s64 p = 1;
        while (map->capacity > p)
            p += p;
        map->capacity = p;

        map->entries = (typename Hash_Map<Key, Value>::Entry *)mem_alloc (
            sizeof (typename Hash_Map<Key, Value>::Entry) * map->capacity, map->allocator);
        if (!map->entries)
            return false;

        for_range (i, 0, map->capacity)
        {
            map->entries[i].hash = HASH_NEVER_OCCUPIED;
        }
    }

    return true;
}

template<typename Key, typename Value>
void hash_map_free (Hash_Map<Key, Value> *map)
{
    mem_free (&map->entries, map->allocator);
    map->count = 0;
    map->capacity = 0;
}

template<typename Key, typename Value>
void hash_map_clear (Hash_Map<Key, Value> *map)
{
    for_range (i, 0, map->capacity)
        map->entries[i].hash = HASH_NEVER_OCCUPIED;

    map->count = 0;
}

template<typename Key, typename Value>
bool hash_map_grow (Hash_Map<Key, Value> *map)
{
    auto old_capacity = map->capacity;
    auto old_entries = map->entries;

    auto new_capacity = max (map->capacity * 2, HASH_MAP_MIN_CAPACITY);
    auto new_entries = map->entries = (typename Hash_Map<Key, Value>::Entry *)mem_alloc (
        sizeof (typename Hash_Map<Key, Value>::Entry) * new_capacity, map->allocator);
    if (!new_entries)
        return false;

    map->capacity = new_capacity;
    map->entries = new_entries;
    map->count = 0;

    for_range (i, 0, new_capacity)
        new_entries[i].hash = HASH_NEVER_OCCUPIED;

    for_range (i, 0, old_capacity)
    {
        if (old_entries[i].hash >= HASH_FIRST_OCCUPIED)
            hash_map_insert (map, old_entries[i].key, old_entries[i].value);
    }

    mem_free (old_entries, map->allocator);

    return true;
}

struct Hash_Map_Probe_Result
{
    u32 hash;
    s64 index;
    bool is_present;
};

template<typename Key, typename Value>
Hash_Map_Probe_Result hash_map_probe (Hash_Map<Key, Value> *map, const Key &key)
{
    u32 mask = cast (u32) (map->capacity - 1);
    u32 hash = map->hash_function (key);
    if (hash < HASH_FIRST_OCCUPIED)
        hash += HASH_FIRST_OCCUPIED;

    s64 index = hash & mask;
    u32 increment = 1 + (hash >> 27);
    while (map->entries[index].hash != HASH_NEVER_OCCUPIED)
    {
        auto entry = &map->entries[index];
        if (entry->hash == hash && map->compare_function (entry->key, key))
            return {hash, index, true};

        index += increment;
        index &= mask;
        increment += 1;
    }

    return {hash, index, false};
}

template<typename Key, typename Value>
struct Hash_Map_Insert_Result
{
    Value *ptr;
    bool was_present;
};

template<typename Key, typename Value>
Hash_Map_Insert_Result<Key, Value> hash_map_insert (Hash_Map<Key, Value> *map, const Key &key)
{
    if ((map->count + 1) * 100 >= map->capacity * HASH_MAP_LOAD_FACTOR)
    {
        if (!hash_map_grow (map))
            panic ("Could not allocate memory for hash map.");
    }

    auto probe_result = hash_map_probe (map, key);

    auto entry = &map->entries[probe_result.index];
    if (probe_result.is_present)
        return {&entry->value, true};

    memset (&entry->value, 0, sizeof (entry->value));
    entry->hash = probe_result.hash;
    entry->key = key;
    map->count += 1;

    return {&entry->value, false};
}

template<typename Key, typename Value>
Hash_Map_Insert_Result<Key, Value> hash_map_insert (Hash_Map<Key, Value> *map, const Key &key, const Value &value)
{
    auto result = hash_map_insert (map, key);
    *(result.ptr) = value;

    return result;
}

template<typename Key, typename Value>
bool hash_map_remove (Hash_Map<Key, Value> *map, const Key &key)
{
    auto probe_result = hash_map_probe (map, key);
    if (!probe_result.is_present)
        return false;

    auto entry = &map->entries[probe_result.index];
    entry->hash = HASH_REMOVED;
    map->count -= 1;

    return true;
}

template<typename Key, typename Value>
Value *hash_map_get (Hash_Map<Key, Value> *map, const Key &key)
{
    if (map->count == 0)
        return null;

    auto probe_result = hash_map_probe (map, key);
    if (probe_result.is_present)
        return &map->entries[probe_result.index].value;

    return null;
}

template<typename Key, typename Value>
bool hash_map_set (Hash_Map<Key, Value> *map, const Key &key, const Value &value)
{
    Value *ptr = hash_map_get (map, key);
    if (ptr)
    {
        *ptr = value;

        return true;
    }

    return false;
}

template<typename Key, typename Value>
struct Hash_Map_Iter
{
    s64 index;
    const Key *key;
    Value *value;
};

template<typename Key, typename Value>
Hash_Map_Iter<Key, Value> hash_map_first (Hash_Map<Key, Value> *map)
{
    s64 index = 0;
    while (index < map->capacity && map->entries[index].hash < HASH_FIRST_OCCUPIED)
        index += 1;

    if (index >= map->capacity)
        return {0, null, null};

    return {index, &map->entries[index].key, &map->entries[index].value};
}

template<typename Key, typename Value>
Hash_Map_Iter<Key, Value> hash_map_next (Hash_Map<Key, Value> *map, const Hash_Map_Iter<Key, Value> &it)
{
    s64 index = it.index + 1;
    while (index < map->capacity && map->entries[index].hash < HASH_FIRST_OCCUPIED)
        index += 1;

    if (index >= map->capacity)
        return {-1, null, null};

    return {index, &map->entries[index].key, &map->entries[index].value};
}

// Hash

inline
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


// Platform layer

void platform_init ();
s64 time_current_monotonic ();
String get_executable_path ();
String get_error_string (u32 error_code);
String get_last_error_string ();
void sleep_milliseconds (u32 ms);

// Debug

void crash_handler_init ();
