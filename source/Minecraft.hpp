#pragma once

#include "Core.hpp"
#include "Linalg.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

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
f64 perlin_fractal_noise (f64 scale, int octaves, Vec2f *offsets, f64 persistance, f64 lacunarity, f64 x, f64 y, f64 z);
f64 perlin_fractal_noise (Perlin_Fractal_Params params, Vec2f *offsets, f64 x, f64 y, f64 z);
void perlin_generate_offsets (LC_RNG *rng, int count, Vec2f *offsets);

inline
static Vec2f bezier_cubic_calculate (const Vec2f &p1, const Vec2f &p2, const Vec2f &p3, const Vec2f &p4, f32 t)
{
    f32 u = 1.0f - t;
    f32 w1 = u * u * u;
    f32 w2 = 3 * u * u * t;
    f32 w3 = 3 * u * t * t;
    f32 w4 = t * t * t;

    return Vec2f{w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x, w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y};
}

Vec2f bezier_cubic_calculate (int count, Vec2f *points, f32 t);

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

bool render_init ();
void draw_chunk (Chunk *chunk, Camera *camera);

enum Block_Type : u8
{
    Block_Type_Air = 0,
    Block_Type_Dirt,
    Block_Type_Stone,
    Block_Type_Bedrock,

    Block_Type_Count,
};

struct Block
{
    Block_Type type;
};

static const Block Block_Air = {};

static const int Chunk_Size = 16;
static const int Chunk_Height = 384;

static const int Terrain_Curves_Max_Points = ImGuiExt_BezierCurve_PointCountFromCurveCount (10);

// The bezier points need to be a #define because we can't assign fixed arrays

#define Default_Continentalness_Bezier_Points {{0,0}, {0,0.5}, {0.5, 1}, {1, 1}}
static const int Default_Continentalness_Bezier_Count = 4;

#define Default_Erosion_Bezier_Points {{0,0}, {0,0.5}, {0.5, 1}, {1, 1}}
static const int Default_Erosion_Bezier_Count = 4;

#define Default_Peaks_And_Valleys_Bezier_Points {{0,0}, {0,0.5}, {0.5, 1}, {1, 1}}
static const int Default_Peaks_And_Valleys_Bezier_Count = 4;

static const Perlin_Fractal_Params Default_Continentalness_Perlin_Params = { 0.012, 3, 0.5, 1.5 };
static const Perlin_Fractal_Params Default_Erosion_Perlin_Params = { 0.012, 3, 0.5, 1.5 };
static const Perlin_Fractal_Params Default_Peaks_And_Valleys_Perlin_Params = { 0.012, 3, 0.5, 1.5 };

static const f64 Surface_Scale = 0.0172;
static const f64 Surface_Height_Threshold = 20;
static const f64 Surface_Level = 120;
static const f64 Cavern_Scale = 0.05674;

enum Terrain_Value
{
    Terrain_Value_Continentalness,
    Terrain_Value_Erosion,
    Terrain_Value_Peaks_And_Valleys,
};

struct Terrain_Values
{
    union
    {
        struct
        {
            f32 continentalness_noise;
            f32 erosion_noise;
            f32 peaks_and_valleys_noise;
        };
        f32 noise_values[3];
    };

    union
    {
        struct
        {
            f32 continentalness_bezier;
            f32 erosion_bezier;
            f32 peaks_and_valleys_bezier;
        };
        f32 bezier_values[3];
    };
};

struct Terrain_Params
{
    Vec2f continentalness_bezier_points[Terrain_Curves_Max_Points] = Default_Continentalness_Bezier_Points;
    int continentalness_bezier_point_count = Default_Continentalness_Bezier_Count;
    Vec2f erosion_bezier_points[Terrain_Curves_Max_Points] = Default_Erosion_Bezier_Points;
    int erosion_bezier_point_count = Default_Erosion_Bezier_Count;
    Vec2f peaks_and_valleys_bezier_points[Terrain_Curves_Max_Points] = Default_Peaks_And_Valleys_Bezier_Points;
    int peaks_and_valleys_bezier_point_count = Default_Peaks_And_Valleys_Bezier_Count;

    Perlin_Fractal_Params continentalness_perlin = Default_Continentalness_Perlin_Params;
    Perlin_Fractal_Params erosion_perlin = Default_Erosion_Perlin_Params;
    Perlin_Fractal_Params peaks_and_valleys_perlin = Default_Peaks_And_Valleys_Perlin_Params;
};

struct Chunk
{
    Chunk *east;
    Chunk *west;
    Chunk *north;
    Chunk *south;

    s64 x, z;

    s64 vertex_count;
    GLuint gl_vbo;
    GLuint opengl_is_stupid_vao;
    bool is_dirty;
    bool generated;

    Terrain_Values terrain_values[Chunk_Size * Chunk_Size];
    Block blocks[Chunk_Size * Chunk_Size * Chunk_Height];
};

#define chunk_block_index(x, y, z) ((y) * Chunk_Size * Chunk_Size + (x) * Chunk_Size + (z))

struct World
{
    s32 seed;

    Terrain_Params terrain_params;

    Vec2f continentalness_offsets[Perlin_Fractal_Max_Octaves];
    Vec2f erosion_offsets[Perlin_Fractal_Max_Octaves];
    Vec2f peaks_and_valleys_offsets[Perlin_Fractal_Max_Octaves];

    Chunk *origin_chunk;
    Hash_Map<Vec2i, Chunk *> all_loaded_chunks;
};

extern World g_world;

void chunk_init (Chunk *chunk, s64 x, s64 z);
void chunk_cleanup (Chunk *chunk);
Chunk *chunk_get_at_relative_coordinates (Chunk *chunk, s64 x, s64 y, s64 z);
Block chunk_get_block_in_chunk (Chunk *chunk, s64 x, s64 y, s64 z);
Block chunk_get_block (Chunk *chunk, s64 x, s64 y, s64 z);
void chunk_generate (World *world, Chunk *chunk);
void chunk_generate_mesh_data (Chunk *chunk);
void chunk_draw (Chunk *chunk, Camera *camera);

void world_init (World *world, s32 seed, int chunks_to_pre_generate = 0, Terrain_Params terrain_params = {});
Chunk *world_get_chunk (World *world, s64 x, s64 z);
Chunk *world_create_chunk (World *world, s64 x, s64 z);
Chunk *world_generate_chunk (World *world, s64 x, s64 z);
void world_draw_chunks (World *world, Camera *camera);
void world_clear_chunks (World *world);
