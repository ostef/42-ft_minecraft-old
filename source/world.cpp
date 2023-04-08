#include "Minecraft.hpp"

void chunk_init (Chunk *chunk, s64 x, s64 y, s64 z)
{
    memset (chunk, 0, sizeof (Chunk));

    chunk->x = x;
    chunk->y = y;
    chunk->z = z;

    array_init (&chunk->vertices, heap_allocator (), Chunk_Size * Chunk_Size * Chunk_Size);
    chunk->is_dirty = true;

    glGenBuffers (1, &chunk->gl_vbo);
    glBindBuffer (GL_ARRAY_BUFFER, chunk->gl_vbo);

    glEnableVertexAttribArray (0);
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), cast (void *) offsetof (Vertex, position));

    glEnableVertexAttribArray (1);
    glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, sizeof (Vertex), cast (void *) offsetof (Vertex, normal));

    glEnableVertexAttribArray (2);
    glVertexAttribPointer (2, 2, GL_FLOAT, GL_FALSE, sizeof (Vertex), cast (void *) offsetof (Vertex, tex_coords));

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

    glBindBuffer (GL_ARRAY_BUFFER, chunk->gl_vbo);
    glBufferData (GL_ARRAY_BUFFER, sizeof (Vertex) * chunk->vertices.count, chunk->vertices.data, GL_DYNAMIC_DRAW);
    glBindBuffer (GL_ARRAY_BUFFER, 0);
}
