#include "Minecraft.hpp"

Arena     frame_arena;
Allocator frame_allocator;

Vec2f g_prev_mouse_pos;
Vec2f g_curr_mouse_pos;
Vec2f g_mouse_delta;

GLFWwindow *g_window;

World g_world;
Camera g_camera;

s64 g_chunk_generation_time = 0;
s64 g_chunk_generation_samples = 0;
s64 g_chunk_creation_time = 0;
s64 g_chunk_creation_samples = 0;
s64 g_drawn_vertex_count = 0;

bool g_generate_new_chunks = true;

int g_render_distance = 5;
int g_generation_height = 2;

void glfw_error_callback (int error, const char *description)
{
    println ("GLFW Error (%d): %s", error, description);
}

int main (int argc, const char **args)
{
    platform_init ();
    crash_handler_init ();

    if (!arena_init (&frame_arena, 4096, heap_allocator ()))
        panic ("Could not initialize frame arena");

    frame_allocator = arena_allocator (&frame_arena);

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

    if (!load_texture_atlas ("data/textures"))
    {
        println ("Error: could not load textures");
        return 1;
    }

    if (!render_init ())
    {
        println ("Could not initialize rendering system");
        return 1;
    }

    Vec3f camera_position {};
    Vec3f camera_direction {};

    g_camera.position.y = 80;
    g_camera.fov = 60;
    g_camera.rotation = {};
    g_camera.transform = {};
    g_camera.view_matrix = {};
    g_camera.projection_matrix = {};
    g_camera.view_projection_matrix = {};

    {
        f64 mx, my;
        glfwGetCursorPos (g_window, &mx, &my);
        g_curr_mouse_pos = {cast (f32) mx, cast (f32) my};
    }

    world_init (&g_world);

    while (!glfwWindowShouldClose (g_window))
    {
        arena_reset (&frame_arena);

        glfwPollEvents ();
        g_prev_mouse_pos = g_curr_mouse_pos;

        f64 mx, my;
        glfwGetCursorPos (g_window, &mx, &my);
        g_curr_mouse_pos = {cast (f32) mx, cast (f32) my};
        g_mouse_delta = g_curr_mouse_pos - g_prev_mouse_pos;

        update_flying_camera (&g_camera);

        if (g_generate_new_chunks)
        {
            s64 camera_chunk_x = cast (s64) g_camera.position.x / Chunk_Size;
            s64 camera_chunk_y = cast (s64) g_camera.position.y / Chunk_Size;
            s64 camera_chunk_z = cast (s64) g_camera.position.z / Chunk_Size;

            for_range (x, camera_chunk_x - g_render_distance, camera_chunk_x + g_render_distance)
            {
                for_range (y,
                    clamp (camera_chunk_y - g_generation_height, cast (s64) Min_Chunk_Y, cast (s64) Max_Chunk_Y),
                    clamp (camera_chunk_y + g_generation_height, cast (s64) Min_Chunk_Y, cast (s64) Max_Chunk_Y)
                )
                {
                    for_range (z, camera_chunk_z - g_render_distance, camera_chunk_z + g_render_distance)
                    {
                        Vec2f planar_camera_pos = Vec2f{g_camera.position.x, g_camera.position.z};
                        Vec2f planar_chunk_pos = Vec2f{cast (f32) x * Chunk_Size, cast (f32) z * Chunk_Size};

                        if (distance (planar_camera_pos, planar_chunk_pos) < g_render_distance * Chunk_Size)
                        {
                            s64 time_start = time_current_monotonic ();

                            auto current_chunk = world_create_chunk (&g_world, x, y, z);

                            if (!current_chunk->generated)
                            {
                                g_chunk_creation_time += time_current_monotonic () - time_start;
                                g_chunk_creation_samples += 1;

                                chunk_generate (current_chunk);

                                s64 time_end = time_current_monotonic ();
                                g_chunk_generation_time += time_end - time_start;
                                g_chunk_generation_samples += 1;
                            }
                        }
                    }
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame ();
        ImGui_ImplGlfw_NewFrame ();
        ImGui::NewFrame ();

        ui_show_windows ();

        int width, height;
        glfwGetFramebufferSize (g_window, &width, &height);
        glViewport (0, 0, width, height);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        g_drawn_vertex_count = 0;
        for_hash_map (it, g_world.all_loaded_chunks)
        {
            for_range (i, 0, Max_Chunk_Y - Min_Chunk_Y)
            {
                auto chunk = (*it.value)[i];
                if (!chunk)
                    continue;

                Vec3f world_chunk_pos = {cast (f32) chunk->x * Chunk_Size, cast (f32) chunk->y * Chunk_Size, cast (f32) chunk->z * Chunk_Size};

                if (distance (world_chunk_pos, g_camera.position) < cast (f64) g_render_distance * Chunk_Size)
                {
                    chunk_generate_mesh_data (chunk);
                    draw_chunk (chunk, &g_camera);
                    g_drawn_vertex_count += chunk->vertex_count;
                }
            }
        }

        ImGui::Render ();
        ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());

        glfwSwapBuffers (g_window);
    }

    return 0;
}
