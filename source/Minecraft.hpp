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
extern int g_generation_height;

struct Vertex;
struct Camera;
struct Block;
enum Block_Type : u8;
struct Chunk;
struct World;

f64 perlin_noise (f64 x, f64 y);
f64 perlin_noise (f64 x, f64 y, f64 z);
f64 perlin_fractal_max (int octaves, f64 persistance);
f64 perlin_fractal_noise (f64 scale, int octaves, Vec2f *offsets, f64 persistance, f64 lacunarity, f64 x, f64 y);
f64 perlin_fractal_noise (f64 scale, int octaves, Vec2f *offsets, f64 persistance, f64 lacunarity, f64 x, f64 y, f64 z);

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

const Block Block_Air = {};

const int Chunk_Size = 16;

struct Chunk
{
    Chunk *east;
    Chunk *west;
    Chunk *north;
    Chunk *south;
    Chunk *below;
    Chunk *above;

    s64 x, y, z;

    s64 vertex_count;
    GLuint gl_vbo;
    GLuint opengl_is_stupid_vao;
    bool is_dirty;
    bool generated;

    Block blocks[Chunk_Size * Chunk_Size * Chunk_Size];
};

const int Min_Chunk_Y = -4;    // -64
const int Max_Chunk_Y = 20;    // 320

typedef Chunk *Chunk_Column[Max_Chunk_Y - Min_Chunk_Y + 1];

struct World
{
    s32 seed;
    Chunk *origin_chunk;
    Hash_Map<Vec2i, Chunk_Column> all_loaded_chunks;
};

extern World g_world;

void chunk_init (Chunk *chunk, s64 x, s64 y, s64 z);
Chunk *chunk_get_at_relative_coordinates (Chunk *chunk, s64 x, s64 y, s64 z);
Block chunk_get_block_in_chunk (Chunk *chunk, s64 x, s64 y, s64 z);
Block chunk_get_block (Chunk *chunk, s64 x, s64 y, s64 z);
void chunk_generate (World *world, Chunk *chunk);
void chunk_generate_mesh_data (Chunk *chunk);
void chunk_draw (Chunk *chunk, Camera *camera);

void world_init (World *world, s32 seed, int chunks_to_pre_generate = 0);
Chunk_Column *world_get_chunk_column (World *world, s64 x, s64 z);
Chunk *world_get_chunk (World *world, s64 x, s64 y, s64 z);
Chunk *world_create_chunk (World *world, s64 x, s64 y, s64 z);
Chunk *world_generate_chunk (World *world, s64 x, s64 y, s64 z);
void world_draw_chunks (World *world, Camera *camera);
