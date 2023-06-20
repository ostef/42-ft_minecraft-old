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

            values->noise[0] = cast (float) np[cubiome::NP_CONTINENTALNESS] / 10000.0f;
            values->noise[1] = cast (float) np[cubiome::NP_EROSION] / 10000.0f;
            values->noise[2] = cast (float) np[cubiome::NP_WEIRDNESS] / 10000.0f;
            values->noise[3] = -3.0f * (fabsf (fabsf (values->noise[2]) - 0.6666667f) - 0.33333334f);

            values->noise[0] = inverse_lerp (-1.0f, 1.0f, values->noise[0]);
            values->noise[1] = inverse_lerp (-1.0f, 1.0f, values->noise[1]);
            values->noise[2] = inverse_lerp (-1.0f, 1.0f, values->noise[2]);
            values->noise[3] = inverse_lerp (-1.0f, 1.0f, values->noise[3]);

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

            values->noise[3] = values->noise[2] * 2 - 1;
            values->noise[3] = -3.0f * (fabsf (fabsf (values->noise[3]) - 0.6666667f) - 0.33333334f);
            values->noise[3] = inverse_lerp (-1.0f, 1.0f, values->noise[3]);

            values->surface_level = hermite_cubic_calculate (world->terrain_params.surface_spline, slice_make (array_size (values->noise), values->noise));
            values->surface_level = lerp (cast (f32) world->terrain_params.height_range.x, cast (f32) world->terrain_params.height_range.y, values->surface_level);
        }
    }
}

void chunk_generate (World *world, Chunk *chunk)
{
    if (chunk->generated)
        return;

    defer (chunk->generated = true);

    // chunk_generate_cubiome (world, chunk);
    chunk_generate_mine (world, chunk);

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
    auto spline_8856 = spline_push (params, 0);
    params->surface_spline = spline_8856;
    spline_push_value (spline_8856, 0.000000, 0.000000, 0.191111);
    spline_push_value (spline_8856, 0.000000, 0.038095, 0.043222);
    spline_push_value (spline_8856, 0.000000, 0.280952, 0.043222);
    spline_push_value (spline_8856, 0.000000, 0.314286, 0.100000);
    spline_push_value (spline_8856, 0.000000, 0.438095, 0.100000);
    auto spline_9512 = spline_push (params, 1);
    auto spline_10168 = spline_push (params, 3);
    spline_push_value (spline_10168, 0.389401, 0.000000, 0.117332);
    spline_push_value (spline_10168, 0.389401, 1.000000, 0.550000);
    spline_push_value (spline_9512, 0.000000, 0.000000, spline_10168);
    auto spline_10824 = spline_push (params, 3);
    spline_push_value (spline_10824, 0.377880, 0.000000, 0.102355);
    spline_push_value (spline_10824, 0.377880, 1.000000, 0.522222);
    spline_push_value (spline_9512, 0.000000, 0.096774, spline_10824);
    auto spline_11480 = spline_push (params, 3);
    spline_push_value (spline_11480, 0.000000, 0.000000, 0.043222);
    spline_push_value (spline_11480, 0.000000, 0.125000, 0.043222);
    spline_push_value (spline_11480, 0.000000, 0.175000, 0.166667);
    spline_push_value (spline_11480, 0.000000, 0.797727, 0.166667);
    spline_push_value (spline_11480, 0.253456, 0.802727, 0.166667);
    spline_push_value (spline_11480, 0.253456, 1.000000, 0.222222);
    spline_push_value (spline_9512, 0.000000, 0.290323, spline_11480);
    auto spline_12136 = spline_push (params, 3);
    spline_push_value (spline_12136, 0.500000, 0.000000, 0.000000);
    spline_push_value (spline_12136, 0.000000, 0.300000, 0.194444);
    spline_push_value (spline_12136, 0.000000, 0.500000, 0.194444);
    spline_push_value (spline_12136, 0.000000, 0.700000, 0.194444);
    spline_push_value (spline_12136, 0.007000, 1.000000, 0.200000);
    spline_push_value (spline_9512, 0.000000, 0.322581, spline_12136);
    auto spline_12792 = spline_push (params, 3);
    spline_push_value (spline_12792, 0.500000, 0.000000, 0.083333);
    spline_push_value (spline_12792, 0.000000, 0.300000, 0.166667);
    spline_push_value (spline_12792, 0.000000, 0.500000, 0.166667);
    spline_push_value (spline_12792, 0.100000, 0.700000, 0.194444);
    spline_push_value (spline_12792, 0.007000, 1.000000, 0.200000);
    spline_push_value (spline_9512, 0.000000, 0.483871, spline_12792);
    auto spline_13448 = spline_push (params, 3);
    spline_push_value (spline_13448, 0.500000, 0.000000, 0.083333);
    spline_push_value (spline_13448, 0.000000, 0.300000, 0.166667);
    spline_push_value (spline_13448, 0.000000, 0.500000, 0.166667);
    spline_push_value (spline_13448, 0.000000, 0.700000, 0.166667);
    spline_push_value (spline_13448, 0.000000, 1.000000, 0.166667);
    spline_push_value (spline_9512, 0.000000, 0.677419, spline_13448);
    auto spline_14104 = spline_push (params, 3);
    spline_push_value (spline_14104, 0.000000, 0.000000, 0.155556);
    spline_push_value (spline_14104, 0.000000, 0.300000, 0.150000);
    spline_push_value (spline_14104, 0.000000, 0.500000, 0.150000);
    spline_push_value (spline_14104, 0.060000, 0.700000, 0.166667);
    spline_push_value (spline_14104, 0.000000, 1.000000, 0.166667);
    spline_push_value (spline_9512, 0.000000, 1.000000, spline_14104);
    spline_push_value (spline_8856, 0.000000, 0.447619, spline_9512);
    auto spline_14760 = spline_push (params, 1);
    auto spline_15416 = spline_push (params, 3);
    spline_push_value (spline_15416, 0.389401, 0.000000, 0.117332);
    spline_push_value (spline_15416, 0.389401, 1.000000, 0.550000);
    spline_push_value (spline_14760, 0.000000, 0.000000, spline_15416);
    auto spline_16072 = spline_push (params, 3);
    spline_push_value (spline_16072, 0.377880, 0.000000, 0.102355);
    spline_push_value (spline_16072, 0.377880, 1.000000, 0.522222);
    spline_push_value (spline_14760, 0.000000, 0.096774, spline_16072);
    auto spline_16728 = spline_push (params, 3);
    spline_push_value (spline_16728, 0.000000, 0.000000, 0.043222);
    spline_push_value (spline_16728, 0.000000, 0.125000, 0.043222);
    spline_push_value (spline_16728, 0.000000, 0.175000, 0.166667);
    spline_push_value (spline_16728, 0.000000, 0.797727, 0.166667);
    spline_push_value (spline_16728, 0.253456, 0.802727, 0.166667);
    spline_push_value (spline_16728, 0.253456, 1.000000, 0.222222);
    spline_push_value (spline_14760, 0.000000, 0.290323, spline_16728);
    auto spline_17384 = spline_push (params, 3);
    spline_push_value (spline_17384, 0.500000, 0.000000, 0.000000);
    spline_push_value (spline_17384, 0.000000, 0.300000, 0.194444);
    spline_push_value (spline_17384, 0.000000, 0.500000, 0.194444);
    spline_push_value (spline_17384, 0.000000, 0.700000, 0.194444);
    spline_push_value (spline_17384, 0.007000, 1.000000, 0.200000);
    spline_push_value (spline_14760, 0.000000, 0.322581, spline_17384);
    auto spline_18040 = spline_push (params, 3);
    spline_push_value (spline_18040, 0.500000, 0.000000, 0.083333);
    spline_push_value (spline_18040, 0.000000, 0.300000, 0.166667);
    spline_push_value (spline_18040, 0.000000, 0.500000, 0.166667);
    spline_push_value (spline_18040, 0.100000, 0.700000, 0.194444);
    spline_push_value (spline_18040, 0.007000, 1.000000, 0.200000);
    spline_push_value (spline_14760, 0.000000, 0.483871, spline_18040);
    auto spline_18696 = spline_push (params, 3);
    spline_push_value (spline_18696, 0.500000, 0.000000, 0.083333);
    spline_push_value (spline_18696, 0.000000, 0.300000, 0.166667);
    spline_push_value (spline_18696, 0.000000, 0.500000, 0.166667);
    spline_push_value (spline_18696, 0.000000, 0.700000, 0.166667);
    spline_push_value (spline_18696, 0.000000, 1.000000, 0.166667);
    spline_push_value (spline_14760, 0.000000, 0.677419, spline_18696);
    auto spline_19352 = spline_push (params, 3);
    spline_push_value (spline_19352, 0.000000, 0.000000, 0.155556);
    spline_push_value (spline_19352, 0.000000, 0.300000, 0.150000);
    spline_push_value (spline_19352, 0.000000, 0.500000, 0.150000);
    spline_push_value (spline_19352, 0.060000, 0.700000, 0.166667);
    spline_push_value (spline_19352, 0.000000, 1.000000, 0.166667);
    spline_push_value (spline_14760, 0.000000, 1.000000, spline_19352);
    spline_push_value (spline_8856, 0.000000, 0.452381, spline_14760);
    auto spline_20008 = spline_push (params, 1);
    auto spline_20664 = spline_push (params, 3);
    spline_push_value (spline_20664, 0.389401, 0.000000, 0.117332);
    spline_push_value (spline_20664, 0.389401, 1.000000, 0.550000);
    spline_push_value (spline_20008, 0.000000, 0.000000, spline_20664);
    auto spline_21320 = spline_push (params, 3);
    spline_push_value (spline_21320, 0.377880, 0.000000, 0.102355);
    spline_push_value (spline_21320, 0.377880, 1.000000, 0.522222);
    spline_push_value (spline_20008, 0.000000, 0.096774, spline_21320);
    auto spline_21976 = spline_push (params, 3);
    spline_push_value (spline_21976, 0.000000, 0.000000, 0.043222);
    spline_push_value (spline_21976, 0.000000, 0.125000, 0.043222);
    spline_push_value (spline_21976, 0.000000, 0.175000, 0.166667);
    spline_push_value (spline_21976, 0.000000, 0.797727, 0.166667);
    spline_push_value (spline_21976, 0.253456, 0.802727, 0.166667);
    spline_push_value (spline_21976, 0.253456, 1.000000, 0.222222);
    spline_push_value (spline_20008, 0.000000, 0.290323, spline_21976);
    auto spline_22632 = spline_push (params, 3);
    spline_push_value (spline_22632, 0.500000, 0.000000, 0.027778);
    spline_push_value (spline_22632, 0.000000, 0.300000, 0.194444);
    spline_push_value (spline_22632, 0.000000, 0.500000, 0.194444);
    spline_push_value (spline_22632, 0.000000, 0.700000, 0.194444);
    spline_push_value (spline_22632, 0.007000, 1.000000, 0.200000);
    spline_push_value (spline_20008, 0.000000, 0.322581, spline_22632);
    auto spline_23288 = spline_push (params, 3);
    spline_push_value (spline_23288, 0.500000, 0.000000, 0.111111);
    spline_push_value (spline_23288, 0.010000, 0.300000, 0.167222);
    spline_push_value (spline_23288, 0.010000, 0.500000, 0.168333);
    spline_push_value (spline_23288, 0.094000, 0.700000, 0.194444);
    spline_push_value (spline_23288, 0.007000, 1.000000, 0.200000);
    spline_push_value (spline_20008, 0.000000, 0.483871, spline_23288);
    auto spline_23944 = spline_push (params, 3);
    spline_push_value (spline_23944, 0.500000, 0.000000, 0.111111);
    spline_push_value (spline_23944, 0.000000, 0.300000, 0.172222);
    spline_push_value (spline_23944, 0.000000, 0.500000, 0.172222);
    spline_push_value (spline_23944, 0.040000, 0.700000, 0.183333);
    spline_push_value (spline_23944, 0.049000, 1.000000, 0.222222);
    spline_push_value (spline_20008, 0.000000, 0.677419, spline_23944);
    auto spline_24600 = spline_push (params, 3);
    spline_push_value (spline_24600, 0.000000, 0.000000, 0.155556);
    spline_push_value (spline_24600, 0.000000, 0.300000, 0.150000);
    spline_push_value (spline_24600, 0.000000, 0.500000, 0.150000);
    spline_push_value (spline_24600, 0.120000, 0.700000, 0.183333);
    spline_push_value (spline_24600, 0.049000, 1.000000, 0.222222);
    spline_push_value (spline_20008, 0.000000, 1.000000, spline_24600);
    spline_push_value (spline_8856, 0.000000, 0.476191, spline_20008);
    auto spline_25256 = spline_push (params, 1);
    auto spline_25912 = spline_push (params, 3);
    spline_push_value (spline_25912, 0.000000, 0.000000, 0.279083);
    spline_push_value (spline_25912, 0.513825, 0.500000, 0.564542);
    spline_push_value (spline_25912, 0.513825, 1.000000, 0.850000);
    spline_push_value (spline_25256, 0.000000, 0.000000, spline_25912);
    auto spline_26568 = spline_push (params, 3);
    spline_push_value (spline_26568, 0.000000, 0.000000, 0.277778);
    spline_push_value (spline_26568, 0.433180, 0.500000, 0.414900);
    spline_push_value (spline_26568, 0.433180, 1.000000, 0.655556);
    spline_push_value (spline_25256, 0.000000, 0.096774, spline_26568);
    auto spline_27224 = spline_push (params, 3);
    spline_push_value (spline_27224, 0.000000, 0.000000, 0.277778);
    spline_push_value (spline_27224, 0.391705, 0.500000, 0.337942);
    spline_push_value (spline_27224, 0.391705, 1.000000, 0.555556);
    spline_push_value (spline_25256, 0.000000, 0.290323, spline_27224);
    auto spline_27880 = spline_push (params, 3);
    spline_push_value (spline_27880, 0.500000, 0.000000, 0.027778);
    spline_push_value (spline_27880, 0.000000, 0.300000, 0.361111);
    spline_push_value (spline_27880, 0.000000, 0.500000, 0.361111);
    spline_push_value (spline_27880, 0.000000, 0.700000, 0.361111);
    spline_push_value (spline_27880, 0.049000, 1.000000, 0.400000);
    spline_push_value (spline_25256, 0.000000, 0.322581, spline_27880);
    auto spline_28536 = spline_push (params, 3);
    spline_push_value (spline_28536, 0.500000, 0.000000, 0.111111);
    spline_push_value (spline_28536, 0.070000, 0.300000, 0.170556);
    spline_push_value (spline_28536, 0.070000, 0.500000, 0.178333);
    spline_push_value (spline_28536, 0.658000, 0.700000, 0.361111);
    spline_push_value (spline_28536, 0.049000, 1.000000, 0.400000);
    spline_push_value (spline_25256, 0.000000, 0.483871, spline_28536);
    auto spline_29192 = spline_push (params, 3);
    spline_push_value (spline_29192, 0.500000, 0.000000, 0.111111);
    spline_push_value (spline_29192, 0.000000, 0.300000, 0.172222);
    spline_push_value (spline_29192, 0.000000, 0.500000, 0.172222);
    spline_push_value (spline_29192, 0.040000, 0.700000, 0.183333);
    spline_push_value (spline_29192, 0.049000, 1.000000, 0.222222);
    spline_push_value (spline_25256, 0.000000, 0.677419, spline_29192);
    auto spline_29848 = spline_push (params, 3);
    spline_push_value (spline_29848, 0.500000, 0.000000, 0.111111);
    spline_push_value (spline_29848, 0.000000, 0.300000, 0.172222);
    spline_push_value (spline_29848, 0.000000, 0.500000, 0.172222);
    spline_push_value (spline_29848, 0.040000, 0.700000, 0.183333);
    spline_push_value (spline_29848, 0.049000, 1.000000, 0.222222);
    spline_push_value (spline_25256, 0.000000, 0.806452, spline_29848);
    auto spline_30504 = spline_push (params, 3);
    spline_push_value (spline_30504, 0.000000, 0.000000, 0.111111);
    auto spline_31160 = spline_push (params, 3);
    spline_push_value (spline_31160, 0.500000, 0.000000, 0.111111);
    spline_push_value (spline_31160, 0.000000, 0.300000, 0.172222);
    spline_push_value (spline_31160, 0.000000, 0.500000, 0.172222);
    spline_push_value (spline_31160, 0.040000, 0.700000, 0.183333);
    spline_push_value (spline_31160, 0.049000, 1.000000, 0.222222);
    spline_push_value (spline_30504, 0.000000, 0.600000, spline_31160);
    spline_push_value (spline_30504, 0.000000, 1.000000, 0.261111);
    spline_push_value (spline_25256, 0.000000, 0.838710, spline_30504);
    auto spline_31816 = spline_push (params, 3);
    spline_push_value (spline_31816, 0.000000, 0.000000, 0.111111);
    auto spline_32472 = spline_push (params, 3);
    spline_push_value (spline_32472, 0.500000, 0.000000, 0.111111);
    spline_push_value (spline_32472, 0.000000, 0.300000, 0.172222);
    spline_push_value (spline_32472, 0.000000, 0.500000, 0.172222);
    spline_push_value (spline_32472, 0.040000, 0.700000, 0.183333);
    spline_push_value (spline_32472, 0.049000, 1.000000, 0.222222);
    spline_push_value (spline_31816, 0.000000, 0.600000, spline_32472);
    spline_push_value (spline_31816, 0.000000, 1.000000, 0.261111);
    spline_push_value (spline_25256, 0.000000, 0.903226, spline_31816);
    auto spline_33128 = spline_push (params, 3);
    spline_push_value (spline_33128, 0.500000, 0.000000, 0.111111);
    spline_push_value (spline_33128, 0.000000, 0.300000, 0.172222);
    spline_push_value (spline_33128, 0.000000, 0.500000, 0.172222);
    spline_push_value (spline_33128, 0.040000, 0.700000, 0.183333);
    spline_push_value (spline_33128, 0.049000, 1.000000, 0.222222);
    spline_push_value (spline_25256, 0.000000, 0.922581, spline_33128);
    auto spline_33784 = spline_push (params, 3);
    spline_push_value (spline_33784, 0.000000, 0.000000, 0.155556);
    spline_push_value (spline_33784, 0.000000, 0.300000, 0.150000);
    spline_push_value (spline_33784, 0.000000, 0.500000, 0.150000);
    spline_push_value (spline_33784, 0.120000, 0.700000, 0.183333);
    spline_push_value (spline_33784, 0.049000, 1.000000, 0.222222);
    spline_push_value (spline_25256, 0.000000, 1.000000, spline_33784);
    spline_push_value (spline_8856, 0.000000, 0.642857, spline_25256);
    auto spline_34440 = spline_push (params, 1);
    auto spline_35096 = spline_push (params, 3);
    spline_push_value (spline_35096, 0.000000, 0.000000, 0.359959);
    spline_push_value (spline_35096, 0.576037, 0.500000, 0.679980);
    spline_push_value (spline_35096, 0.576037, 1.000000, 1.000000);
    spline_push_value (spline_34440, 0.000000, 0.000000, spline_35096);
    auto spline_35752 = spline_push (params, 3);
    spline_push_value (spline_35752, 0.000000, 0.000000, 0.277778);
    spline_push_value (spline_35752, 0.460829, 0.500000, 0.466206);
    spline_push_value (spline_35752, 0.460829, 1.000000, 0.722222);
    spline_push_value (spline_34440, 0.000000, 0.096774, spline_35752);
    auto spline_36408 = spline_push (params, 3);
    spline_push_value (spline_36408, 0.000000, 0.000000, 0.277778);
    spline_push_value (spline_36408, 0.460829, 0.500000, 0.466206);
    spline_push_value (spline_36408, 0.460829, 1.000000, 0.722222);
    spline_push_value (spline_34440, 0.000000, 0.290323, spline_36408);
    auto spline_37064 = spline_push (params, 3);
    spline_push_value (spline_37064, 0.500000, 0.000000, 0.055556);
    spline_push_value (spline_37064, 0.000000, 0.300000, 0.444444);
    spline_push_value (spline_37064, 0.000000, 0.500000, 0.444444);
    spline_push_value (spline_37064, 0.000000, 0.700000, 0.444444);
    spline_push_value (spline_37064, 0.070000, 1.000000, 0.500000);
    spline_push_value (spline_34440, 0.000000, 0.322581, spline_37064);
    auto spline_37720 = spline_push (params, 3);
    spline_push_value (spline_37720, 0.500000, 0.000000, 0.138889);
    spline_push_value (spline_37720, 0.100000, 0.300000, 0.172222);
    spline_push_value (spline_37720, 0.100000, 0.500000, 0.183333);
    spline_push_value (spline_37720, 0.940000, 0.700000, 0.444444);
    spline_push_value (spline_37720, 0.070000, 1.000000, 0.500000);
    spline_push_value (spline_34440, 0.000000, 0.483871, spline_37720);
    auto spline_38376 = spline_push (params, 3);
    spline_push_value (spline_38376, 0.500000, 0.000000, 0.138889);
    spline_push_value (spline_38376, 0.000000, 0.300000, 0.172222);
    spline_push_value (spline_38376, 0.000000, 0.500000, 0.172222);
    spline_push_value (spline_38376, 0.040000, 0.700000, 0.183333);
    spline_push_value (spline_38376, 0.049000, 1.000000, 0.222222);
    spline_push_value (spline_34440, 0.000000, 0.677419, spline_38376);
    auto spline_39032 = spline_push (params, 3);
    spline_push_value (spline_39032, 0.500000, 0.000000, 0.138889);
    spline_push_value (spline_39032, 0.000000, 0.300000, 0.172222);
    spline_push_value (spline_39032, 0.000000, 0.500000, 0.172222);
    spline_push_value (spline_39032, 0.040000, 0.700000, 0.183333);
    spline_push_value (spline_39032, 0.049000, 1.000000, 0.222222);
    spline_push_value (spline_34440, 0.000000, 0.806452, spline_39032);
    auto spline_39688 = spline_push (params, 3);
    spline_push_value (spline_39688, 0.000000, 0.000000, 0.138889);
    auto spline_40344 = spline_push (params, 3);
    spline_push_value (spline_40344, 0.500000, 0.000000, 0.138889);
    spline_push_value (spline_40344, 0.000000, 0.300000, 0.172222);
    spline_push_value (spline_40344, 0.000000, 0.500000, 0.172222);
    spline_push_value (spline_40344, 0.040000, 0.700000, 0.183333);
    spline_push_value (spline_40344, 0.049000, 1.000000, 0.222222);
    spline_push_value (spline_39688, 0.000000, 0.600000, spline_40344);
    spline_push_value (spline_39688, 0.000000, 1.000000, 0.261111);
    spline_push_value (spline_34440, 0.000000, 0.838710, spline_39688);
    auto spline_41000 = spline_push (params, 3);
    spline_push_value (spline_41000, 0.000000, 0.000000, 0.138889);
    auto spline_41656 = spline_push (params, 3);
    spline_push_value (spline_41656, 0.500000, 0.000000, 0.138889);
    spline_push_value (spline_41656, 0.000000, 0.300000, 0.172222);
    spline_push_value (spline_41656, 0.000000, 0.500000, 0.172222);
    spline_push_value (spline_41656, 0.040000, 0.700000, 0.183333);
    spline_push_value (spline_41656, 0.049000, 1.000000, 0.222222);
    spline_push_value (spline_41000, 0.000000, 0.600000, spline_41656);
    spline_push_value (spline_41000, 0.000000, 1.000000, 0.261111);
    spline_push_value (spline_34440, 0.000000, 0.903226, spline_41000);
    auto spline_42312 = spline_push (params, 3);
    spline_push_value (spline_42312, 0.500000, 0.000000, 0.138889);
    spline_push_value (spline_42312, 0.000000, 0.300000, 0.172222);
    spline_push_value (spline_42312, 0.000000, 0.500000, 0.172222);
    spline_push_value (spline_42312, 0.040000, 0.700000, 0.183333);
    spline_push_value (spline_42312, 0.049000, 1.000000, 0.222222);
    spline_push_value (spline_34440, 0.000000, 0.922581, spline_42312);
    auto spline_42968 = spline_push (params, 3);
    spline_push_value (spline_42968, 0.015000, 0.000000, 0.155556);
    spline_push_value (spline_42968, 0.000000, 0.300000, 0.172222);
    spline_push_value (spline_42968, 0.000000, 0.500000, 0.172222);
    spline_push_value (spline_42968, 0.040000, 0.700000, 0.183333);
    spline_push_value (spline_42968, 0.049000, 1.000000, 0.222222);
    spline_push_value (spline_34440, 0.000000, 1.000000, spline_42968);
    spline_push_value (spline_8856, 0.000000, 1.000000, spline_34440);
}

void world_init (World *world, s32 seed, int chunks_to_pre_generate, Terrain_Params terrain_params)
{
    memset (world, 0, sizeof (World));

    world->seed = seed;
    world->terrain_params = terrain_params;
    if (!terrain_params.surface_spline)
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
