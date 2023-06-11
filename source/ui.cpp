#include "Minecraft.hpp"

static bool g_show_demo_window;
static bool g_show_metrics_window;
static bool g_show_perlin_test_window;
static bool g_show_texture_atlas_window;
static bool g_show_world_window;
static bool g_show_terrain_noise_maps_window;

void generate_terrain_value_texture (GLuint *tex, int x, int z, int size, Terrain_Value terrain_value)
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
                    if (terrain_value == Terrain_Value_Surface)
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
                        f32 val = chunk->terrain_values[cx * Chunk_Size + cz].noise[terrain_value];

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
            if (val < min_non_normalized)
                min_non_normalized = val;
            if (val > max_non_normalized)
                max_non_normalized = val;
            val = inverse_lerp (-max_val, max_val, val);

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
                total_vertex_count += chunk->total_vertex_count;
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
        auto draw_list = ImGui::GetWindowDrawList ();
        auto avail_width = ImGui::GetContentRegionAvail ().x;
        ImVec2 min = ImGui::GetCursorScreenPos ();

        static const float Checkerboard_Grid_Size = 16;
        int checkerboard_size = cast (int) (avail_width / Checkerboard_Grid_Size);
        for_range (i, 0, checkerboard_size)
        {
            for_range (j, 0, checkerboard_size)
            {
                if ((i + j) % 2 == 0)
                    continue;

                draw_list->AddRectFilled (
                    min + ImVec2{i*Checkerboard_Grid_Size, j*Checkerboard_Grid_Size},
                    min + ImVec2{(i + 1)*Checkerboard_Grid_Size, (j + 1)*Checkerboard_Grid_Size},
                    ImGui::ColorConvertFloat4ToU32 (ImVec4{1,1,1,0.5})
                );
            }
        }

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
    static GLuint weirdness_tex;
    static GLuint surface_tex;

    static const f32 Scale = 1;

    static int size = 8;

    int lines = 3;
    auto child_height = ImGui::GetContentRegionAvail ().y - lines * ImGui::GetFrameHeightWithSpacing ();
    if (ImGui::BeginChild ("Noise Maps", {0, child_height}, true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        int column_count = clamp (cast (int) (ImGui::GetContentRegionAvail ().x / (size * Chunk_Size * Scale)), 1, 4);
        ImGui::Columns (column_count, 0, false);

        ImGui::Text ("Continentalness");
        ImGui::Image (cast (ImTextureID) continentalness_tex, {size * Chunk_Size * Scale, size * Chunk_Size * Scale});
        ImGui::NextColumn ();

        ImGui::Text ("Erosion");
        ImGui::Image (cast (ImTextureID) erosion_tex, {size * Chunk_Size * Scale, size * Chunk_Size * Scale});
        ImGui::NextColumn ();

        ImGui::Text ("Weirdness");
        ImGui::Image (cast (ImTextureID) weirdness_tex, {size * Chunk_Size * Scale, size * Chunk_Size * Scale});
        ImGui::NextColumn ();

        ImGui::Text ("Surface");
        ImGui::Image (cast (ImTextureID) surface_tex, {size * Chunk_Size * Scale, size * Chunk_Size * Scale});
        ImGui::NextColumn ();

        ImGui::Columns ();
    }
    ImGui::EndChild ();

    if (!continentalness_tex || !erosion_tex || !weirdness_tex || !surface_tex)
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

        generate_terrain_value_texture (&weirdness_tex,
            cast (int) (g_camera.position.x / Chunk_Size), cast (int) (g_camera.position.z / Chunk_Size),
            size, Terrain_Value_Weirdness);

        generate_terrain_value_texture (&surface_tex,
            cast (int) (g_camera.position.x / Chunk_Size), cast (int) (g_camera.position.z / Chunk_Size),
            size, Terrain_Value_Surface);
    }
}

/*
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
            ImGuiExt::BezierSplineEditor ("Height Curve", size,
                array_size (params->bezier_points[i]),
                &params->bezier_point_counts[i],
                cast (ImVec2 *) params->bezier_points[i]
            );

            {
                ImGui::LogButtons ();

                auto control_points = params->bezier_points[i];
                int control_point_count = params->bezier_point_counts[i];
                int curve_count = ImGuiExt_BezierSpline_CurveCountFromPointCount (control_point_count);

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
*/

void ui_show_advanced_world_settings ()
{
    static Terrain_Params world_params;

    //ui_show_terrain_params_editor (&world_params);

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

void ui_show_cubiomes_viewer (bool *opened)
{
    static GLuint texture;
    static cubiome::Generator gen;
    static int size = 4096;
    static int seed = 218936743;
    static int noise_map_type = cubiome::NP_WEIRDNESS;

    if (ImGui::Begin ("Cubiomes Viewer", opened))
    {
        bool generate = false;

        if (!texture)
        {
            glGenTextures (1, &texture);
            cubiome::setupGenerator (&gen, cubiome::MC_1_20, 0);
            cubiome::applySeed (&gen, cubiome::DIM_OVERWORLD, seed);
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

            cubiome::setClimateParaSeed (&gen.bn, gen.seed, 0, noise_map_type, 4);

            for (int j = 0; j < size; j += 1)
            {
                for (int i = 0; i < size; i += 1)
                {
                    float y = cubiome::sampleClimatePara (&gen.bn, null, cast (f64) i, cast (f64) j);
                    // s64 np[6];
                    // cubiome::sampleBiomeNoise (&gen.bn, np, i, 0, j, null, 0);
                    // float y = np[cubiome::NP_WEIRDNESS
                    y = -3.0F * ( fabsf( fabsf(y) - 0.6666667F ) - 0.33333334F );
                    y = inverse_lerp (-1.0f, 1.0f, y);

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

void calculate_spline_range (Nested_Hermite_Spline *sp, Vec2f *x, Vec2f *y)
{
    for_array (i, sp->knots)
    {
        if (sp->knots[i].x < x->x)
            x->x = sp->knots[i].x;
        if (sp->knots[i].x > x->y)
            x->y = sp->knots[i].x;

        if (sp->knots[i].is_nested_spline)
        {
            calculate_spline_range (sp->knots[i].spline, x, y);
        }
        else
        {
            if (sp->knots[i].y < y->x)
                y->x = sp->knots[i].y;
            if (sp->knots[i].y > y->y)
                y->y = sp->knots[i].y;
        }
    }
}

void convert_cubiome_spline (cubiome::Spline *cub_sp, Nested_Hermite_Spline *sp)
{
    f32 x_min = F32_MAX;
    f32 x_max = -F32_MAX;
    for_range (i, 0, cub_sp->len)
    {
        x_max = max (cub_sp->loc[i], x_max);
        x_min = min (cub_sp->loc[i], x_min);
    }

    sp->t_value_index = cub_sp->typ;
    for_range (i, 0, cub_sp->len)
    {
        Nested_Hermite_Spline::Knot knot {};
        knot.derivative = cub_sp->der[i];
        knot.x = inverse_lerp (x_min, x_max, cub_sp->loc[i]);

        if (cub_sp->val[i]->len <= 1)
        {
            knot.y = (cast (cubiome::FixSpline *) cub_sp->val[i])->val;
            knot.y = (knot.y + 1) * 0.5f;
        }
        else
        {
            knot.is_nested_spline = true;
            knot.spline = mem_alloc_typed (Nested_Hermite_Spline, 1, heap_allocator ());
            convert_cubiome_spline (cub_sp->val[i], knot.spline);
        }

        array_push (&sp->knots, knot);
    }
}

void fix_spline_range (Nested_Hermite_Spline *sp, f32 x_min, f32 x_max, f32 y_min, f32 y_max)
{
    for_array (i, sp->knots)
    {
        //sp->knots[i].x = inverse_lerp (x_min, x_max, sp->knots[i].x);
        sp->knots[i].y = inverse_lerp (y_min, y_max, sp->knots[i].y);
        if (sp->knots[i].is_nested_spline)
            fix_spline_range (sp->knots[i].spline, x_min, x_max, y_min, y_max);
    }
}

void ui_show_windows ()
{
    static bool show_terrain_creator = false;
    static bool show_cubiomes_viewer = false;

    if (ImGui::BeginMainMenuBar ())
    {
        if (ImGui::MenuItem ("Demo Window"))
            g_show_demo_window = true;
        if (ImGui::MenuItem ("Metrics and Settings"))
            g_show_metrics_window = true;
        if (ImGui::MenuItem ("Perlin Test"))
            g_show_perlin_test_window = true;
        if (ImGui::MenuItem ("Texture Atlas"))
            g_show_texture_atlas_window = true;
        if (ImGui::MenuItem ("World Settings"))
            g_show_world_window = true;
        if (ImGui::MenuItem ("Terrain Noise Maps"))
            g_show_terrain_noise_maps_window = true;
        // if (ImGui::MenuItem ("Terrain Creator"))
        //     show_terrain_creator = true;
        if (ImGui::MenuItem ("Cubiomes Viewer"))
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

    // if (show_terrain_creator)
    //     ui_show_terrain_creator_window (&show_terrain_creator);

    if (show_cubiomes_viewer)
        ui_show_cubiomes_viewer (&show_cubiomes_viewer);

    if (ImGui::Begin ("Pan Zoom Test"))
    {
        static ImVec2 offset = {-1.0f, -1.0f};
        static float scale = 100;
        static ImGuiExt::HermiteSplineParams params = ImGuiExt::HermiteSplineParams_Default;
        params.ViewParams.XOffsetRange = {-2.0f, 2.0f};
        params.ViewParams.YOffsetRange = {-2.0f, 2.0f};
        params.XRange = {-1.0f, 1.0f};
        params.YRange = {-2.0f, 1.0f};

        static const int Max_Splines = 10;

        static Static_Array<Nested_Hermite_Spline, Max_Splines> splines;
        static int selected_spline = -1;
        static int selected_point = -1;

        if (splines.count == 0)
        {
            array_push (&splines);
            selected_spline = 0;
        }

        ImGui::Columns (2);

        if (ImGuiExt::BeginHermiteSpline ("Hermite Spline", &offset, &scale, {500, 400}, true, 0, params))
        {
            if (selected_spline >= 0)
            {
                auto spline = &splines[selected_spline];
                for_range (i, 0, spline->knots.count)
                {
                    auto knot = &spline->knots[i];

                    ImGuiExt::HermiteSplinePointValues curr = {&knot->x, &knot->y, &knot->derivative};
                    ImGuiExt::HermiteSplinePointValues next = {};
                    if (i != spline->knots.count - 1)
                    {
                        auto next_knot = &spline->knots[i + 1];
                        next = {&next_knot->x, &next_knot->y, &next_knot->derivative};
                    }

                    if (ImGuiExt::HermiteSplinePoint (ImGui::GetID (knot), offset, scale, selected_point == i, curr, next))
                        selected_point = i;

                    if (ImGuiExt::IsItemDoubleClicked (ImGuiMouseButton_Left))
                    {
                        if (knot->is_nested_spline)
                        {
                            selected_spline = cast (int) (knot->spline - splines.data);
                        }
                        else
                        {
                            knot->is_nested_spline = true;
                            knot->spline = array_push (&splines);
                            selected_spline = splines.count - 1;
                        }
                    }

                    knot->x = clamp (knot->x, params.XRange.x, params.XRange.y);
                    knot->y = clamp (knot->y, params.YRange.x, params.YRange.y);
                }
            }
        }

        if (selected_spline >= 0 && !ImGui::IsAnyItemHovered () && ImGui::IsWindowHovered () && ImGui::IsMouseDoubleClicked (ImGuiMouseButton_Left))
        {
            auto spline = &splines[selected_spline];
            auto *pt = array_push (&spline->knots);

            auto mouse_pos = ImGui::GetMousePos () - ImGui::GetWindowPos ();

            ImGuiExt::WindowToPanZoom (offset, scale, &mouse_pos, null, false);
            pt->x = mouse_pos.x;
            pt->y = mouse_pos.y;
            pt->derivative = 0;
        }

        ImGuiExt::EndHermiteSpline ();

        ImGui::NextColumn ();

        if (selected_spline >= 0 && selected_point >= 0)
        {
            auto spline = &splines[selected_spline];
            auto point = &spline->knots[selected_point];

            float v_min = -1.0f;
            float v_max = 1.0f;
            ImGui::DragScalar ("Location", ImGuiDataType_Float, &point->x, 0.01f, &v_min, &v_max, "%.2f");
            ImGui::DragScalar ("Value", ImGuiDataType_Float, &point->y, 0.01f, &v_min, &v_max, "%.2f");

            v_min = -100.0f;
            v_max = 100.0f;
            ImGui::DragScalar ("Derivative", ImGuiDataType_Float, &point->derivative, 0.01f, &v_min, &v_max, "%.2f");
        }

        ImGui::Columns ();
    }
    ImGui::End ();
}
