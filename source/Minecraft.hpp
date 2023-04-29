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
f64 perlin_fractal_noise (f64 scale, int octaves, Vec2f *offsets, f64 persistance, f64 lacunarity, f64 x, f64 y, f64 z);
f64 perlin_fractal_noise (Perlin_Fractal_Params params, Vec2f *offsets, f64 x, f64 y, f64 z);
void perlin_generate_offsets (LC_RNG *rng, int count, Vec2f *offsets);

Vec2f bezier_cubic_calculate (int count, Vec2f *points, f32 t);

inline
static Vec2f bezier_cubic_calculate (const Vec2f &p1, const Vec2f &p2, const Vec2f &p3, const Vec2f &p4, f32 t)
{
    f32 u = 1.0f - t;
    f32 w1 = u * u * u;
    f32 w2 = 3 * u * u * t;
    f32 w3 = 3 * u * t * t;
    f32 w4 = t * t * t;

    return {w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x, w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y};
}

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

namespace ImGuiExt
{
   struct NestedHermiteSplineEditorData
   {
       Nested_Hermite_Spline *root_spline;
       Static_Array<Nested_Hermite_Spline *, 20> spline_stack;
       s64 index_in_stack;
       s64 selected_knot;
   };

    bool NestedHermiteSplineEditor (const char *str_id, const ImVec2 &size, NestedHermiteSplineEditorData *data, const Slice<f32> &t_values,
       const char *zero_separated_t_value_names = null);

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

static const int Terrain_Curves_Max_Points = ImGuiExt_BezierSpline_PointCountFromCurveCount (10);

// The bezier points need to be a #define because we can't assign fixed arrays

#define Default_Continentalness_Bezier_Points {\
    {0.000000, 0.000000}, {0.043313, 0.003790}, {0.294532, -0.016243},\
    {0.311857, 0.000000}, {0.329182, 0.016243}, {0.317271, 0.400830},\
    {0.335679, 0.417073}, {0.354088, 0.433315}, {0.507851, 0.408049},\
    {0.519762, 0.429706}, {0.531673, 0.451363}, {0.523010, 0.861036},\
    {0.546833, 0.880888}, {0.570655, 0.900740}, {0.753655, 0.985562},\
    {1.000000, 1.000000},\
}

static const int Default_Continentalness_Bezier_Count = 16;

#define Default_Erosion_Bezier_Points {\
    {0.000000, 1.000000}, {0.022740, 0.935030}, {0.055762, 0.836182},\
    {0.070922, 0.816330}, {0.121815, 0.816330}, {0.198854, 0.510862},\
    {0.234588, 0.516276}, {0.270322, 0.521690}, {0.278889, 0.663070},\
    {0.306443, 0.689587}, {0.333997, 0.716104}, {0.410395, 0.155387},\
    {0.430969, 0.139145}, {0.451543, 0.122902}, {0.708175, 0.121097},\
    {0.737412, 0.126512}, {0.766649, 0.131926}, {0.760152, 0.399025},\
    {0.777477, 0.406244}, {0.794802, 0.413463}, {0.834867, 0.417073},\
    {0.847861, 0.406244}, {0.860855, 0.395416}, {0.870601, 0.128316},\
    {0.896589, 0.122902}, {0.951814, 0.119293}, {0.922577, 0.054322},\
    {1.000000, 0.048908},\
}

static const int Default_Erosion_Bezier_Count = 28;

#define Default_Peaks_And_Valleys_Bezier_Points {\
    {0.000000, 0.929078}, {0.082924, 0.878160}, {0.133115, 1.041826},\
    {0.182215, 0.856338}, {0.231315, 0.670849}, {0.232406, 0.447172},\
    {0.252046, 0.410802}, {0.377523, 0.416258}, {0.400000, 0.300000},\
    {0.500000, -0.000000}, {0.600000, 0.300000}, {0.623022, 0.416258},\
    {0.744135, 0.414439}, {0.771413, 0.447172}, {0.771413, 0.669031},\
    {0.822695, 0.849063}, {0.873977, 1.029096}, {0.927441, 0.890889},\
    {1.000000, 0.925441},\
}

static const int Default_Peaks_And_Valleys_Bezier_Count = 19;

static const Vec2f Default_Bezier_Points[3][Terrain_Curves_Max_Points] = {
    Default_Continentalness_Bezier_Points,
    Default_Erosion_Bezier_Points,
    Default_Peaks_And_Valleys_Bezier_Points,
};

static const int Default_Bezier_Point_Counts[3] = {
    Default_Continentalness_Bezier_Count,
    Default_Erosion_Bezier_Count,
    Default_Peaks_And_Valleys_Bezier_Count,
};

static const f32 Default_Influences[3] = {
    1.0f,
    0.6f,
    0.7f,
};

static const Vec2i Default_Height_Range = {50,355};

static const Perlin_Fractal_Params Default_Continentalness_Perlin_Params = { 0.001340, 3, 0.25, 1.3 };
static const Perlin_Fractal_Params Default_Erosion_Perlin_Params = { 0.002589, 5, 0.5, 1.5 };
static const Perlin_Fractal_Params Default_Peaks_And_Valleys_Perlin_Params = { 0.033, 3, 0.5, 1.8 };

static const Perlin_Fractal_Params Default_Perlin_Params[3] = {
    Default_Continentalness_Perlin_Params,
    Default_Erosion_Perlin_Params,
    Default_Peaks_And_Valleys_Perlin_Params,
};

static const int Default_Water_Level = 126;

static const f64 Surface_Scale = 0.0172;
static const f64 Surface_Height_Threshold = 50;
static const f64 Surface_Dirt_Height = 8;
static const f64 Surface_Level = 120;
static const f64 Cavern_Scale = 0.05674;

enum Terrain_Value
{
    Terrain_Value_Continentalness,
    Terrain_Value_Erosion,
    Terrain_Value_Peaks_And_Valleys,
    Terrain_Value_Count,
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

    f32 surface_level;
};

struct Terrain_Params
{
    union
    {
        Vec2f bezier_points[3][Terrain_Curves_Max_Points] = {
            Default_Continentalness_Bezier_Points,
            Default_Erosion_Bezier_Points,
            Default_Peaks_And_Valleys_Bezier_Points,
        };
        struct
        {
            Vec2f continentalness_bezier_points[Terrain_Curves_Max_Points];
            Vec2f erosion_bezier_points[Terrain_Curves_Max_Points];
            Vec2f peaks_and_valleys_bezier_points[Terrain_Curves_Max_Points];
        };
    };

    union
    {
        int bezier_point_counts[3] = {
            Default_Continentalness_Bezier_Count,
            Default_Erosion_Bezier_Count,
            Default_Peaks_And_Valleys_Bezier_Count,
        };
        struct
        {
            int continentalness_bezier_point_count;
            int erosion_bezier_point_count;
            int peaks_and_valleys_bezier_point_count;
        };
    };

    f32 influences[3] = {
        Default_Influences[0],
        Default_Influences[1],
        Default_Influences[2],
    };

    union
    {
        Perlin_Fractal_Params perlin_params[3] = {
            Default_Continentalness_Perlin_Params,
            Default_Erosion_Perlin_Params,
            Default_Peaks_And_Valleys_Perlin_Params
        };
        struct
        {
            Perlin_Fractal_Params continentalness_perlin;
            Perlin_Fractal_Params erosion_perlin;
            Perlin_Fractal_Params peaks_and_valleys_perlin;
        };
    };

    Vec2i height_range = Default_Height_Range;
    int water_level = Default_Water_Level;
};

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

    Vec2f continentalness_offsets[Perlin_Fractal_Max_Octaves];
    Vec2f erosion_offsets[Perlin_Fractal_Max_Octaves];
    Vec2f peaks_and_valleys_offsets[Perlin_Fractal_Max_Octaves];

    Chunk *origin_chunk;
    Hash_Map<Vec2i, Chunk *> all_loaded_chunks;
};

extern World g_world;

void chunk_init (Chunk *chunk, s64 x, s64 z);
void chunk_cleanup (Chunk *chunk);
Vec2i chunk_absolute_to_relative_coordinates (Chunk *chunk, s64 x, s64 z);
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
