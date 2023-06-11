#include "Minecraft.hpp"

void chunk_init (Chunk *chunk, s64 x, s64 z)
{
    memset (chunk, 0, offsetof (Chunk, terrain_values));

    chunk->x = x;
    chunk->z = z;

    chunk->is_dirty = true;

    glGenVertexArrays (Chunk_Mesh_Count, chunk->opengl_is_stupid_vaos);
    glGenBuffers (Chunk_Mesh_Count, chunk->gl_vbos);

    for_range (i, 0, Chunk_Mesh_Count)
    {
        glBindVertexArray (chunk->opengl_is_stupid_vaos[i]);
        glBindBuffer (GL_ARRAY_BUFFER, chunk->gl_vbos[i]);

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
}

void chunk_cleanup (Chunk *chunk)
{
    glDeleteVertexArrays (Chunk_Mesh_Count, chunk->opengl_is_stupid_vaos);
    glDeleteBuffers (Chunk_Mesh_Count, chunk->gl_vbos);
}

Vec2i chunk_absolute_to_relative_coordinates (Chunk *chunk, s64 x, s64 z)
{
    return {cast (int) (x - chunk->x * Chunk_Size) - (x < 0), cast (int) (z - chunk->z * Chunk_Size) - (z < 0)};
}

Vec2l chunk_position_from_block_position (s64 x, s64 z)
{
    return {x / Chunk_Size - (x < 0), z / Chunk_Size - (z < 0)};
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

Terrain_Values chunk_get_terrain_values (Chunk *chunk, s64 x, s64 z)
{
    assert (x >= 0 && x < Chunk_Size && z >= 0 && z < Chunk_Size, "Bounds check failed (%lld, %lld)", x, z);

    return chunk->terrain_values[x * Chunk_Size + z];
}

void chunk_generate_cubiome (World *world, Chunk *chunk)
{
    for_range (x, 0, Chunk_Size)
    {
        for_range (z, 0, Chunk_Size)
        {
            int sample_x = x + chunk->x * Chunk_Size;
            int sample_z = z + chunk->z * Chunk_Size;

            auto values = &chunk->terrain_values[x * Chunk_Size + z];
            s64 np[6];
            cubiome::sampleBiomeNoise (&world->cubiome_gen.bn, np, sample_x, 0, sample_z, null, 0);
            values->surface_level = 64 + np[cubiome::NP_DEPTH] / 76.0;
        }
    }
}

void chunk_generate_mine (World *world, Chunk *chunk)
{
    f32 max_amplitude[3];
    for_range (i, 0, 3)
        max_amplitude[i] = perlin_fractal_max (world->terrain_params.noise[i].octaves, world->terrain_params.noise[i].persistance);

    for_range (x, 0, Chunk_Size)
    {
        for_range (z, 0, Chunk_Size)
        {
            int sample_x = x + chunk->x * Chunk_Size;
            int sample_z = z + chunk->z * Chunk_Size;

            auto values = &chunk->terrain_values[x * Chunk_Size + z];
            values->noise[0] = perlin_fractal_noise (world->terrain_params.noise[0], world->noise_offsets[0], sample_x, sample_z);
            values->noise[1] = perlin_fractal_noise (world->terrain_params.noise[1], world->noise_offsets[1], sample_x, sample_z);
            values->noise[2] = perlin_fractal_noise (world->terrain_params.noise[2], world->noise_offsets[2], sample_x, sample_z);

            values->noise[0] = inverse_lerp (-max_amplitude[0], max_amplitude[0], values->noise[0]);
            values->noise[1] = inverse_lerp (-max_amplitude[1], max_amplitude[1], values->noise[1]);
            values->noise[2] = inverse_lerp (-max_amplitude[2], max_amplitude[2], values->noise[2]);

            values->surface_level = hermite_cubic_calculate (world->terrain_params.spline, slice_make (array_size (values->noise), values->noise));
            values->surface_level = lerp (cast (f32) world->terrain_params.height_range.x, cast (f32) world->terrain_params.height_range.y, values->surface_level);
        }
    }
}

void chunk_generate (World *world, Chunk *chunk)
{
    if (chunk->generated)
        return;

    defer (chunk->generated = true);

    chunk_generate_cubiome (world, chunk);
    // chunk_generate_mine (world, chunk);

    for_range (i, 0, Chunk_Size)
    {
        for_range (j, 0, Chunk_Height)
        {
            for_range (k, 0, Chunk_Size)
            {
                s64 index = chunk_block_index (i, j, k);

                f32 surface_level = chunk_get_terrain_values (chunk, i, k).surface_level;

                if (j == 0)
                {
                    chunk->blocks[index].type = Block_Type_Bedrock;
                }
                else if (j > surface_level)
                {
                    if (j <= world->terrain_params.water_level)
                        chunk->blocks[index].type = Block_Type_Water;
                    else
                        chunk->blocks[index].type = Block_Type_Air;
                }
                else if (j > surface_level - Surface_Dirt_Height)
                {
                    chunk->blocks[index].type = Block_Type_Dirt;
                }
                else
                {
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

inline
bool block_is_of_mesh_type (Block_Type block, Chunk_Mesh_Type mesh_type)
{
    switch (mesh_type)
    {
    case Chunk_Mesh_Solid:
        return block != Block_Type_Water && block != Block_Type_Air;
    case Chunk_Mesh_Water:
        return block == Block_Type_Water;
    default:
        return false;
    }
}

void chunk_generate_mesh_data (Chunk *chunk, Array<Vertex> *vertices, Chunk_Mesh_Type type)
{
    if (!chunk->is_dirty)
       return;

    Vec3f position = {cast (f32) chunk->x * Chunk_Size, 0, cast (f32) chunk->z * Chunk_Size};
    for_range (x, 0, Chunk_Size)
    {
        for_range (y, 0, Chunk_Height)
        {
            for_range (z, 0, Chunk_Size)
            {
                auto block = chunk_get_block_in_chunk (chunk, x, y, z);
                if (!block_is_of_mesh_type (block.type, type))
                    continue;

                Block_Face_Flags visible_faces = 0;
                if (!block_is_of_mesh_type (chunk_get_block (chunk, x + 1, y, z).type, type))
                    visible_faces |= Block_Face_Flag_East;
                if (!block_is_of_mesh_type (chunk_get_block (chunk, x - 1, y, z).type, type))
                    visible_faces |= Block_Face_Flag_West;
                if (!block_is_of_mesh_type (chunk_get_block (chunk, x, y + 1, z).type, type))
                    visible_faces |= Block_Face_Flag_Above;
                if (!block_is_of_mesh_type (chunk_get_block (chunk, x, y - 1, z).type, type))
                    visible_faces |= Block_Face_Flag_Below;
                if (!block_is_of_mesh_type (chunk_get_block (chunk, x, y, z + 1).type, type))
                    visible_faces |= Block_Face_Flag_North;
                if (!block_is_of_mesh_type (chunk_get_block (chunk, x, y, z - 1).type, type))
                    visible_faces |= Block_Face_Flag_South;

                push_block (vertices, cast (u8) block.type, position + Vec3f{cast (f32) x, cast (f32) y, cast (f32) z}, visible_faces);
            }
        }
    }
}

void chunk_generate_mesh_data (Chunk *chunk)
{
    if (!chunk->is_dirty)
        return;
    defer (chunk->is_dirty = false);

    auto state = arena_get_state (&frame_arena);
    defer (arena_set_state (&frame_arena, state));

    Array<Vertex> vertices;
    array_init (&vertices, frame_allocator, 12000);

    chunk->total_vertex_count = 0;
    for_range (i, 0, Chunk_Mesh_Count)
    {
        chunk_generate_mesh_data (chunk, &vertices, cast (Chunk_Mesh_Type) i);
        chunk->vertex_counts[i] = vertices.count;
        chunk->total_vertex_count += vertices.count;

        glBindVertexArray (chunk->opengl_is_stupid_vaos[i]);
        glBindBuffer (GL_ARRAY_BUFFER, chunk->gl_vbos[i]);

        glBufferData (GL_ARRAY_BUFFER, sizeof (Vertex) * vertices.count, vertices.data, GL_DYNAMIC_DRAW);

        glBindVertexArray (0);
        glBindBuffer (GL_ARRAY_BUFFER, 0);

        array_clear (&vertices);
    }
}

u32 hash_vec2i (const Vec2i &v)
{
    return hash_combine (hash_s32 (v.x), hash_s32 (v.y));
}

bool compare_vec2i (const Vec2i &a, const Vec2i &b)
{
    return a == b;
}

void spline_push_value (Nested_Hermite_Spline *spline, f32 derivative, f32 x, Nested_Hermite_Spline *value)
{
    auto knot = array_push (&spline->knots);
    knot->is_nested_spline = true;
    knot->x = x;
    knot->spline = value;
    knot->derivative = derivative;
}

void spline_push_value (Nested_Hermite_Spline *spline, f32 derivative, f32 x, f32 value)
{
    auto knot = array_push (&spline->knots);
    knot->is_nested_spline = false;
    knot->x = x;
    knot->y = value;
    knot->derivative = derivative;
}

Nested_Hermite_Spline *spline_push (Terrain_Params *params, int t_value_index)
{
    auto spline = array_push (&params->spline_stack);
    spline->t_value_index = t_value_index;

    return spline;
}

void init_default_spline (Terrain_Params *params)
{
    auto spline = spline_push (params, 0);
    params->spline = spline;

    spline_push_value (spline, 0, 0, 0.5);
    spline_push_value (spline, 0, 1, 0.5);
}

void world_init (World *world, s32 seed, int chunks_to_pre_generate, Terrain_Params terrain_params)
{
    memset (world, 0, sizeof (World));

    world->seed = seed;
    world->terrain_params = terrain_params;
    if (!terrain_params.spline)
        init_default_spline (&world->terrain_params);
    cubiome::setupGenerator (&world->cubiome_gen, cubiome::MC_1_20, 0);
    cubiome::applySeed (&world->cubiome_gen, cubiome::DIM_OVERWORLD, seed);

    LC_RNG rng;
    random_seed (&rng, seed);
    random_seed (&rng, random_get_s32 (&rng));

    perlin_generate_offsets (&rng, world->terrain_params.noise[0].octaves, world->noise_offsets[0]);
    perlin_generate_offsets (&rng, world->terrain_params.noise[1].octaves, world->noise_offsets[1]);
    perlin_generate_offsets (&rng, world->terrain_params.noise[2].octaves, world->noise_offsets[2]);

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

Chunk *world_get_chunk_at_block_position (World *world, s64 x, s64 z)
{
    return world_get_chunk (world, x / Chunk_Size - (x < 0), z / Chunk_Size - (z < 0));
}

Chunk *world_create_chunk (World *world, s64 x, s64 z)
{
    auto chunk = world_get_chunk (world, x, z);
    if (chunk)
        return chunk;

    chunk = mem_alloc_uninit (Chunk, 1, heap_allocator ());
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

Block world_get_block (World *world, s64 x, s64 y, s64 z)
{
    s64 chunk_x = x / Chunk_Size;
    s64 chunk_z = z / Chunk_Size;
    auto chunk = world_get_chunk (world, chunk_x, chunk_z);
    if (!chunk)
        return Block_Air;

    auto rel_xz = chunk_absolute_to_relative_coordinates (chunk, x, z);

    return chunk_get_block_in_chunk (chunk, rel_xz.x, y, rel_xz.y);
}
