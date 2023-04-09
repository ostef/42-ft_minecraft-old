#include "Minecraft.hpp"

static bool g_show_demo_window;
static bool g_show_metrics_window;
static bool g_show_perlin_test_window;
static bool g_show_texture_atlas_window;

void ui_show_perlin_test_window (bool *opened)
{
    static f32 scale = 0.1;
    static GLuint texture_handle;
    static u32 *texture_buffer;
    static int texture_size;
    static int ui_texture_size = 256;
    static Vec2i offset = {-2, -2};
    static f32 z;

    if (ImGui::Begin ("Perlin Test", opened))
    {
        {
            auto child_height = ImGui::GetContentRegionAvail ().y - 4 * ImGui::GetFrameHeightWithSpacing ();
            if (ImGui::BeginChild ("Image", {0, child_height}, true, ImGuiWindowFlags_HorizontalScrollbar))
            {
                if (texture_handle)
                    ImGui::Image (cast (ImTextureID) texture_handle, {cast (f32) texture_size, cast (f32) texture_size});
            }
            ImGui::EndChild ();
        }

        ImGui::SliderInt ("Size", &ui_texture_size, 128, 4096);
        ImGui::SliderFloat ("Scale", &scale, 0.001, 1);

        bool should_generate = false;

        if (ImGui::SliderFloat ("Z", &z, -100, 100))
            should_generate = true;

        if (ImGui::Button ("Generate"))
        {
            should_generate = true;
        }

        ImGui::SameLine ();

        if (ImGui::Button ("Randomize"))
        {
            offset.x = cast (int) random_get ();
            offset.y = cast (int) random_get ();
            should_generate = true;
        }

        if (should_generate)
        {
            if (texture_size != ui_texture_size || !texture_buffer)
            {
                texture_size = ui_texture_size;
                mem_free (texture_buffer, heap_allocator ());
                texture_buffer = mem_alloc_uninit (u32, texture_size * texture_size, heap_allocator ());
                assert (texture_buffer != null);
            }

            if (texture_handle == 0)
            {
                glGenTextures (1, &texture_handle);
                assert (texture_handle != 0);
                glBindTexture (GL_TEXTURE_2D, texture_handle);
                glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            }

            for_range (i, 0, texture_size)
            {
                for_range (j, 0, texture_size)
                {
                    auto value = perlin_noise (cast (f64) (i + offset.x) * scale, cast (f64) (j + offset.y) * scale, z);
                    value = (value + 1) * 0.5;
                    u8 color_comp = cast (u8) (value * 255);
                    texture_buffer[i * texture_size + j] = (0xff << 24) | (color_comp << 16) | (color_comp << 8) | (color_comp << 0);
                }
            }

            glBindTexture (GL_TEXTURE_2D, texture_handle);
            glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, texture_size, texture_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_buffer);
            glBindTexture (GL_TEXTURE_2D, 0);
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
            for_range (i, 0, Max_Chunk_Y - Min_Chunk_Y)
            {
                auto chunk = (*it.value)[i];
                if (chunk)
                    total_vertex_count += chunk->vertex_count;
            }
        }

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
}
