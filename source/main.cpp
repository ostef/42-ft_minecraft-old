#include "Minecraft.hpp"

Arena     frame_arena;
Allocator frame_allocator;

Vec2f g_prev_mouse_pos;
Vec2f g_curr_mouse_pos;
Vec2f g_mouse_delta;

GLFWwindow *g_window;

World g_world;

void glfw_error_callback (int error, const char *description)
{
    println ("GLFW Error (%d): %s", error, description);
}

int main (int argc, const char **args)
{
    Vec2f p = {1, 2};

    platform_init ();
    crash_handler_init ();

    arena_init (&frame_arena, 4096, heap_allocator ());

    glfwSetErrorCallback (glfw_error_callback);

    if (!glfwInit ())
        return 1;

    defer (glfwTerminate ());

    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint (GLFW_SAMPLES, 4);

    g_window = glfwCreateWindow (1280, 720, "ft_minecraft", null, null);
    if (!g_window)
        return 1;

    defer (glfwDestroyWindow (g_window));

    if (glfwRawMouseMotionSupported ())
        glfwSetInputMode (g_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    glfwMakeContextCurrent (g_window);
    glfwSwapInterval(1);

    int gl_version = gladLoadGL (cast (GLADloadfunc) glfwGetProcAddress);
    println ("GL Version %d.%d", GLAD_VERSION_MAJOR (gl_version), GLAD_VERSION_MINOR (gl_version));

    IMGUI_CHECKVERSION ();
    ImGui::CreateContext ();
    auto io = ImGui::GetIO ();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.Fonts->AddFontFromFileTTF ("data/Roboto-Regular.ttf", 18);

    ImGui::StyleColorsDark ();
    {
        auto style = &ImGui::GetStyle ();
        style->WindowRounding = 10;
        style->WindowTitleAlign = {0.5, 0.5};
        style->ChildRounding = 3;
        style->FrameRounding = 6;
        style->PopupRounding = 3;
        style->GrabRounding = 4;
        style->TabRounding = 6;
        style->FramePadding = {10, 6};
        style->SeparatorTextBorderSize = 1;
        style->FrameBorderSize = 1;
        style->ItemInnerSpacing.x = 8;
        style->Colors[ImGuiCol_Border].w = 0.25;
    }

    ImGui_ImplGlfw_InitForOpenGL (g_window, true);
    ImGui_ImplOpenGL3_Init ("#version 130");

    glEnable (GL_MULTISAMPLE);
    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_LESS);
    glClearColor (0.2, 0.3, 0.6, 1.0);	// Sky color
    glEnable (GL_CULL_FACE);
    glFrontFace (GL_CW);

    if (!render_init ())
    {
        println ("Could not initialize rendering system");
        return 1;
    }

    Vec3f camera_position {};
    Vec3f camera_direction {};

    Camera camera = {};
    camera.position.y = 40;
    camera.fov = 60;
    camera.rotation = {};
    camera.transform = {};
    camera.view_matrix = {};
    camera.projection_matrix = {};
    camera.view_projection_matrix = {};

    {
        f64 mx, my;
        glfwGetCursorPos (g_window, &mx, &my);
        g_curr_mouse_pos = {cast (f32) mx, cast (f32) my};
    }

    world_init (&g_world);

    s64 chunk_generation_time = 0;
    s64 chunk_generation_samples = 0;
    s64 chunk_creation_time = 0;
    s64 chunk_creation_samples = 0;

    bool generate_new_chunks = true;

    int render_distance = 5;
    int generation_height = 2;

    bool show_demo_window = false;
    bool show_perlin_test = false;
    bool show_metrics_window = true;
    while (!glfwWindowShouldClose (g_window))
    {
        arena_reset (&frame_arena);

        glfwPollEvents ();
        g_prev_mouse_pos = g_curr_mouse_pos;

        f64 mx, my;
        glfwGetCursorPos (g_window, &mx, &my);
        g_curr_mouse_pos = {cast (f32) mx, cast (f32) my};
        g_mouse_delta = g_curr_mouse_pos - g_prev_mouse_pos;

        update_flying_camera (&camera);

        if (generate_new_chunks)
        {
            for_range (x, -render_distance, render_distance)
            {
                for_range (y, -generation_height, generation_height)
                {
                    for_range (z, -render_distance, render_distance)
                    {
                        Vec2f planar_camera_pos = Vec2f{camera.position.x, camera.position.z};
                        Vec2f planar_chunk_pos = Vec2f{cast (f32) x * Chunk_Size, cast (f32) z * Chunk_Size};
                        planar_chunk_pos += planar_camera_pos;

                        if (distance (planar_camera_pos, planar_chunk_pos) < render_distance * Chunk_Size)
                        {
                            s64 time_start = time_current_monotonic ();

                            auto current_chunk = world_create_chunk (&g_world,
                                cast (s64) camera.position.x / Chunk_Size + x,
                                cast (s64) camera.position.y / Chunk_Size + y,
                                cast (s64) camera.position.z / Chunk_Size + z
                            );

                            if (!current_chunk->generated)
                            {
                                chunk_creation_time += time_current_monotonic () - time_start;
                                chunk_creation_samples += 1;

                                chunk_generate (current_chunk);

                                s64 time_end = time_current_monotonic ();
                                chunk_generation_time += time_end - time_start;
                                chunk_generation_samples += 1;
                            }
                        }
                    }
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame ();
        ImGui_ImplGlfw_NewFrame ();
        ImGui::NewFrame ();

        if (show_demo_window)
           ImGui::ShowDemoWindow (&show_demo_window);

        if (show_metrics_window)
        {
            if (ImGui::Begin ("Metrics", &show_metrics_window))
            {
                s64 total_vertex_count = 0;
                for_array (i, g_world.all_loaded_chunks)
                    total_vertex_count += g_world.all_loaded_chunks[i]->vertices.count;

                ImGui::Text ("Average chunk creation   time: %f us", chunk_creation_time / cast (f32) chunk_creation_samples);
                ImGui::Text ("Average chunk generation time: %f us", chunk_generation_time / cast (f32) chunk_generation_samples);
                ImGui::Text ("Loaded chunks:      %lld", g_world.all_loaded_chunks.count);
                ImGui::Text ("Total vertex count: %lld", total_vertex_count);
                ImGui::Checkbox ("Generate new chunks", &generate_new_chunks);
                ImGui::SliderInt ("Render distance", &render_distance, 1, 12);
            }
            ImGui::End ();
        }

        show_perlin_test_window (&show_perlin_test);

        int width, height;
        glfwGetFramebufferSize (g_window, &width, &height);
        glViewport (0, 0, width, height);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for_array (i, g_world.all_loaded_chunks)
        {
            auto chunk = g_world.all_loaded_chunks[i];
            Vec3f world_chunk_pos = {cast (f32) chunk->x * Chunk_Size, cast (f32) chunk->y * Chunk_Size, cast (f32) chunk->z * Chunk_Size};
            if (distance (world_chunk_pos, camera.position) < cast (f64) render_distance * Chunk_Size)
            {
                chunk_generate_mesh_data (chunk);
                draw_chunk (chunk, &camera);
            }
        }

        ImGui::Render ();
        ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());

        glfwSwapBuffers (g_window);
    }

    return 0;
}
