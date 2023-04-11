#include "Minecraft.hpp"

static bool g_show_demo_window;
static bool g_show_metrics_window;
static bool g_show_perlin_test_window;
static bool g_show_texture_atlas_window;
static bool g_show_world_window;

void generate_terrain_value_texture (GLuint *tex, int x, int z, int size, Terrain_Value terrain_value)
{
    int texture_size = size * Chunk_Size;
    u32 *texture_buffer = mem_alloc_uninit (u32, texture_size * texture_size, frame_allocator);

    for_range (i, 0, size)
    {
        for_range (j, 0, size)
        {
            s64 tex_x = i * Chunk_Size;
            s64 tex_y = j * Chunk_Size;

            auto chunk = world_get_chunk (&g_world, x + i - size / 2, z + j - size / 2);
            if (!chunk)
            {
                for_range (cx, 0, Chunk_Size)
                {
                    for_range (cz, 0, Chunk_Size)
                    {
                        texture_buffer[(tex_y + cz) * texture_size + tex_x + cx] = 0;
                    }
                }

                continue;
            }

            for_range (cx, 0, Chunk_Size)
            {
                for_range (cz, 0, Chunk_Size)
                {
                    auto val = chunk->terrain_values[cx * Chunk_Size + cz].array[terrain_value];

                    u8 color_comp = cast (u8) (val * 255);
                    texture_buffer[(tex_y + cz) * texture_size + tex_x + cx] = (0xff << 24) | (color_comp << 16) | (color_comp << 8) | (color_comp << 0);
                }
            }
        }
    }

    if (!*tex)
        glGenTextures (1, tex);

    glBindTexture (GL_TEXTURE_2D, *tex);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, texture_size, texture_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_buffer);
    glBindTexture (GL_TEXTURE_2D, 0);
}

void generate_noise_texture (GLuint *tex,
    int width, int height, f64 offset_x, f64 offset_y,
    int seed, f64 scale, int octaves, f64 persistance, f64 lacunarity)
{
    Vec2f offsets[6];

    octaves = min (octaves, 6);

    LC_RNG rng;
    random_seed (&rng, seed);

    for_range (i, 0, octaves)
    {
        offsets[i].x = random_rangef (&rng, -10000, 10000);
        offsets[i].y = random_rangef (&rng, -10000, 10000);
    }

    u32 *texture_buffer = mem_alloc_uninit (u32, width * height, frame_allocator);

    auto max_val = perlin_fractal_max (octaves, persistance);
    for_range (x, 0, width)
    {
        for_range (y, 0, height)
        {
            auto val = perlin_fractal_noise (scale, octaves, offsets, persistance, lacunarity, x + offset_x, y + offset_y);
            val = inverse_lerp (-max_val, max_val, val);

            u8 color_comp = cast (u8) (val * 255);
            texture_buffer[y * height + x] = (0xff << 24) | (color_comp << 16) | (color_comp << 8) | (color_comp << 0);
        }
    }

    if (!*tex)
        glGenTextures (1, tex);

    glBindTexture (GL_TEXTURE_2D, *tex);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_buffer);
    glBindTexture (GL_TEXTURE_2D, 0);
}

void ui_show_perlin_test_window (bool *opened)
{
    static const int Max_Octaves = 6;

    static f32 scale = 0.05;
    static GLuint texture_handle;
    static int texture_size = 256;
    static int seed;
    static f32 offset_x, offset_y;
    static int octaves = 3;
    static f32 lacunarity = 1;
    static f32 persistance = 0.5;

    if (ImGui::Begin ("Perlin Test", opened))
    {
        {
            int lines = 7;
            auto child_height = ImGui::GetContentRegionAvail ().y - lines * ImGui::GetFrameHeightWithSpacing ();
            if (ImGui::BeginChild ("Image", {0, child_height}, true, ImGuiWindowFlags_HorizontalScrollbar))
            {
                if (texture_handle)
                    ImGui::Image (cast (ImTextureID) texture_handle, {cast (f32) texture_size, cast (f32) texture_size});
            }
            ImGui::EndChild ();
        }

        bool should_generate = false;

        if (!texture_handle)
            should_generate = true;

        if (ImGui::SliderInt ("Size", &texture_size, 128, 4096))
            should_generate = true;
        if (ImGui::SliderFloat ("Scale", &scale, 0.001, 0.2))
            should_generate = true;
        if (ImGui::SliderInt ("Octaves", &octaves, 1, Max_Octaves))
            should_generate = true;
        if (ImGui::SliderFloat ("Persistance", &persistance, 0.001, 1))
            should_generate = true;
        if (ImGui::SliderFloat ("Lacunarity", &lacunarity, 1, 10))
            should_generate = true;

        if (ImGui::Button ("Generate"))
            should_generate = true;

        ImGui::SameLine ();

        if (ImGui::Button ("Randomize") || !texture_handle)
        {
            seed = random_get_s32 ();

            LC_RNG rng;
            random_seed (&rng, seed);
            offset_x = random_rangef (&rng, -10000, 10000);
            offset_y = random_rangef (&rng, -10000, 10000);
            should_generate = true;
        }

        if (should_generate)
        {
            generate_noise_texture (&texture_handle, texture_size, texture_size, offset_x, offset_y, seed, scale, octaves, persistance, lacunarity);
        }
    }
    ImGui::End ();
}

void ui_show_metrics_and_settings_window (bool *opened)
{
    if (ImGui::Begin ("Metrics and Settings", opened))
    {
        s64 total_vertex_count = 0;
        for_hash_map (it, g_world.all_loaded_chunks)
        {
            auto chunk = *it.value;
            if (chunk)
                total_vertex_count += chunk->vertex_count;
        }

        ImGui::Text ("Frame time: %.2f ms, %.2f FPS", g_delta_time / 1000.0, 1000000.0 / g_delta_time);
        ImGui::Text ("Position: %.2f %.2f %.2f", g_camera.position.x, g_camera.position.y, g_camera.position.z);
        ImGui::Text ("Average chunk creation   time: %f us", g_chunk_creation_time / cast (f32) g_chunk_creation_samples);
        ImGui::Text ("Average chunk generation time: %f us", g_chunk_generation_time / cast (f32) g_chunk_generation_samples);
        ImGui::Text ("Loaded chunks: %lld", g_world.all_loaded_chunks.count);
        ImGui::Text ("Total vertex count: %lld", total_vertex_count);
        ImGui::Text ("Drawn vertex count: %lld", g_drawn_vertex_count);
        ImGui::Text ("Average vertices per chunk: %lld", total_vertex_count / g_world.all_loaded_chunks.count);
        ImGui::Checkbox ("Generate new chunks", &g_generate_new_chunks);
        ImGui::SliderInt ("Render distance", &g_render_distance, 1, 12);
    }
    ImGui::End ();
}

void ui_show_texture_atlas_window (bool *opened)
{
    if (ImGui::Begin ("Texture Atlas", opened))
    {
        auto avail_width = ImGui::GetContentRegionAvail ().x;
        ImGui::Image (cast (ImTextureID) g_texture_atlas, {cast (f32) avail_width, cast (f32) avail_width});
    }
    ImGui::End ();
}

void ui_show_basic_world_settings ()
{
    static int new_seed = -758789230;

    ImGui::Text ("Seed: %d", g_world.seed);


    if (ImGui::Button ("Generate New"))
    {
        new_seed = random_get_s32 ();
        world_clear_chunks (&g_world);
        world_init (&g_world, new_seed);
    }

    ImGui::InputInt ("New Seed", &new_seed, 0);

    if (ImGui::Button ("Regenerate"))
    {
        world_clear_chunks (&g_world);
        world_init (&g_world, new_seed);
    }
}

void ui_show_terrain_noise_maps ()
{
    static GLuint continentalness_tex;
    static GLuint erosion_tex;
    static GLuint peaks_and_valleys_tex;

    static const f32 Scale = 2;

    static int size = 8;

    int lines = 2;
    auto child_height = ImGui::GetContentRegionAvail ().y - lines * ImGui::GetFrameHeightWithSpacing ();
    if (ImGui::BeginChild ("Noise Maps", {0, child_height}, true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        int column_count = clamp (cast (int) (ImGui::GetContentRegionAvail ().x / (size * Chunk_Size * Scale)), 1, 3);
        ImGui::Columns (column_count, 0, false);

        ImGui::Text ("Continentalness");
        ImGui::Image (cast (ImTextureID) continentalness_tex, {size * Chunk_Size * Scale, size * Chunk_Size * Scale});
        ImGui::NextColumn ();

        ImGui::Text ("Erosion");
        ImGui::Image (cast (ImTextureID) erosion_tex, {size * Chunk_Size * Scale, size * Chunk_Size * Scale});
        ImGui::NextColumn ();

        ImGui::Text ("Peaks And Valleys");
        ImGui::Image (cast (ImTextureID) peaks_and_valleys_tex, {size * Chunk_Size * Scale, size * Chunk_Size * Scale});
        ImGui::NextColumn ();
    }
    ImGui::EndChild ();

    bool generate = false;

    if (!continentalness_tex || !erosion_tex || !peaks_and_valleys_tex)
        generate = true;

    if (ImGui::SliderInt ("Size", &size, 1, 100))
        generate = true;

    if (ImGui::Button ("Generate"))
        generate = true;

    if (generate)
    {
        generate_terrain_value_texture (&continentalness_tex,
            cast (int) (g_camera.position.x / Chunk_Size), cast (int) (g_camera.position.z / Chunk_Size),
            size, Terrain_Value_Continentalness);

        generate_terrain_value_texture (&erosion_tex,
            cast (int) (g_camera.position.x / Chunk_Size), cast (int) (g_camera.position.z / Chunk_Size),
            size, Terrain_Value_Erosion);

        generate_terrain_value_texture (&peaks_and_valleys_tex,
            cast (int) (g_camera.position.x / Chunk_Size), cast (int) (g_camera.position.z / Chunk_Size),
            size, Terrain_Value_Peaks_And_Valleys);
    }
}

void ui_show_world_window (bool *opened)
{
    if (ImGui::Begin ("World", opened))
    {
        if (ImGui::BeginTabBar ("World Tabs"))
        {
            if (ImGui::BeginTabItem ("Basic Settings"))
            {
                ui_show_basic_world_settings ();
                ImGui::EndTabItem ();
            }

            if (ImGui::BeginTabItem ("Terrain Noise Maps"))
            {
                ui_show_terrain_noise_maps ();
                ImGui::EndTabItem ();
            }

            ImGui::EndTabBar ();
        }
    }
    ImGui::End ();
}

void ui_show_windows ()
{
    if (ImGui::BeginMainMenuBar ())
    {
        if (ImGui::Button ("Demo Window"))
            g_show_demo_window = true;
        if (ImGui::Button ("Metrics and Settings Window"))
            g_show_metrics_window = true;
        if (ImGui::Button ("Perlin Test Window"))
            g_show_perlin_test_window = true;
        if (ImGui::Button ("Texture Atlas Window"))
            g_show_texture_atlas_window = true;
        if (ImGui::Button ("World Window"))
            g_show_world_window = true;

        ImGui::EndMainMenuBar ();
    }

    if (g_show_demo_window)
        ImGui::ShowDemoWindow (&g_show_demo_window);

    if (g_show_metrics_window)
        ui_show_metrics_and_settings_window (&g_show_metrics_window);

    if (g_show_perlin_test_window)
        ui_show_perlin_test_window (&g_show_perlin_test_window);

    if (g_show_texture_atlas_window)
        ui_show_texture_atlas_window (&g_show_texture_atlas_window);

    if (g_show_world_window)
        ui_show_world_window (&g_show_world_window);
}
