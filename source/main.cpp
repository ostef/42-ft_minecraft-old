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
s64 g_delta_time = 0;

bool g_generate_new_chunks = true;
int g_render_distance = 4;

bool g_show_ui = true;

f32 hermite_cubic_calculate (const Nested_Hermite_Spline *spline, const Slice<f32> &t_values)
{
    f32 t = t_values[spline->t_value_index];

    if (spline->knots.count == 0)
        return 0;

    // @Speed @Todo: test different searching algorithms
    int index;
    for (index = 0; index < spline->knots.count; index += 1)
    {
        if (spline->knots[index].x >= t)
            break;
    }

    // Special case if t is outside of the range of the spline
    if (index == 0 || index == spline->knots.count)
    {
        if (index != 0)
            index -= 1;

        auto knot = spline->knots[index];
        f32 y = hermite_knot_value (knot, t_values);

        // Linearly extend the spline
        return y + knot.derivative * (t - knot.x);
    }

    auto k0 = spline->knots[index - 1];
    auto k1 = spline->knots[index];

    return hermite_cubic_calculate (
        k0.x, hermite_knot_value (k0, t_values), k0.derivative,
        k1.x, hermite_knot_value (k1, t_values), k1.derivative,
        inverse_lerp (k0.x, k1.x, t)
    );
}

void glfw_error_callback (int error, const char *description)
{
    println ("GLFW Error (%d): %s", error, description);
}

void glfw_key_callback (GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        g_show_ui = !g_show_ui;
        if (g_show_ui)
            glfwSetInputMode (window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else
            glfwSetInputMode (window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

void update_flying_camera (Camera *camera)
{
    Vec2f mouse_delta = {};
    Vec3f move_input = {};
    f32 move_speed = 0;

    if (glfwGetWindowAttrib (g_window, GLFW_FOCUSED))
    {
        move_speed = 0.1;
        if (glfwGetKey (g_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            move_speed *= 10;
        mouse_delta = g_mouse_delta;
        move_input.x = cast (f32) (glfwGetKey (g_window, GLFW_KEY_D) == GLFW_PRESS)
            - cast (f32) (glfwGetKey (g_window, GLFW_KEY_A) == GLFW_PRESS);
        move_input.y = cast (f32) (glfwGetKey (g_window, GLFW_KEY_SPACE) == GLFW_PRESS)
            - cast (f32) (glfwGetKey (g_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);
        move_input.z = cast (f32) (glfwGetKey (g_window, GLFW_KEY_W) == GLFW_PRESS)
            - cast (f32) (glfwGetKey (g_window, GLFW_KEY_S) == GLFW_PRESS);
    }

    move_input = normalized (move_input);
    camera->position += right_vector (camera->transform) * move_input.x * move_speed
        + forward_vector (camera->transform) * move_input.y * move_speed
        + up_vector (camera->transform) * move_input.z * move_speed;

    camera->rotation_input = lerp (camera->rotation_input, mouse_delta, 0.3);
    auto delta = camera->rotation_input * 0.3f;
    camera->euler_angles.yaw   += to_rads (delta.x);
    camera->euler_angles.pitch += to_rads (delta.y);
    camera->euler_angles.pitch = clamp (camera->euler_angles.pitch, to_rads (-80.0f), to_rads (80.0f));
    camera->rotation = quat_from_euler_angles<f32> (camera->euler_angles);
}

void update_camera_matrices (Camera *camera)
{
    camera->transform = mat4_transform<f32> (camera->position, camera->rotation);
    camera->view_matrix = inverse (camera->transform);

    int viewport_w, viewport_h;
    glfwGetFramebufferSize (g_window, &viewport_w, &viewport_h);
    f32 aspect_ratio = viewport_w / cast (f32) viewport_h;

    camera->projection_matrix = mat4_perspective_projection<f32> (camera->fov, aspect_ratio, 0.01, 1000.0);
    camera->view_projection_matrix = camera->projection_matrix * camera->view_matrix;
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

    glfwSetKeyCallback (g_window, glfw_key_callback);

    if (g_show_ui)
        glfwSetInputMode (g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else
        glfwSetInputMode (g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    int gl_version = gladLoadGL (cast (GLADloadfunc) glfwGetProcAddress);
    println ("GL Version %d.%d", GLAD_VERSION_MAJOR (gl_version), GLAD_VERSION_MINOR (gl_version));

    IMGUI_CHECKVERSION ();
    ImGui::CreateContext ();
    auto io = &ImGui::GetIO ();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io->Fonts->AddFontFromFileTTF ("data/Roboto-Regular.ttf", 18);

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
        style->ItemSpacing.y = 6;
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

    if (!render_init ("data/textures"))
    {
        println ("Could not initialize rendering system");
        return 1;
    }

    Vec3f camera_position {};
    Vec3f camera_direction {};

    g_camera.position.y = cast (f32) Default_Height_Range.y;
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

    world_init (&g_world, cast (s32) time_current_monotonic ());

    while (!glfwWindowShouldClose (g_window))
    {
        s64 frame_start = time_current_monotonic ();

        arena_reset (&frame_arena);

        glfwPollEvents ();
        g_prev_mouse_pos = g_curr_mouse_pos;

        f64 mx, my;
        glfwGetCursorPos (g_window, &mx, &my);
        g_curr_mouse_pos = {cast (f32) mx, cast (f32) my};
        g_mouse_delta = g_curr_mouse_pos - g_prev_mouse_pos;

        if (!g_show_ui)
            update_flying_camera (&g_camera);
        update_camera_matrices (&g_camera);

        if (g_generate_new_chunks)
        {
            s64 camera_chunk_x = chunk_position_from_block_position (cast (s64) g_camera.position.x, cast (s64) g_camera.position.z).x;
            s64 camera_chunk_z = chunk_position_from_block_position (cast (s64) g_camera.position.x, cast (s64) g_camera.position.z).y;

            for_range (x, camera_chunk_x - g_render_distance, camera_chunk_x + g_render_distance)
            {
                for_range (z, camera_chunk_z - g_render_distance, camera_chunk_z + g_render_distance)
                {
                    Vec2f planar_camera_pos = Vec2f{g_camera.position.x, g_camera.position.z};
                    Vec2f chunk_pos = Vec2f{cast (f32) x * Chunk_Size, cast (f32) z * Chunk_Size};

                    if (distance (planar_camera_pos, chunk_pos) < g_render_distance * Chunk_Size)
                    {
                        s64 time_start = time_current_monotonic ();

                        auto current_chunk = world_create_chunk (&g_world, x, z);

                        if (!current_chunk->generated)
                        {
                            g_chunk_creation_time += time_current_monotonic () - time_start;
                            g_chunk_creation_samples += 1;

                            chunk_generate (&g_world, current_chunk);

                            s64 time_end = time_current_monotonic ();
                            g_chunk_generation_time += time_end - time_start;
                            g_chunk_generation_samples += 1;
                        }
                    }
                }
            }
        }

        int width, height;
        glfwGetFramebufferSize (g_window, &width, &height);

            ImGui_ImplOpenGL3_NewFrame ();
            ImGui_ImplGlfw_NewFrame ();
            ImGui::NewFrame ();

        if (g_show_ui)
        {
            ImGui::DockSpaceOverViewport (null, ImGuiDockNodeFlags_PassthruCentralNode);

            ui_show_windows ();
        }
        else
        {
            auto viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);

            ImGuiWindowFlags host_window_flags = 0;
            host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
            host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
            host_window_flags |= ImGuiWindowFlags_NoBackground;
            if (ImGui::Begin ("##overlay", null, host_window_flags))
            {
                ImGui::Text ("%.0f %.0f %.0f", g_camera.position.x, g_camera.position.y, g_camera.position.z);

                s64 x = cast (s64) g_camera.position.x;
                s64 z = cast (s64) g_camera.position.z;
                auto chunk = world_get_chunk_at_block_position (&g_world, x, z);
                if (chunk)
                {
                    auto pos = chunk_absolute_to_relative_coordinates (chunk, x, z);
                    auto values = chunk_get_terrain_values (chunk, pos.x, pos.y);
                    ImGui::Text ("C: %.3f E: %.3f PV: %.3f S: %.3f", values.noise[0], values.noise[1], values.noise[2], values.surface_level);
                }
            }
            ImGui::End ();
        }

        glViewport (0, 0, width, height);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        world_draw_chunks (&g_world, &g_camera);

        ImGui::Render ();

        ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());

        auto io = ImGui::GetIO ();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            auto backup_current_context = glfwGetCurrentContext ();
            ImGui::UpdatePlatformWindows ();
            ImGui::RenderPlatformWindowsDefault ();
            glfwMakeContextCurrent (backup_current_context);
        }

        glfwSwapBuffers (g_window);

        g_delta_time = time_current_monotonic () - frame_start;
    }

    return 0;
}
