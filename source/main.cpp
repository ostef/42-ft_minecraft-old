#include "Minecraft.hpp"

Arena     frame_arena;
Allocator frame_allocator;

void glfw_error_callback (int error, const char *description)
{
	fprintln (stderr, "GLFW Error (%d): %s", error, description);
}

int main (int argc, const char **args)
{
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

	auto window = glfwCreateWindow (1280, 720, "ft_minecraft", null, null);
	if (!window)
		return 1;

	defer (glfwDestroyWindow (window));

	glfwMakeContextCurrent (window);
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
	}

	ImGui_ImplGlfw_InitForOpenGL (window, true);
	ImGui_ImplOpenGL3_Init ("#version 130");

	glEnable (GL_MULTISAMPLE);
	glClearColor (0.2, 0.3, 0.6, 1.0);	// Sky color

	bool show_demo_window = true;
	while (!glfwWindowShouldClose (window))
	{
		arena_reset (&frame_arena);

		glfwPollEvents ();

		ImGui_ImplOpenGL3_NewFrame ();
		ImGui_ImplGlfw_NewFrame ();
		ImGui::NewFrame ();

		if (show_demo_window)
			ImGui::ShowDemoWindow (&show_demo_window);

		ImGui::Render ();
		int width, height;
		glfwGetFramebufferSize (window, &width, &height);
		glViewport (0, 0, width, height);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());

		glfwSwapBuffers (window);
	}

	return 0;
}
