#include "Minecraft.hpp"

void chunk_init (Chunk *chunk, s64 x, s64 y, s64 z)
{
    memset (chunk, 0, sizeof (Chunk));

    chunk->x = x;
    chunk->y = y;
    chunk->z = z;

    chunk->is_dirty = true;

    glGenVertexArrays (1, &chunk->opengl_is_stupid_vao);
    glBindVertexArray (chunk->opengl_is_stupid_vao);

    glGenBuffers (1, &chunk->gl_vbo);
    glBindBuffer (GL_ARRAY_BUFFER, chunk->gl_vbo);

    glEnableVertexAttribArray (0);
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), cast (void *) offsetof (Vertex, position));

    glEnableVertexAttribArray (1);
    glVertexAttribIPointer (1, 1, GL_UNSIGNED_BYTE, sizeof (Vertex), cast (void *) offsetof (Vertex, face));

    glEnableVertexAttribArray (2);
    glVertexAttribIPointer (2, 1, GL_UNSIGNED_BYTE, sizeof (Vertex), cast (void *) offsetof (Vertex, block_id));

    glEnableVertexAttribArray (3);
    glVertexAttribIPointer (3, 1, GL_UNSIGNED_BYTE, sizeof (Vertex), cast (void *) offsetof (Vertex, block_corner));

    glBindVertexArray (0);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
}

Chunk *chunk_get_at_relative_coordinates (Chunk *chunk, s64 *x, s64 *y, s64 *z)
{
    while (chunk && *x < 0)
    {
        chunk = chunk->west;
        *x += Chunk_Size;
    }
    while (chunk && *x >= Chunk_Size)
    {
        chunk = chunk->east;
        *x -= Chunk_Size;
    }
    while (chunk && *y < 0)
    {
        chunk = chunk->below;
        *y += Chunk_Size;
    }
    while (chunk && *y >= Chunk_Size)
    {
        chunk = chunk->above;
        *y -= Chunk_Size;
    }
    while (chunk && *z < 0)
    {
        chunk = chunk->south;
        *z += Chunk_Size;
    }
    while (chunk && *z >= Chunk_Size)
    {
        chunk = chunk->north;
        *z -= Chunk_Size;
    }

    return chunk;
}

inline
Block chunk_get_block_in_chunk (Chunk *chunk, s64 x, s64 y, s64 z)
{
    return chunk->blocks[x * Chunk_Size * Chunk_Size + y * Chunk_Size + z];
}

Block chunk_get_block (Chunk *chunk, s64 x, s64 y, s64 z)
{
    chunk = chunk_get_at_relative_coordinates (chunk, &x, &y, &z);
    if (!chunk) // Chunk is not loaded
        return Block_Air;

    return chunk_get_block_in_chunk (chunk, x, y, z);
}

static const f64 Surface_Scale = 0.0172;
static const f64 Surface_Height_Threshold = 8;
static const f64 Surface_Level = 64;
static const f64 Cavern_Scale = 0.05674;

void chunk_generate (World *world, Chunk *chunk)
{
    if (chunk->generated)
        return;

    LC_RNG rng;
    random_seed (&rng, world->seed);
    f64 offset_x = cast (f64) random_get (&rng);
    f64 offset_y = cast (f64) random_get (&rng);
    f64 offset_z = cast (f64) random_get (&rng);

    defer (chunk->generated = true);

    for_range (i, 0, Chunk_Size)
    {
        for_range (j, 0, Chunk_Size)
        {
            for_range (k, 0, Chunk_Size)
            {
                f64 x = cast (f64) (chunk->x * Chunk_Size + i);
                f64 y = cast (f64) (chunk->y * Chunk_Size + j);
                f64 z = cast (f64) (chunk->z * Chunk_Size + k);
                f64 perlin_x = x + offset_x;
                f64 perlin_y = y + offset_y;
                f64 perlin_z = z + offset_z;

                s64 index = i * Chunk_Size * Chunk_Size + j * Chunk_Size + k;

                auto surface = perlin_noise (perlin_x * Surface_Scale, perlin_z * Surface_Scale);
                surface = (surface + 1) * 0.5;

                if (y > Surface_Level + surface * Surface_Height_Threshold)
                {
                    chunk->blocks[index].type = Block_Type_Air;
                }
                else if (y > Surface_Level + surface * Surface_Height_Threshold - Surface_Height_Threshold)
                {
                    chunk->blocks[index].type = Block_Type_Dirt;
                }
                else if (y == Min_Chunk_Y * Chunk_Size)
                {
                    chunk->blocks[index].type = Block_Type_Bedrock;
                }
                else
                {
                    auto val = perlin_noise (perlin_x * Cavern_Scale, perlin_y * Cavern_Scale, perlin_z * Cavern_Scale);
                    if (val < 0.2)
                        chunk->blocks[index].type = Block_Type_Air;
                    else
                        chunk->blocks[index].type = Block_Type_Stone;
                }
            }
        }
    }
}

void push_block (Array<Vertex> *vertices, u8 id, const Vec3f &position, Block_Face_Flags visible_faces)
{
    if (visible_faces & Block_Face_Flag_East)
    {
        auto v = array_push (vertices, {position, Block_Face_East, id, Block_Corner_Bottom_Left});
        v->position += {0.5, -0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_East, id, Block_Corner_Top_Left});
        v->position += {0.5, 0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_East, id, Block_Corner_Top_Right});
        v->position += {0.5, 0.5, 0.5};

        v = array_push (vertices, {position, Block_Face_East, id, Block_Corner_Bottom_Left});
        v->position += {0.5, -0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_East, id, Block_Corner_Top_Right});
        v->position += {0.5, 0.5, 0.5};

        v = array_push (vertices, {position, Block_Face_East, id, Block_Corner_Bottom_Right});
        v->position += {0.5, -0.5, 0.5};
    }

    if (visible_faces & Block_Face_Flag_West)
    {
        auto v = array_push (vertices, {position, Block_Face_West, id, Block_Corner_Bottom_Right});
        v->position += {-0.5, -0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_West, id, Block_Corner_Top_Left});
        v->position += {-0.5, 0.5, 0.5};

        v = array_push (vertices, {position, Block_Face_West, id, Block_Corner_Top_Right});
        v->position += {-0.5, 0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_West, id, Block_Corner_Bottom_Right});
        v->position += {-0.5, -0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_West, id, Block_Corner_Bottom_Left});
        v->position += {-0.5, -0.5, 0.5};

        v = array_push (vertices, {position, Block_Face_West, id, Block_Corner_Top_Left});
        v->position += {-0.5, 0.5, 0.5};
    }

    if (visible_faces & Block_Face_Flag_Above)
    {
        auto v = array_push (vertices, {position, Block_Face_Above, id, Block_Corner_Bottom_Left});
        v->position += {-0.5, 0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_Above, id, Block_Corner_Top_Right});
        v->position += {0.5, 0.5, 0.5};

        v = array_push (vertices, {position, Block_Face_Above, id, Block_Corner_Bottom_Right});
        v->position += {0.5, 0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_Above, id, Block_Corner_Bottom_Left});
        v->position += {-0.5, 0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_Above, id, Block_Corner_Top_Left});
        v->position += {-0.5, 0.5, 0.5};

        v = array_push (vertices, {position, Block_Face_Above, id, Block_Corner_Top_Right});
        v->position += {0.5, 0.5, 0.5};
    }

    if (visible_faces & Block_Face_Flag_Below)
    {
        auto v = array_push (vertices, {position, Block_Face_Below, id, Block_Corner_Top_Left});
        v->position += {-0.5, -0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_Below, id, Block_Corner_Top_Right});
        v->position += {0.5, -0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_Below, id, Block_Corner_Bottom_Right});
        v->position += {0.5, -0.5, 0.5};

        v = array_push (vertices, {position, Block_Face_Below, id, Block_Corner_Top_Left});
        v->position += {-0.5, -0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_Below, id, Block_Corner_Bottom_Right});
        v->position += {0.5, -0.5, 0.5};

        v = array_push (vertices, {position, Block_Face_Below, id, Block_Corner_Bottom_Left});
        v->position += {-0.5, -0.5, 0.5};
    }

    if (visible_faces & Block_Face_Flag_North)
    {
        auto v = array_push (vertices, {position, Block_Face_North, id, Block_Corner_Bottom_Right});
        v->position += {-0.5, -0.5, 0.5};

        v = array_push (vertices, {position, Block_Face_North, id, Block_Corner_Bottom_Left});
        v->position += {0.5, -0.5, 0.5};

        v = array_push (vertices, {position, Block_Face_North, id, Block_Corner_Top_Left});
        v->position += {0.5, 0.5, 0.5};

        v = array_push (vertices, {position, Block_Face_North, id, Block_Corner_Bottom_Right});
        v->position += {-0.5, -0.5, 0.5};

        v = array_push (vertices, {position, Block_Face_North, id, Block_Corner_Top_Left});
        v->position += {0.5, 0.5, 0.5};

        v = array_push (vertices, {position, Block_Face_North, id, Block_Corner_Top_Right});
        v->position += {-0.5, 0.5, 0.5};
    }

    if (visible_faces & Block_Face_Flag_South)
    {
        auto v = array_push (vertices, {position, Block_Face_South, id, Block_Corner_Bottom_Left});
        v->position += {-0.5, -0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_South, id, Block_Corner_Top_Right});
        v->position += {0.5, 0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_South, id, Block_Corner_Bottom_Right});
        v->position += {0.5, -0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_South, id, Block_Corner_Bottom_Left});
        v->position += {-0.5, -0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_South, id, Block_Corner_Top_Left});
        v->position += {-0.5, 0.5, -0.5};

        v = array_push (vertices, {position, Block_Face_South, id, Block_Corner_Top_Right});
        v->position += {0.5, 0.5, -0.5};
    }
}

void chunk_generate_mesh_data (Chunk *chunk)
{
    if (!chunk->is_dirty)
       return;

    auto state = arena_get_state (&frame_arena);
    defer (arena_set_state (&frame_arena, state));

    Array<Vertex> vertices;
    array_init (&vertices, frame_allocator, 12000);

    defer (chunk->vertex_count = vertices.count);
    defer (chunk->is_dirty = false);

    Vec3f position = {cast (f32) chunk->x * Chunk_Size, cast (f32) chunk->y * Chunk_Size, cast (f32) chunk->z * Chunk_Size};
    for_range (x, 0, Chunk_Size)
    {
        for_range (y, 0, Chunk_Size)
        {
            for_range (z, 0, Chunk_Size)
            {
                auto block = chunk_get_block_in_chunk (chunk, x, y, z);
                if (block.type == Block_Type_Air)
                    continue;

                Block_Face_Flags visible_faces = 0;
                if (chunk_get_block (chunk, x + 1, y, z).type == Block_Type_Air)
                    visible_faces |= Block_Face_Flag_East;
                if (chunk_get_block (chunk, x - 1, y, z).type == Block_Type_Air)
                    visible_faces |= Block_Face_Flag_West;
                if (chunk_get_block (chunk, x, y + 1, z).type == Block_Type_Air)
                    visible_faces |= Block_Face_Flag_Above;
                if (chunk_get_block (chunk, x, y - 1, z).type == Block_Type_Air)
                    visible_faces |= Block_Face_Flag_Below;
                if (chunk_get_block (chunk, x, y, z + 1).type == Block_Type_Air)
                    visible_faces |= Block_Face_Flag_North;
                if (chunk_get_block (chunk, x, y, z - 1).type == Block_Type_Air)
                    visible_faces |= Block_Face_Flag_South;

                push_block (&vertices, cast (u8) block.type, position + Vec3f{cast (f32) x, cast (f32) y, cast (f32) z}, visible_faces);
            }
        }
    }

    glBindVertexArray (chunk->opengl_is_stupid_vao);
    glBindBuffer (GL_ARRAY_BUFFER, chunk->gl_vbo);

    glBufferData (GL_ARRAY_BUFFER, sizeof (Vertex) * vertices.count, vertices.data, GL_DYNAMIC_DRAW);

    glBindVertexArray (0);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
}

u32 hash_vec2i (const Vec2i &v)
{
    return hash_combine (hash_s32 (v.x), hash_s32 (v.y));
}

bool compare_vec2i (const Vec2i &a, const Vec2i &b)
{
    return a == b;
}

void world_init (World *world, s32 seed, int chunks_to_pre_generate)
{
    memset (world, 0, sizeof (World));

    world->seed = seed;

    hash_map_init (&world->all_loaded_chunks, hash_vec2i, compare_vec2i, heap_allocator ());

    world->origin_chunk = world_create_chunk (world, 0, 0, 0);
    chunk_generate (world, world->origin_chunk);

    for_range (i, -chunks_to_pre_generate, chunks_to_pre_generate)
    {
        for_range (j, -chunks_to_pre_generate, chunks_to_pre_generate)
        {
            for_range (k, -chunks_to_pre_generate, chunks_to_pre_generate)
            {
                auto chunk = world_create_chunk (world, i, j, k);
                chunk_generate (world, chunk);
            }
        }
    }

    println ("[WORLD] Created world with seed %d", seed);
}

Chunk_Column *world_get_chunk_column (World *world, s64 x, s64 z)
{
    return hash_map_get (&world->all_loaded_chunks, {cast (s32) x, cast (s32) z});
}

Chunk *world_get_chunk (World *world, s64 x, s64 y, s64 z)
{
    assert (y >= Min_Chunk_Y && y <= Max_Chunk_Y, "y is %lld", y);

    auto column = hash_map_get (&world->all_loaded_chunks, {cast (s32) x, cast (s32) z});
    if (!column)
        return null;

    return (*column)[y - Min_Chunk_Y];
}

Chunk *world_create_chunk (World *world, s64 x, s64 y, s64 z)
{
    assert (y >= Min_Chunk_Y && y <= Max_Chunk_Y, "y is %lld", y);

    auto column = world_get_chunk_column (world, x, z);
    if (column && (*column)[y - Min_Chunk_Y])
        return (*column)[y - Min_Chunk_Y];

    auto chunk = mem_alloc_uninit (Chunk, 1, heap_allocator ());
    println ("[WORLD] Created new chunk at %lld %lld %lld", x, y, z);
    chunk_init (chunk, x, y, z);
    chunk->east  = world_get_chunk (world, x + 1, y, z);
    chunk->west  = world_get_chunk (world, x - 1, y, z);
    chunk->north = world_get_chunk (world, x, y, z + 1);
    chunk->south = world_get_chunk (world, x, y, z - 1);
    chunk->above = y < Max_Chunk_Y ? world_get_chunk (world, x, y + 1, z) : null;
    chunk->below = y > Min_Chunk_Y ? world_get_chunk (world, x, y - 1, z) : null;

    if (chunk->east)
    {
        chunk->east->west = chunk;
        chunk->east->is_dirty = true;
    }
    if (chunk->west)
    {
        chunk->west->east = chunk;
        chunk->west->is_dirty = true;
    }
    if (chunk->north)
    {
        chunk->north->south = chunk;
        chunk->north->is_dirty = true;
    }
    if (chunk->south)
    {
        chunk->south->north = chunk;
        chunk->south->is_dirty = true;
    }
    if (chunk->above)
    {
        chunk->above->below = chunk;
        chunk->above->is_dirty = true;
    }
    if (chunk->below)
    {
        chunk->below->above = chunk;
        chunk->below->is_dirty = true;
    }

    if (!column)
    {
        column = hash_map_insert (&world->all_loaded_chunks, {cast (s32) x, cast (s32) z}).ptr;
        memset (*column, 0, sizeof (*column));
    }

    (*column)[y - Min_Chunk_Y] = chunk;

    return chunk;
}
