#include "Minecraft.hpp"

Arena     frame_arena;
Allocator frame_allocator;

Vec2f g_prev_mouse_pos;
Vec2f g_curr_mouse_pos;
Vec2f g_mouse_delta;

GLFWwindow *g_window;

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

    static Chunk chunk;
    chunk_init (&chunk, 0, 0, 0);

    for_range (x, 0, Chunk_Size)
    {
        for_range (y, 0, Chunk_Size)
        {
            for_range (z, 0, Chunk_Size)
            {
                s64 i = x * Chunk_Size * Chunk_Size + y * Chunk_Size + z;
                if (perlin_noise (cast (f64) x * 0.1, cast (f64) y * 0.1, cast (f64) z * 0.1) < 0.5)
                    chunk.blocks[i].type = cast (Block_Type) Block_Type_Air;
                else
                    chunk.blocks[i].type = cast (Block_Type) Block_Type_Stone;
            }
        }
    }

    Camera camera = {};
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

        ImGui_ImplOpenGL3_NewFrame ();
        ImGui_ImplGlfw_NewFrame ();
        ImGui::NewFrame ();

        if (show_demo_window)
           ImGui::ShowDemoWindow (&show_demo_window);

        if (show_metrics_window)
        {
            if (ImGui::Begin ("Metrics", &show_metrics_window))
            {
                ImGui::Text ("Vertices: %lld", chunk.vertices.count);
            }
            ImGui::End ();
        }

        show_perlin_test_window (&show_perlin_test);

        int width, height;
        glfwGetFramebufferSize (g_window, &width, &height);
        glViewport (0, 0, width, height);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        chunk_generate_mesh_data (&chunk);
        draw_chunk (&chunk, &camera);

        ImGui::Render ();
        ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());

        glfwSwapBuffers (g_window);
    }

    return 0;
}
