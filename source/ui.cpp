#include "Minecraft.hpp"

static bool g_show_demo_window;
static bool g_show_metrics_window;
static bool g_show_perlin_test_window;
static bool g_show_texture_atlas_window;
static bool g_show_world_window;
static bool g_show_terrain_noise_maps_window;

void generate_terrain_value_texture (GLuint *tex, int x, int z, int size, Terrain_Value terrain_value, bool only_noise)
{
    static const ImVec4 Block_Color_Water = {0.2, 0.3, 0.6, 1.0};
    static const ImVec4 Block_Color_Dirt = {0.4, 0.4, 0.2, 1.0};

    int texture_size = size * Chunk_Size;
    u32 *texture_buffer = mem_alloc_uninit (u32, texture_size * texture_size, frame_allocator);

    for_range (i, 0, size)
    {
        for_range (j, 0, size)
        {
            s64 tex_x = i * Chunk_Size;
            s64 tex_y = (size - j - 1) * Chunk_Size;

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
                    if (terrain_value == Terrain_Value_Count)
                    {
                        ImVec4 color = Block_Color_Water;
                        f32 val = chunk->terrain_values[cx * Chunk_Size + cz].surface_level;
                        f32 normalized_val = val / cast (f32) Chunk_Height;

                        if (val > g_world.terrain_params.water_level)
                        {
                            color = ImVec4{normalized_val,normalized_val,normalized_val,1};
                        }

                        texture_buffer[(tex_y + Chunk_Size - cz - 1) * texture_size + tex_x + cx] = ImGui::ColorConvertFloat4ToU32 (color);
                    }
                    else
                    {
                        f32 val;

                        if (only_noise)
                            val = chunk->terrain_values[cx * Chunk_Size + cz].noise_values[terrain_value];
                        else
                            val = chunk->terrain_values[cx * Chunk_Size + cz].bezier_values[terrain_value];

                        u8 color_comp = cast (u8) (val * 255);
                        texture_buffer[(tex_y + Chunk_Size - cz - 1) * texture_size + tex_x + cx] = (0xff << 24) | (color_comp << 16) | (color_comp << 8) | (color_comp << 0);
                    }

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
    int seed, f64 scale, int octaves, f64 persistance, f64 lacunarity,
    f32 *out_min = null, f32 *out_max = null)
{
    Vec2f offsets[Perlin_Fractal_Max_Octaves];
    f32 min_non_normalized = F32_MAX;
    f32 max_non_normalized = -F32_MAX;

    octaves = clamp (octaves, 1, Perlin_Fractal_Max_Octaves);

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
            if (val < min_non_normalized)
                min_non_normalized = val;
            if (val > max_non_normalized)
                max_non_normalized = val;

            u8 color_comp = cast (u8) (val * 255);
            texture_buffer[y * height + x] = (0xff << 24) | (color_comp << 16) | (color_comp << 8) | (color_comp << 0);
        }
    }

    if (out_min)
        *out_min = min_non_normalized;
    if (out_max)
        *out_max = max_non_normalized;

    if (!*tex)
        glGenTextures (1, tex);

    glBindTexture (GL_TEXTURE_2D, *tex);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_buffer);
    glBindTexture (GL_TEXTURE_2D, 0);
}

bool ui_show_perlin_fractal_params (ImGuiID id, Perlin_Fractal_Params *params)
{
    ImGui::PushID (id);

    bool result = false;
    if (ImGui::SliderFloat ("Scale", &params->scale, 0.001, 0.2, "%.6f"))
        result = true;
    if (ImGui::SliderInt ("Octaves", &params->octaves, 1, Perlin_Fractal_Max_Octaves))
        result = true;
    if (ImGui::SliderFloat ("Persistance", &params->persistance, 0.001, 1))
        result = true;
    if (ImGui::SliderFloat ("Lacunarity", &params->lacunarity, 1, 10))
        result = true;

    ImGui::PopID ();

    return result;
}

void ui_show_perlin_test_window (bool *opened)
{
    static GLuint texture_handle;
    static int texture_size = 256;
    static int seed;
    static f32 offset_x, offset_y;
    static Perlin_Fractal_Params params = {0.05, 3, 0.5, 1.5};
    static f32 min_value = F32_MAX;
    static f32 max_value = -F32_MAX;

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

        ImGui::LabelText ("Min Value", "%.3f", min_value);
        ImGui::LabelText ("Max Value", "%.3f", max_value);

        if (ImGui::SliderInt ("Size", &texture_size, 128, 4096))
            should_generate = true;

        if (ui_show_perlin_fractal_params (0, &params))
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
            generate_noise_texture (&texture_handle, texture_size, texture_size, offset_x, offset_y, seed, params.scale, params.octaves, params.persistance, params.lacunarity, &min_value, &max_value);
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

        ImGui::LabelText ("Frame time", "%.2f ms, %.2f FPS", g_delta_time / 1000.0, 1000000.0 / g_delta_time);
        ImGui::LabelText ("Position", "%.2f %.2f %.2f", g_camera.position.x, g_camera.position.y, g_camera.position.z);
        ImGui::LabelText ("Average chunk creation   time", "%f us", g_chunk_creation_time / cast (f32) g_chunk_creation_samples);
        ImGui::LabelText ("Average chunk generation time", "%f us", g_chunk_generation_time / cast (f32) g_chunk_generation_samples);
        ImGui::LabelText ("Loaded chunks", "%lld", g_world.all_loaded_chunks.count);
        ImGui::LabelText ("Total vertex count", "%lld", total_vertex_count);
        ImGui::LabelText ("Drawn vertex count", "%lld", g_drawn_vertex_count);
        ImGui::LabelText ("Average vertices per chunk", "%lld", total_vertex_count / g_world.all_loaded_chunks.count);
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

    ImGui::LabelText ("Seed", "%d", g_world.seed);

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

void ui_show_terrain_noise_maps (bool generate = false)
{
    static GLuint continentalness_tex;
    static GLuint erosion_tex;
    static GLuint peaks_and_valleys_tex;
    static GLuint height_tex;

    static const f32 Scale = 1;

    static int size = 8;
    static bool only_noise = false;

    int lines = 3;
    auto child_height = ImGui::GetContentRegionAvail ().y - lines * ImGui::GetFrameHeightWithSpacing ();
    if (ImGui::BeginChild ("Noise Maps", {0, child_height}, true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        int column_count = clamp (cast (int) (ImGui::GetContentRegionAvail ().x / (size * Chunk_Size * Scale)), 1, 4);
        ImGui::Columns (column_count, 0, false);

        /*
        ImGui::Text ("Continentalness");
        ImGui::Image (cast (ImTextureID) continentalness_tex, {size * Chunk_Size * Scale, size * Chunk_Size * Scale});
        ImGui::NextColumn ();

        ImGui::Text ("Erosion");
        ImGui::Image (cast (ImTextureID) erosion_tex, {size * Chunk_Size * Scale, size * Chunk_Size * Scale});
        ImGui::NextColumn ();

        ImGui::Text ("Peaks And Valleys");
        ImGui::Image (cast (ImTextureID) peaks_and_valleys_tex, {size * Chunk_Size * Scale, size * Chunk_Size * Scale});
        ImGui::NextColumn ();
        */

        ImGui::Text ("Height");

        ImVec2 texture_size;
        texture_size.x = size * Chunk_Size * Scale;
        texture_size.y = texture_size.x;

        auto top_left = ImGui::GetCursorScreenPos ();

        ImGui::Image (cast (ImTextureID) height_tex, texture_size);

        if (ImGui::IsItemHovered ())
        {
            ImVec2 pos_in_tex = ImGui::GetMousePos () - top_left;
            pos_in_tex.y -= ImGui::GetScrollX ();
            pos_in_tex.x = pos_in_tex.x / texture_size.x * size * Chunk_Size;
            pos_in_tex.y = pos_in_tex.y / texture_size.y * size * Chunk_Size;

            f32 val = 0;
            int x = cast (int) (g_camera.position.x + pos_in_tex.x - Chunk_Size * size / 2);
            int z = cast (int) (g_camera.position.z + pos_in_tex.y - Chunk_Size * size / 2);
            auto chunk_x = x / Chunk_Size;
            auto chunk_z = z / Chunk_Size;
            auto chunk = world_get_chunk (&g_world, chunk_x, chunk_z);
            if (chunk)
            {
                int rel_x = x - chunk_x * Chunk_Size;
                int rel_z = z - chunk_z * Chunk_Size;
                val = chunk->terrain_values[rel_x * Chunk_Size + rel_z].surface_level;
            }

            ImGui::SetTooltip ("%d %d\n%f", x, z, val);
        }

        ImGui::NextColumn ();

        ImGui::Columns ();
    }
    ImGui::EndChild ();

    if (!continentalness_tex || !erosion_tex || !peaks_and_valleys_tex)
        generate = true;

    if (ImGui::SliderInt ("Size", &size, 1, 100))
        generate = true;

    if (ImGui::Checkbox ("Only Noise", &only_noise))
        generate = true;

    if (ImGui::Button ("Generate"))
        generate = true;

    if (generate)
    {
        generate_terrain_value_texture (&height_tex,
            cast (int) (g_camera.position.x / Chunk_Size), cast (int) (g_camera.position.z / Chunk_Size),
            size, Terrain_Value_Count, only_noise);
    }
}

void ui_show_terrain_params_editor (Terrain_Params *params)
{
    static const f32 Curve_Editor_Size_Ratio = 0.6;
    static const char *Value_Names[] = {
        "Continentalness",
        "Erosion",
        "Peaks and Valleys",
    };

    for_range (i, 0, 3)
    {
        if (ImGui::TreeNode (Value_Names[i]))
        {
            ImGui::Columns (2, 0, false);

            ImVec2 size;
            size.x = ImGui::GetContentRegionAvail ().x - ImGui::GetStyle ().WindowPadding.x;
            size.y = size.x * Curve_Editor_Size_Ratio;
            ImGuiExt::BezierCurveEditor ("Height Curve", size,
                array_size (params->bezier_points[i]),
                &params->bezier_point_counts[i],
                cast (ImVec2 *) params->bezier_points[i],
                ImVec2{0,1},
                ImVec2{cast (f32) params->height_range.x, cast (f32) params->height_range.y}
            );

            {
                ImGui::LogButtons ();

                auto control_points = params->bezier_points[i];
                int control_point_count = params->bezier_point_counts[i];
                int curve_count = ImGuiExt_BezierCurve_CurveCountFromPointCount (control_point_count);

                ImGui::LogText ("{\n");
                for (int i = 0; i < curve_count; i += 1)
                {
                    ImGui::LogText ("    {%f, %f}, {%f, %f}, {%f, %f},\n",
                        control_points[i * 3 + 0].x, control_points[i * 3 + 0].y,
                        control_points[i * 3 + 1].x, control_points[i * 3 + 1].y,
                        control_points[i * 3 + 2].x, control_points[i * 3 + 2].y
                    );
                }

                ImGui::LogText ("    {%f, %f},\n",
                    control_points[control_point_count - 1].x, control_points[control_point_count - 1].y
                );

                ImGui::LogText ("}");

                ImGui::LogFinish ();
            }

            ImGui::NextColumn ();

            ui_show_perlin_fractal_params (0, &params->perlin_params[i]);
            ImGui::SliderFloat ("Influence", &params->influences[i], 0.0f, 1.0f);

            {
                ImGui::LogButtons ();

                ImGui::LogText ("{ ");
                ImGui::LogText ("%f, %i, %f, %f", params->perlin_params[i].scale, params->perlin_params[i].octaves,
                    params->perlin_params[i].persistance, params->perlin_params[i].lacunarity);
                ImGui::LogText (" }");

                ImGui::LogFinish ();
            }

            if (ImGui::Button ("Default"))
            {
                memcpy (&params->bezier_points[i], Default_Bezier_Points[i], Default_Bezier_Point_Counts[i] * sizeof (Vec2f));
                params->bezier_point_counts[i] = Default_Bezier_Point_Counts[i];
                params->perlin_params[i] = Default_Perlin_Params[i];
            }

            ImGui::Columns ();

            ImGui::TreePop ();
        }
    }

    ImGui::SliderInt ("Min Height", &params->height_range.x, 0, params->height_range.y);
    ImGui::SliderInt ("Max Height", &params->height_range.y, params->height_range.x, Chunk_Height);

    ImGui::SliderInt ("Water Level", &params->water_level, 0, Chunk_Height);
}

void ui_show_advanced_world_settings ()
{
    static Terrain_Params world_params;

    ui_show_terrain_params_editor (&world_params);

    ImGui::Separator ();

    bool generated = false;

    if (ImGui::Button ("Generate New"))
    {
        world_clear_chunks (&g_world);
        world_init (&g_world, random_get_s32 (), g_render_distance / 2 + 1, world_params);
        generated = true;
    }

    ImGui::SameLine ();

    if (ImGui::Button ("Regenerate"))
    {
        world_clear_chunks (&g_world);
        world_init (&g_world, g_world.seed, g_render_distance / 2 + 1, world_params);
        generated = true;
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

            if (ImGui::BeginTabItem ("Advanced Settings"))
            {
                ui_show_advanced_world_settings ();
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

void ui_show_terrain_creator_window (bool *opened)
{
    static Terrain_Params params;
    static int size = 1000;
    static GLuint texture;
    static ImVec4 water_color = {0.2,0.3,0.6,1.0};
    static ImVec4 dirt_color = {0.6,0.4,0.2,1.0};
    static f32 global_scale = 1.0;
    static int seed = 127384;
    static bool show_c = true;
    static bool show_e = true;
    static bool show_pv = true;
    static bool show_colors = true;
    static Terrain_Values *values;

    if (ImGui::Begin ("Terrain Creator", opened))
    {
        ui_show_terrain_params_editor (&params);
        ImGui::Separator ();
        ImGui::ColorEdit3 ("Water Color", &water_color.x);
        ImGui::ColorEdit3 ("Dirt Color", &dirt_color.x);
        ImGui::SliderInt ("Size", &size, 128, 10000);
        ImGui::SliderFloat ("Global Scale", &global_scale, 0.1, 5.0);

        ImGui::Checkbox ("Show Continentalness", &show_c);
        ImGui::Checkbox ("Show Erosion", &show_e);
        ImGui::Checkbox ("Show Peaks and Valleys", &show_pv);
        ImGui::Checkbox ("Show Colors", &show_colors);

        bool generate = false;

        if (!values)
            generate = true;

        if (ImGui::Button ("Generate New"))
        {
            seed = random_get_s32 ();
            generate = true;
        }

        ImGui::SameLine ();

        if (ImGui::Button ("Regenerate"))
        {
            generate = true;
        }

        if (ImGui::Button ("Generate World"))
        {
            generate = true;
            world_clear_chunks (&g_world);
            world_init (&g_world, seed, g_render_distance / 2 + 1, params);
        }

        if (generate)
        {
            mem_free (values, heap_allocator ());
            values = mem_alloc_uninit (Terrain_Values, size * size, heap_allocator ());

            u32 *pixels = mem_alloc_uninit (u32, size * size, heap_allocator ());
            defer (mem_free (pixels, heap_allocator ()));

            LC_RNG rng;
            random_seed (&rng, seed);

            Vec2f coffsets[Perlin_Fractal_Max_Octaves];
            perlin_generate_offsets (&rng, params.perlin_params[0].octaves, coffsets);

            Vec2f eoffsets[Perlin_Fractal_Max_Octaves];
            perlin_generate_offsets (&rng, params.perlin_params[1].octaves, eoffsets);

            Vec2f pvoffsets[Perlin_Fractal_Max_Octaves];
            perlin_generate_offsets (&rng, params.perlin_params[1].octaves, pvoffsets);

            f32 cmax = perlin_fractal_max (params.perlin_params[0].octaves, params.perlin_params[0].persistance);
            f32 emax = perlin_fractal_max (params.perlin_params[1].octaves, params.perlin_params[1].persistance);
            f32 pvmax = perlin_fractal_max (params.perlin_params[2].octaves, params.perlin_params[2].persistance);
            for_range (x, 0, size)
            {
                for_range (y, 0, size)
                {
                    auto val = &values[x * size + y];

                    f32 perlin_x = cast (f32) x * global_scale;
                    f32 perlin_y = cast (f32) y * global_scale;

                    f32 normalized_surface_level = 0.0;
                    f32 div = 0.0;
                    if (show_c)
                    {
                        auto cnoise = cast (f32) perlin_fractal_noise (params.perlin_params[0], coffsets, cast (f32) perlin_x, cast (f32) perlin_y);
                        cnoise = inverse_lerp (-cmax, cmax, cnoise);

                        val->continentalness_noise = cnoise;

                        val->continentalness_bezier = bezier_cubic_calculate (params.bezier_point_counts[0], params.bezier_points[0], cnoise).y * params.influences[0];

                        normalized_surface_level += val->continentalness_bezier;

                        div += 1;
                    }
                    if (show_e)
                    {
                        auto enoise = cast (f32) perlin_fractal_noise (params.perlin_params[1], eoffsets, cast (f32) perlin_x, cast (f32) perlin_y);
                        enoise = inverse_lerp (-emax, emax, enoise);

                        val->erosion_noise = enoise;

                        val->erosion_bezier = bezier_cubic_calculate (params.bezier_point_counts[1], params.bezier_points[1], enoise).y * params.influences[1];

                        normalized_surface_level += val->erosion_bezier;

                        div += 1;
                    }
                    if (show_pv)
                    {
                        auto pvnoise = cast (f32) perlin_fractal_noise (params.perlin_params[2], pvoffsets, cast (f32) perlin_x, cast (f32) perlin_y);
                        pvnoise = inverse_lerp (-pvmax, pvmax, pvnoise);

                        val->peaks_and_valleys_noise = -3 * (fabs (fabs (pvnoise) - 0.6666667f) - 0.3333334f);

                        val->peaks_and_valleys_bezier = bezier_cubic_calculate (params.bezier_point_counts[2], params.bezier_points[2], pvnoise).y * params.influences[2];

                        normalized_surface_level += val->peaks_and_valleys_bezier;

                        div += 1;
                    }

                    normalized_surface_level /= div;

                    auto surface_level = lerp (
                        cast (f32) params.height_range.x,
                        cast (f32) params.height_range.y,
                        normalized_surface_level
                    );

                    ImVec4 color;
                    if (show_colors)
                    {
                        if (surface_level <= params.water_level)
                        {
                            color = water_color;
                            f32 depth = inverse_lerp (cast (f32) params.height_range.x, cast (f32) params.water_level, surface_level);
                            depth = max (depth, 0.2f);
                            color.x *= depth;
                            color.y *= depth;
                            color.z *= depth;
                        }
                        else
                        {
                            color = dirt_color;
                            f32 depth = inverse_lerp (cast (f32) params.water_level, cast (f32) params.height_range.y, surface_level);
                            depth = clamp (depth, 0.3f, 1.0f);
                            color.x *= depth;
                            color.y *= depth;
                            color.z *= depth;
                        }
                    }
                    else
                    {
                        color = ImVec4{normalized_surface_level, normalized_surface_level, normalized_surface_level, 1.0};
                    }

                    u32 ucolor = ImGui::ColorConvertFloat4ToU32 (color);
                    pixels[y * size + x] = ucolor;
                }
            }

            if (!texture)
                glGenTextures (1, &texture);
            glBindTexture (GL_TEXTURE_2D, texture);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            glBindTexture (GL_TEXTURE_2D, 0);
        }
    }
    ImGui::End ();

    if (*opened)
    {
        if (ImGui::Begin ("Terrain Texture"))
        {
            ImVec2 texture_size;
            texture_size.x = ImGui::GetContentRegionAvail ().x;
            texture_size.y = texture_size.x;

            auto top_left = ImGui::GetCursorScreenPos ();
            ImGui::Image (cast (ImTextureID) texture, texture_size);
            if (ImGui::IsItemHovered ())
            {
                ImVec2 pos_in_tex = ImGui::GetMousePos () - top_left;
                pos_in_tex.y -= ImGui::GetScrollX ();
                pos_in_tex.x = pos_in_tex.x / texture_size.x * size;
                pos_in_tex.y = pos_in_tex.y / texture_size.y * size;

                auto val = values[cast (int) pos_in_tex.x * size + cast (int) pos_in_tex.y];

                ImGui::SetTooltip ("%f %f\nC: %.3f E: %.3f PV: %.3f",
                    pos_in_tex.x, pos_in_tex.y,
                    val.bezier_values[0], val.bezier_values[1], val.bezier_values[2]);
            }
        }
        ImGui::End ();
    }
}

void ui_show_cubiomes_viewer (bool *opened)
{
    static GLuint texture;
    static Generator gen;
    static int size = 4096;
    static int seed = 218936743;
    static int noise_map_type = 0;

    if (ImGui::Begin ("Cubiomes Viewer", opened))
    {
        bool generate = false;

        if (!texture)
        {
            glGenTextures (1, &texture);
            setupGenerator (&gen, MC_1_20, 0);
            applySeed (&gen, DIM_OVERWORLD, seed);
            generate = true;
        }

        int lines = 3;
        auto child_height = ImGui::GetContentRegionAvail ().y - lines * ImGui::GetFrameHeightWithSpacing ();
        if (ImGui::BeginChild ("Noise Map", {0, child_height}, true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGui::Image (cast (ImTextureID) texture, ImVec2{cast (f32) size, cast (f32) size});
        }
        ImGui::EndChild ();

        ImGui::SliderInt ("Size", &size, 128, 8192);

        if (ImGui::Combo ("Noise Map", &noise_map_type, "Temperature\0Humidity\0Continentalness\0Erosion\0Depth\0Weirdness\0"))
            generate = true;

        if (ImGui::Button ("Generate"))
            generate = true;

        if (generate)
        {
            u32 *pixels = mem_alloc_uninit (u32, size * size, heap_allocator ());
            defer (mem_free (pixels, heap_allocator ()));

            setClimateParaSeed (&gen.bn, gen.seed, 0, noise_map_type, 4);

            for (int j = 0; j < size; j += 1)
            {
                for (int i = 0; i < size; i += 1)
                {
                    float y = sampleClimatePara (&gen.bn, null, cast (f64) i, cast (f64) j);

                    pixels[j * size + i] = ImGui::ColorConvertFloat4ToU32 (ImVec4{y, y, y, 1});
                }
            }

            glBindTexture (GL_TEXTURE_2D, texture);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            glBindTexture (GL_TEXTURE_2D, 0);
        }
    }
    ImGui::End ();
}

void ui_show_windows ()
{
    static bool show_terrain_creator = false;
    static bool show_cubiomes_viewer = false;

    if (ImGui::BeginMainMenuBar ())
    {
        if (ImGui::Button ("Demo Window"))
            g_show_demo_window = true;
        if (ImGui::Button ("Metrics and Settings"))
            g_show_metrics_window = true;
        if (ImGui::Button ("Perlin Test"))
            g_show_perlin_test_window = true;
        if (ImGui::Button ("Texture Atlas"))
            g_show_texture_atlas_window = true;
        if (ImGui::Button ("World Settings"))
            g_show_world_window = true;
        if (ImGui::Button ("Terrain Noise Maps"))
            g_show_terrain_noise_maps_window = true;
        if (ImGui::Button ("Terrain Creator"))
            show_terrain_creator = true;
        if (ImGui::Button ("Cubiomes Viewer"))
            show_cubiomes_viewer = true;

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

    if (g_show_terrain_noise_maps_window)
    {
        if (ImGui::Begin ("Terrain Noise Maps", &g_show_terrain_noise_maps_window))
        {
            ui_show_terrain_noise_maps (&g_show_world_window);
        }
        ImGui::End ();
    }

    if (show_terrain_creator)
        ui_show_terrain_creator_window (&show_terrain_creator);

    if (show_cubiomes_viewer)
        ui_show_cubiomes_viewer (&show_cubiomes_viewer);
}
