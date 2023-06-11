#pragma once

#include "Core.hpp"
#include "Linalg.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cubiomes/generator.h>

// We need to define IM_ASSERT otherwise ImGui will overwrite
// our own with assert from ucrt/assert.h
#define IM_ASSERT assert
#include "ImGui.hpp"

#include <stb_image.h>

extern Arena     frame_arena;
extern Allocator frame_allocator;

extern Vec2f g_prev_mouse_pos;
extern Vec2f g_mouse_delta;

extern GLuint g_texture_atlas;
extern s64 g_texture_atlas_size;

extern GLFWwindow *g_window;

extern s64 g_chunk_generation_time;
extern s64 g_chunk_generation_samples;
extern s64 g_chunk_creation_time;
extern s64 g_chunk_creation_samples;
extern s64 g_drawn_vertex_count;
extern s64 g_delta_time;    // In micro seconds

extern bool g_generate_new_chunks;
extern int g_render_distance;

struct Vertex;
struct Camera;
struct Block;
enum Block_Type : u8;
struct Chunk;
struct World;

const int Perlin_Fractal_Max_Octaves = 10;

struct Perlin_Fractal_Params
{
    f32 scale;
    int octaves;
    f32 persistance;
    f32 lacunarity;
};

f64 perlin_noise (f64 x, f64 y);
f64 perlin_noise (f64 x, f64 y, f64 z);
f64 perlin_fractal_max (int octaves, f64 persistance);
f64 perlin_fractal_noise (f64 scale, int octaves, Vec2f *offsets, f64 persistance, f64 lacunarity, f64 x, f64 y);
f64 perlin_fractal_noise (Perlin_Fractal_Params params, Vec2f *offsets, f64 x, f64 y);
f64 perlin_fractal_noise (f64 scale, int octaves, Vec3f *offsets, f64 persistance, f64 lacunarity, f64 x, f64 y, f64 z);
f64 perlin_fractal_noise (Perlin_Fractal_Params params, Vec3f *offsets, f64 x, f64 y, f64 z);
void perlin_generate_offsets (LC_RNG *rng, int count, Vec2f *offsets);
void perlin_generate_offsets (LC_RNG *rng, int count, Vec3f *offsets);

struct Nested_Hermite_Spline
{
    struct Knot
    {
        bool is_nested_spline;
        f32 x;
        f32 y;
        Nested_Hermite_Spline *spline;
        f32 derivative;
    };

    static const int Max_Knots = 12;

    int t_value_index;  // Index into an array of T values
    Static_Array<Knot, Max_Knots> knots;
};

inline
static f32 hermite_cubic_calculate (f32 x0, f32 y0, f32 der0, f32 x1, f32 y1, f32 der1, f32 t)
{
    f32 f8 = der0 * (x1 - x0) - (y1 - y0);
    f32 f9 = -der1 * (x1 - x0) + (y1 - y0);

    return lerp (y0, y1, t) + t * (1 - t) * lerp (f8, f9, t);
}

f32 hermite_cubic_calculate (const Nested_Hermite_Spline *spline, const Slice<f32> &t_values);

inline
static f32 hermite_knot_value (const Nested_Hermite_Spline::Knot &knot, const Slice<f32> &t_values)
{
    if (knot.is_nested_spline)
        return hermite_cubic_calculate (knot.spline, t_values);

    return knot.y;
}

struct Image
{
    s32 width;
    s32 height;
    u32 *data;
};

bool load_texture_atlas (const char *texture_dirname);

struct Camera
{
    Vec3f position;
    Vec3f euler_angles;
    Quatf rotation;
    Vec2f rotation_input;

    f32 fov;

    Mat4f transform;
    Mat4f view_matrix;
    Mat4f projection_matrix;
    Mat4f view_projection_matrix;
};

extern Camera g_camera;

void update_flying_camera (Camera *camera);

enum Block_Face : u8
{
    Block_Face_East  = 0, // +X
    Block_Face_West  = 1, // -X
    Block_Face_Above = 2, // +Y
    Block_Face_Below = 3, // -Y
    Block_Face_North = 4, // +Z
    Block_Face_South = 5, // -Z
};

enum Block_Corner : u8
{
    Block_Corner_Top_Left     = 0,
    Block_Corner_Top_Right    = 1,
    Block_Corner_Bottom_Left  = 2,
    Block_Corner_Bottom_Right = 3,
};

typedef int Block_Face_Flags;
enum
{
    Block_Face_Flag_East  = (1 << Block_Face_East),  // +X
    Block_Face_Flag_West  = (1 << Block_Face_West),  // -X
    Block_Face_Flag_Above = (1 << Block_Face_Above), // +Y
    Block_Face_Flag_Below = (1 << Block_Face_Below), // -Y
    Block_Face_Flag_North = (1 << Block_Face_North), // +Z
    Block_Face_Flag_South = (1 << Block_Face_South), // -Z
};

struct Vertex
{
    Vec3f position;
    Block_Face face;
    u8 block_id;
    Block_Corner block_corner;
};

bool render_init (const char *texture_dirname);
void draw_chunk (Chunk *chunk, Camera *camera);

enum Block_Type : u8
{
    Block_Type_Air = 0,
    Block_Type_Dirt,
    Block_Type_Stone,
    Block_Type_Bedrock,
    Block_Type_Water,

    Block_Type_Count,
};

struct Block
{
    Block_Type type;
};

static const Block Block_Air = {};

static const int Chunk_Size = 16;
static const int Chunk_Height = 384;

static const Vec2i Default_Height_Range = {50,200};

static const Perlin_Fractal_Params Default_Continentalness_Perlin_Params = { 0.001340, 3, 0.25, 1.3 };
static const Perlin_Fractal_Params Default_Erosion_Perlin_Params = { 0.002589, 5, 0.5, 1.5 };
static const Perlin_Fractal_Params Default_Weirdness_Perlin_Params = { 0.033, 3, 0.5, 1.8 };

static const Perlin_Fractal_Params Default_Perlin_Params[3] = {
    Default_Continentalness_Perlin_Params,
    Default_Erosion_Perlin_Params,
    Default_Weirdness_Perlin_Params,
};

static const int Default_Water_Level = 126;
static const f64 Surface_Dirt_Height = 8;

enum Terrain_Value
{
    Terrain_Value_Continentalness,
    Terrain_Value_Erosion,
    Terrain_Value_Weirdness,
    Terrain_Value_Surface,
    Terrain_Value_Count,
};

struct Terrain_Values
{
    union
    {
        struct
        {
            f32 continentalness;
            f32 erosion;
            f32 weirdness;
        };
        f32 noise[3];
    };

    f32 surface_level;
};

struct Terrain_Params
{
    Perlin_Fractal_Params noise[3] = {
        Default_Continentalness_Perlin_Params,
        Default_Erosion_Perlin_Params,
        Default_Weirdness_Perlin_Params
    };

    Vec2i height_range = Default_Height_Range;
    int water_level = Default_Water_Level;

    Nested_Hermite_Spline *spline;
    Static_Array<Nested_Hermite_Spline, 30> spline_stack;
};

void init_default_spline (Terrain_Params *params);

enum Chunk_Mesh_Type : u8
{
    Chunk_Mesh_Solid,
    Chunk_Mesh_Water,
    Chunk_Mesh_Count,
};

struct Chunk
{
    Chunk *east;
    Chunk *west;
    Chunk *north;
    Chunk *south;

    s64 x, z;

    s64 total_vertex_count;
    s64 vertex_counts[Chunk_Mesh_Count];
    GLuint gl_vbos[Chunk_Mesh_Count];
    GLuint opengl_is_stupid_vaos[Chunk_Mesh_Count];
    bool is_dirty;
    bool generated;

    Terrain_Values terrain_values[Chunk_Size * Chunk_Size];
    Block blocks[Chunk_Size * Chunk_Size * Chunk_Height];
};

#define chunk_block_index(x, y, z) ((y) * Chunk_Size * Chunk_Size + (x) * Chunk_Size + (z))

struct World
{
    s32 seed;

    cubiome::Generator cubiome_gen;
    Terrain_Params terrain_params;

    Vec2f noise_offsets[Perlin_Fractal_Max_Octaves][3];

    Chunk *origin_chunk;
    Hash_Map<Vec2i, Chunk *> all_loaded_chunks;
};

extern World g_world;

void chunk_init (Chunk *chunk, s64 x, s64 z);
void chunk_cleanup (Chunk *chunk);
Vec2i chunk_absolute_to_relative_coordinates (Chunk *chunk, s64 x, s64 z);
Vec2l chunk_position_from_block_position (s64 x, s64 z);
Chunk *chunk_get_at_relative_coordinates (Chunk *chunk, s64 x, s64 y, s64 z);
Block chunk_get_block_in_chunk (Chunk *chunk, s64 x, s64 y, s64 z);
Block chunk_get_block (Chunk *chunk, s64 x, s64 y, s64 z);
Terrain_Values chunk_get_terrain_values (Chunk *chunk, s64 x, s64 z);
void chunk_generate (World *world, Chunk *chunk);
void chunk_generate_mesh_data (Chunk *chunk);
void chunk_draw (Chunk *chunk, Camera *camera);

void world_init (World *world, s32 seed, int chunks_to_pre_generate = 0, Terrain_Params terrain_params = {});
Chunk *world_get_chunk (World *world, s64 x, s64 z);
Chunk *world_get_chunk_at_block_position (World *world, s64 x, s64 z);
Chunk *world_create_chunk (World *world, s64 x, s64 z);
Chunk *world_generate_chunk (World *world, s64 x, s64 z);
void world_draw_chunks (World *world, Camera *camera);
void world_clear_chunks (World *world);
Block world_get_block (World *world, s64 x, s64 y, s64 z);

