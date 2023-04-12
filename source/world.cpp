#include "Minecraft.hpp"

void chunk_init (Chunk *chunk, s64 x, s64 z)
{
    memset (chunk, 0, offsetof (Chunk, terrain_values));

    chunk->x = x;
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

void chunk_cleanup (Chunk *chunk)
{
    glDeleteVertexArrays (1, &chunk->opengl_is_stupid_vao);
    glDeleteBuffers (1, &chunk->gl_vbo);
}

Chunk *chunk_get_at_relative_coordinates (Chunk *chunk, s64 *x, s64 *z)
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
Block *chunk_get_block_ptr_in_chunk (Chunk *chunk, s64 x, s64 y, s64 z)
{
    assert (x >= 0 && x < Chunk_Size && y >= 0 && y < Chunk_Height && z >= 0 && z < Chunk_Size);
    // Blocks are layed out by layers on the y axis,
    chunk->blocks[chunk_block_index (x, y, z)];
}

inline
Block chunk_get_block_in_chunk (Chunk *chunk, s64 x, s64 y, s64 z)
{
    assert (x >= 0 && x < Chunk_Size && z >= 0 && z < Chunk_Size);

    if (y < 0 || y >= Chunk_Height)
        return Block_Air;

    return chunk->blocks[chunk_block_index (x, y, z)];
}

inline
Block chunk_get_block (Chunk *chunk, s64 x, s64 y, s64 z)
{
    if (y < 0 || y > Chunk_Height)
        return Block_Air;

    chunk = chunk_get_at_relative_coordinates (chunk, &x, &z);
    if (!chunk) // Chunk is not loaded
        return Block_Air;

    return chunk_get_block_in_chunk (chunk, x, y, z);
}

void chunk_generate (World *world, Chunk *chunk)
{
    if (chunk->generated)
        return;

    LC_RNG rng;
    random_seed (&rng, world->seed);
    f64 offset_x = random_rangef (&rng, -10000, 10000);
    f64 offset_y = random_rangef (&rng, -10000, 10000);
    f64 offset_z = random_rangef (&rng, -10000, 10000);

    defer (chunk->generated = true);

    f32 continentalness_max = perlin_fractal_max (world->terrain_params.continentalness_perlin.octaves, world->terrain_params.continentalness_perlin.persistance);
    f32 erosion_max = perlin_fractal_max (world->terrain_params.erosion_perlin.octaves, world->terrain_params.erosion_perlin.persistance);
    f32 peaks_and_valleys_max = perlin_fractal_max (world->terrain_params.peaks_and_valleys_perlin.octaves, world->terrain_params.peaks_and_valleys_perlin.persistance);

    for_range (x, 0, Chunk_Size)
    {
        for_range (z, 0, Chunk_Size)
        {
            f64 perlin_x = cast (f64) (x + chunk->x * Chunk_Size);
            f64 perlin_z = cast (f64) (z + chunk->z * Chunk_Size);

            auto values = &chunk->terrain_values[x * Chunk_Size + z];
            values->continentalness = perlin_fractal_noise (world->terrain_params.continentalness_perlin, world->continentalness_offsets, perlin_x, perlin_z);
            values->continentalness = inverse_lerp (-continentalness_max, continentalness_max, values->continentalness);

            values->erosion = perlin_fractal_noise (world->terrain_params.erosion_perlin, world->erosion_offsets, perlin_x, perlin_z);
            values->erosion = inverse_lerp (-erosion_max, erosion_max, values->erosion);

            values->peaks_and_valleys = perlin_fractal_noise (world->terrain_params.peaks_and_valleys_perlin, world->peaks_and_valleys_offsets, perlin_x, perlin_z);
            values->peaks_and_valleys = inverse_lerp (-peaks_and_valleys_max, peaks_and_valleys_max, values->peaks_and_valleys);
        }
    }

    for_range (i, 0, Chunk_Size)
    {
        for_range (j, 0, Chunk_Height)
        {
            for_range (k, 0, Chunk_Size)
            {
                f64 x = cast (f64) (chunk->x * Chunk_Size + i);
                f64 y = cast (f64) j;
                f64 z = cast (f64) (chunk->z * Chunk_Size + k);
                f64 perlin_x = x + offset_x;
                f64 perlin_y = y + offset_y;
                f64 perlin_z = z + offset_z;

                s64 index = chunk_block_index (i, j, k);

                if (y > Surface_Level)
                {
                    chunk->blocks[index].type = Block_Type_Air;
                }
                else if (y > Surface_Level - Surface_Height_Threshold)
                {
                    chunk->blocks[index].type = Block_Type_Dirt;
                }
                else if (j == 0)
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

    Vec3f position = {cast (f32) chunk->x * Chunk_Size, 0, cast (f32) chunk->z * Chunk_Size};
    for_range (x, 0, Chunk_Size)
    {
        for_range (y, 0, Chunk_Height)
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

void world_init (World *world, s32 seed, int chunks_to_pre_generate, Terrain_Params terrain_params)
{
    memset (world, 0, sizeof (World));

    world->seed = seed;
    world->terrain_params = terrain_params;

    LC_RNG rng;
    random_seed (&rng, seed);
    random_seed (&rng, random_get_s32 (&rng));

    for_range (i, 0, world->terrain_params.continentalness_perlin.octaves)
    {
        world->continentalness_offsets[i].x = random_rangef (&rng, -10000, 10000);
        world->continentalness_offsets[i].y = random_rangef (&rng, -10000, 10000);
    }

    for_range (i, 0, world->terrain_params.erosion_perlin.octaves)
    {
        world->erosion_offsets[i].x = random_rangef (&rng, -10000, 10000);
        world->erosion_offsets[i].y = random_rangef (&rng, -10000, 10000);
    }

    for_range (i, 0, world->terrain_params.peaks_and_valleys_perlin.octaves)
    {
        world->peaks_and_valleys_offsets[i].x = random_rangef (&rng, -10000, 10000);
        world->peaks_and_valleys_offsets[i].y = random_rangef (&rng, -10000, 10000);
    }

    hash_map_init (&world->all_loaded_chunks, hash_vec2i, compare_vec2i, heap_allocator ());

    world->origin_chunk = world_create_chunk (world, 0, 0);
    chunk_generate (world, world->origin_chunk);

    for_range (i, -chunks_to_pre_generate, chunks_to_pre_generate)
    {
        for_range (k, -chunks_to_pre_generate, chunks_to_pre_generate)
        {
            auto chunk = world_create_chunk (world, i, k);
            chunk_generate (world, chunk);
        }
    }

    println ("[WORLD] Created world with seed %d", seed);
}

Chunk *world_get_chunk (World *world, s64 x, s64 z)
{
    auto chunk_ptr = hash_map_get (&world->all_loaded_chunks, {cast (s32) x, cast (s32) z});
    if (!chunk_ptr)
        return null;

    return *chunk_ptr;
}

Chunk *world_create_chunk (World *world, s64 x, s64 z)
{
    auto chunk = world_get_chunk (world, x, z);
    if (chunk)
        return chunk;

    chunk = mem_alloc_uninit (Chunk, 1, heap_allocator ());
    println ("[WORLD] Created new chunk at %lld %lld", x, z);
    chunk_init (chunk, x, z);
    chunk->east  = world_get_chunk (world, x + 1, z);
    chunk->west  = world_get_chunk (world, x - 1, z);
    chunk->north = world_get_chunk (world, x, z + 1);
    chunk->south = world_get_chunk (world, x, z - 1);

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

    hash_map_insert (&world->all_loaded_chunks, {cast (s32) x, cast (s32) z}, chunk);

    return chunk;
}

void world_clear_chunks (World *world)
{
    for_hash_map (it, world->all_loaded_chunks)
    {
        chunk_cleanup (*it.value);
        mem_free (*it.value, heap_allocator ());
        hash_map_it_remove (&world->all_loaded_chunks, it);
    }

    world->origin_chunk = null;
}
