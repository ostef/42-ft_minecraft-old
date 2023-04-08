#include "Minecraft.hpp"

void chunk_init (Chunk *chunk, s64 x, s64 y, s64 z)
{
    memset (chunk, 0, sizeof (Chunk));

    chunk->x = x;
    chunk->y = y;
    chunk->z = z;

    array_init (&chunk->vertices, heap_allocator (), Chunk_Size * Chunk_Size * Chunk_Size);
    chunk->is_dirty = true;

    glGenVertexArrays (1, &chunk->opengl_is_stupid_vao);
    glBindVertexArray (chunk->opengl_is_stupid_vao);

    glGenBuffers (1, &chunk->gl_vbo);
    glBindBuffer (GL_ARRAY_BUFFER, chunk->gl_vbo);

    glEnableVertexAttribArray (0);
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), cast (void *) offsetof (Vertex, position));

    glEnableVertexAttribArray (1);
    glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), cast (void *) offsetof (Vertex, normal));

    glEnableVertexAttribArray (2);
    glVertexAttribPointer (2, 2, GL_FLOAT, GL_FALSE, sizeof (Vertex), cast (void *) offsetof (Vertex, tex_coords));

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
        chunk = chunk->south;
        *y += Chunk_Size;
    }
    while (chunk && *y >= Chunk_Size)
    {
        chunk = chunk->north;
        *y -= Chunk_Size;
    }
    while (chunk && *z < 0)
    {
        chunk = chunk->below;
        *z += Chunk_Size;
    }
    while (chunk && *z >= Chunk_Size)
    {
        chunk = chunk->above;
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

void chunk_generate (Chunk *chunk)
{
    static const f64 Surface_Scale = 0.0172;
    static const f64 Surface_Height_Threshold = 8;
    static const f64 Surface_Level = 16;
    static const f64 Cavern_Scale = 0.05674;

    if (chunk->generated)
        return;

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

                s64 index = i * Chunk_Size * Chunk_Size + j * Chunk_Size + k;

                auto surface = perlin_noise (x * Surface_Scale, 0, z * Surface_Scale);
                surface = (surface + 1) * 0.5;

                if (y > Surface_Level + surface * Surface_Height_Threshold)
                {
                    chunk->blocks[index].type = Block_Type_Air;
                }
                else if (y > Surface_Level + surface * Surface_Height_Threshold - Surface_Height_Threshold)
                {
                    chunk->blocks[index].type = Block_Type_Stone;
                }
                else
                {
                    auto val = perlin_noise (x * Cavern_Scale, y * Cavern_Scale, z * Cavern_Scale);
                    if (val < 0.2)
                        chunk->blocks[index].type = Block_Type_Air;
                    else
                        chunk->blocks[index].type = Block_Type_Stone;
                }
            }
        }
    }
}

typedef int Block_Face_Flags;
enum
{
    Block_Face_East  = 0x01, // +X
    Block_Face_West  = 0x02, // -X
    Block_Face_North = 0x04, // +Y
    Block_Face_South = 0x08, // -Y
    Block_Face_Above = 0x10, // +Z
    Block_Face_Below = 0x20, // -Z
};

void push_block (Chunk *chunk, const Vec3f &position, Block_Face_Flags visible_faces)
{
    if (visible_faces & Block_Face_East)
    {
        auto v = array_push (&chunk->vertices, {position, {1, 0, 0}});
        v->position += {0.5, -0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {1, 0, 0}});
        v->position += {0.5, 0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {1, 0, 0}});
        v->position += {0.5, 0.5, 0.5};

        v = array_push (&chunk->vertices, {position, {1, 0, 0}});
        v->position += {0.5, -0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {1, 0, 0}});
        v->position += {0.5, 0.5, 0.5};

        v = array_push (&chunk->vertices, {position, {1, 0, 0}});
        v->position += {0.5, -0.5, 0.5};
    }

    if (visible_faces & Block_Face_West)
    {
        auto v = array_push (&chunk->vertices, {position, {-1, 0, 0}});
        v->position += {-0.5, -0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {-1, 0, 0}});
        v->position += {-0.5, 0.5, 0.5};

        v = array_push (&chunk->vertices, {position, {-1, 0, 0}});
        v->position += {-0.5, 0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {-1, 0, 0}});
        v->position += {-0.5, -0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {-1, 0, 0}});
        v->position += {-0.5, -0.5, 0.5};

        v = array_push (&chunk->vertices, {position, {-1, 0, 0}});
        v->position += {-0.5, 0.5, 0.5};
    }

    if (visible_faces & Block_Face_North)
    {
        auto v = array_push (&chunk->vertices, {position, {0, 1, 0}});
        v->position += {-0.5, 0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {0, 1, 0}});
        v->position += {0.5, 0.5, 0.5};

        v = array_push (&chunk->vertices, {position, {0, 1, 0}});
        v->position += {0.5, 0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {0, 1, 0}});
        v->position += {-0.5, 0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {0, 1, 0}});
        v->position += {-0.5, 0.5, 0.5};

        v = array_push (&chunk->vertices, {position, {0, 1, 0}});
        v->position += {0.5, 0.5, 0.5};
    }

    if (visible_faces & Block_Face_South)
    {
        auto v = array_push (&chunk->vertices, {position, {0, -1, 0}});
        v->position += {-0.5, -0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {0, -1, 0}});
        v->position += {0.5, -0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {0, -1, 0}});
        v->position += {0.5, -0.5, 0.5};

        v = array_push (&chunk->vertices, {position, {0, -1, 0}});
        v->position += {-0.5, -0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {0, -1, 0}});
        v->position += {0.5, -0.5, 0.5};

        v = array_push (&chunk->vertices, {position, {0, -1, 0}});
        v->position += {-0.5, -0.5, 0.5};
    }

    if (visible_faces & Block_Face_Above)
    {
        auto v = array_push (&chunk->vertices, {position, {0, 0, 1}});
        v->position += {-0.5, -0.5, 0.5};

        v = array_push (&chunk->vertices, {position, {0, 0, 1}});
        v->position += {0.5, -0.5, 0.5};

        v = array_push (&chunk->vertices, {position, {0, 0, 1}});
        v->position += {0.5, 0.5, 0.5};

        v = array_push (&chunk->vertices, {position, {0, 0, 1}});
        v->position += {-0.5, -0.5, 0.5};

        v = array_push (&chunk->vertices, {position, {0, 0, 1}});
        v->position += {0.5, 0.5, 0.5};

        v = array_push (&chunk->vertices, {position, {0, 0, 1}});
        v->position += {-0.5, 0.5, 0.5};
    }

    if (visible_faces & Block_Face_Below)
    {
        auto v = array_push (&chunk->vertices, {position, {0, 0, -1}});
        v->position += {-0.5, -0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {0, 0, -1}});
        v->position += {0.5, 0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {0, 0, -1}});
        v->position += {0.5, -0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {0, 0, -1}});
        v->position += {-0.5, -0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {0, 0, -1}});
        v->position += {-0.5, 0.5, -0.5};

        v = array_push (&chunk->vertices, {position, {0, 0, -1}});
        v->position += {0.5, 0.5, -0.5};
    }
}

void chunk_generate_mesh_data (Chunk *chunk)
{
    if (!chunk->is_dirty)
       return;

    defer (chunk->is_dirty = false);

    array_clear (&chunk->vertices);

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
                    visible_faces |= Block_Face_East;
                if (chunk_get_block (chunk, x - 1, y, z).type == Block_Type_Air)
                    visible_faces |= Block_Face_West;
                if (chunk_get_block (chunk, x, y + 1, z).type == Block_Type_Air)
                    visible_faces |= Block_Face_North;
                if (chunk_get_block (chunk, x, y - 1, z).type == Block_Type_Air)
                    visible_faces |= Block_Face_South;
                if (chunk_get_block (chunk, x, y, z + 1).type == Block_Type_Air)
                    visible_faces |= Block_Face_Above;
                if (chunk_get_block (chunk, x, y, z - 1).type == Block_Type_Air)
                    visible_faces |= Block_Face_Below;

                push_block (chunk, position + Vec3f{cast (f32) x, cast (f32) y, cast (f32) z}, visible_faces);
            }
        }
    }

    glBindVertexArray (chunk->opengl_is_stupid_vao);
    glBindBuffer (GL_ARRAY_BUFFER, chunk->gl_vbo);

    glBufferData (GL_ARRAY_BUFFER, sizeof (Vertex) * chunk->vertices.count, chunk->vertices.data, GL_DYNAMIC_DRAW);

    glBindVertexArray (0);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
}

void world_init (World *world, int chunks_to_pre_generate)
{
    array_init (&world->all_loaded_chunks, heap_allocator ());

    world->origin_chunk = world_create_chunk (world, 0, 0, 0);
    chunk_generate (world->origin_chunk);

    for_range (i, -chunks_to_pre_generate, chunks_to_pre_generate)
    {
        for_range (j, -chunks_to_pre_generate, chunks_to_pre_generate)
        {
            for_range (k, -chunks_to_pre_generate, chunks_to_pre_generate)
            {
                auto chunk = world_create_chunk (world, i, j, k);
                chunk_generate (chunk);
            }
        }
    }
}

Chunk *world_get_chunk (World *world, s64 x, s64 y, s64 z)
{
    for_array (i, world->all_loaded_chunks)
    {
        auto chunk = world->all_loaded_chunks[i];
        if (chunk->x == x && chunk->y == y && chunk->z == z)
            return chunk;
    }

    return null;
}

Chunk *world_create_chunk (World *world, s64 x, s64 y, s64 z)
{
    Chunk *east = null;
    Chunk *west = null;
    Chunk *north = null;
    Chunk *south = null;
    Chunk *above = null;
    Chunk *below = null;

    for_array (i, world->all_loaded_chunks)
    {
        auto chunk = world->all_loaded_chunks[i];
        if (chunk->x == x && chunk->y == y && chunk->z == z)
            return chunk;

        if (chunk->x == x - 1 && chunk->y == y && chunk->z == z)
            west = chunk;
        else if (chunk->x == x + 1 && chunk->y == y && chunk->z == z)
            east = chunk;
        else if (chunk->x == x && chunk->y == y - 1 && chunk->z == z)
            south = chunk;
        else if (chunk->x == x && chunk->y == y + 1 && chunk->z == z)
            north = chunk;
        else if (chunk->x == x && chunk->y == y && chunk->z == z - 1)
            below = chunk;
        else if (chunk->x == x && chunk->y == y && chunk->z == z + 1)
            above = chunk;
    }

    Chunk *chunk = mem_alloc_uninit (Chunk, 1, heap_allocator ());
    println ("[WORLD] Created new chunk at %lld %lld %lld", x, y, z);
    chunk_init (chunk, x, y, z);
    chunk->east = east;
    chunk->west = west;
    chunk->north = north;
    chunk->south = south;
    chunk->above = above;
    chunk->below = below;

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

    array_push (&world->all_loaded_chunks, chunk);

    return chunk;
}
