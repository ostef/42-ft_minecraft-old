#include "Minecraft.hpp"

GLuint g_block_shader;
GLuint g_texture_atlas;

const char *GL_Block_Shader_Vertex = R"""(
#version 330 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_Tex_Coords;

out vec3 Normal;
out vec2 Tex_Coords;

uniform mat4 u_View_Projection_Matrix;

void main ()
{
    gl_Position = u_View_Projection_Matrix * vec4 (a_Position, 1);
    Normal      = a_Normal;
    Tex_Coords  = a_Tex_Coords;
}
)""";

const char *GL_Block_Shader_Fragment = R"""(
#version 330 core

in vec3 Normal;
in vec2 Tex_Coords;

out vec4 Frag_Color;

void main ()
{
    vec3 light_direction = normalize (vec3 (1, 1, 1));
    Frag_Color.rgb = Normal; //vec3 (1, 1, 1) * max (dot (light_direction, Normal), 0);
    Frag_Color.a = 1;
}
)""";

bool render_init ()
{
    GLuint vertex_shader = glCreateShader (GL_VERTEX_SHADER);
    defer (glDeleteShader (vertex_shader));

    GLuint fragment_shader = glCreateShader (GL_FRAGMENT_SHADER);
    defer (glDeleteShader (fragment_shader));

    int status;
    char info_log[4096];

    glShaderSource (vertex_shader, 1, &GL_Block_Shader_Vertex, null);
    glCompileShader (vertex_shader);
    glGetShaderiv (vertex_shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        glGetShaderInfoLog (vertex_shader, sizeof (info_log), null, info_log);
        println ("GL Error: could not compile vertex shader.\n%s", info_log);

        return false;
    }

    glShaderSource (fragment_shader, 1, &GL_Block_Shader_Fragment, null);
    glCompileShader (fragment_shader);
    glGetShaderiv (fragment_shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        glGetShaderInfoLog (fragment_shader, sizeof (info_log), null, info_log);
        println ("GL Error: could not compile fragment shader.\n%s", info_log);

        return false;
    }

    g_block_shader = glCreateProgram ();
    glAttachShader (g_block_shader, vertex_shader);
    glAttachShader (g_block_shader, fragment_shader);
    glLinkProgram (g_block_shader);
    glGetProgramiv (g_block_shader, GL_LINK_STATUS, &status);
    if (!status)
    {
        glGetProgramInfoLog (g_block_shader, sizeof (info_log), null, info_log);
        println ("GL Error: could not link shader program.\n%s", info_log);

        return false;
    }

    return true;
}

bool load_texture_atlas (const char *textures_dirname)
{
    static const char *Texture_Names[] = {
        "dirt.png",
        "bedrock.png",
        "stone.png",
    };

    static const int Texture_Count = array_size (Texture_Names);

    int atlas_cell_size = cast (int) ceil (sqrt (Texture_Count));
    int atlas_size = 16 * atlas_cell_size;
    u32 *atlas_data = mem_alloc_typed (u32, atlas_size * atlas_size, heap_allocator ());
    defer (mem_free (atlas_data, heap_allocator ()));

    for_range (i, 0, Texture_Count)
    {
        int w, h;
        auto data = stbi_load (fcstring (frame_allocator, "%s/%s", textures_dirname, Texture_Names[i]), &w, &h, null, 4);

        if (!data)
        {
            println ("Error: could not load texture %s", Texture_Names[i]);
            return false;
        }

        defer (stbi_image_free (data));

        if (w != 16 || h != 16)
        {
            println ("Error: texture %s dimensions are invalid. All textures must be 16 by 16", Texture_Names[i]);
            return false;
        }

        int cell_x = i % atlas_cell_size;
        int cell_y = i / atlas_cell_size;

        for_range (row, 0, 16)
        {
            memcpy (atlas_data + (cell_y * 16 + row) * atlas_size + cell_x, data + row * 16, 16 * 4);
        }
    }

    glGenTextures (1, &g_texture_atlas);
    glBindTexture (GL_TEXTURE_2D, g_texture_atlas);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, atlas_size, atlas_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas_data);
    glBindTexture (GL_TEXTURE_2D, 0);

    return true;
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
        move_input.z = cast (f32) (glfwGetKey (g_window, GLFW_KEY_W) == GLFW_PRESS)
            - cast (f32) (glfwGetKey (g_window, GLFW_KEY_S) == GLFW_PRESS);
        move_input.y = cast (f32) (glfwGetKey (g_window, GLFW_KEY_E) == GLFW_PRESS)
            - cast (f32) (glfwGetKey (g_window, GLFW_KEY_Q) == GLFW_PRESS);
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

    camera->transform = mat4_transform<f32> (camera->position, camera->rotation);
    camera->view_matrix = inverse (camera->transform);

    int viewport_w, viewport_h;
    glfwGetFramebufferSize (g_window, &viewport_w, &viewport_h);
    f32 aspect_ratio = viewport_w / cast (f32) viewport_h;
    camera->projection_matrix = mat4_perspective_projection<f32> (camera->fov, aspect_ratio, 0.01, 1000.0);
    camera->view_projection_matrix = camera->projection_matrix * camera->view_matrix;
}

void draw_chunk (Chunk *chunk, Camera *camera)
{
    if (chunk->vertex_count == 0)
        return;

    glUseProgram (g_block_shader);
    auto loc = glGetUniformLocation (g_block_shader, "u_View_Projection_Matrix");
    glUniformMatrix4fv (loc, 1, GL_TRUE, camera->view_projection_matrix.comps);

    glBindVertexArray (chunk->opengl_is_stupid_vao);
    glBindBuffer (GL_ARRAY_BUFFER, chunk->gl_vbo);

    glDrawArrays (GL_TRIANGLES, 0, chunk->vertex_count);

    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindVertexArray (0);
}
