#include "Minecraft.hpp"

GLuint g_block_shader;
GLuint g_texture_atlas;
s64 g_texture_atlas_size;

const char *GL_Block_Shader_Vertex = R"""(
#version 330 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in int a_Face;
layout (location = 2) in int a_Block_Id;
layout (location = 3) in int a_Block_Corner;

const int Atlas_Cell_Size = 16;

out vec3 Normal;
out vec2 Tex_Coords;

uniform mat4 u_View_Projection_Matrix;
uniform sampler2D u_Texture_Atlas;

void main ()
{
    gl_Position = u_View_Projection_Matrix * vec4 (a_Position, 1);

    switch (a_Face)
    {
    case 0: Normal = vec3 ( 1, 0, 0); break;
    case 1: Normal = vec3 (-1, 0, 0); break;
    case 2: Normal = vec3 (0,  1, 0); break;
    case 3: Normal = vec3 (0, -1, 0); break;
    case 4: Normal = vec3 (0, 0,  1); break;
    case 5: Normal = vec3 (0, 0, -1); break;
    }

    int atlas_size = textureSize (u_Texture_Atlas, 0).x / Atlas_Cell_Size;    // Assume width == height
    int atlas_cell_x = a_Block_Id % atlas_size;
    int atlas_cell_y = a_Block_Id / atlas_size;

    switch (a_Block_Corner)
    {
    case 0: break;
    case 1: atlas_cell_x += 1; break;
    case 2: atlas_cell_y -= 1; break;
    case 3: atlas_cell_x += 1; atlas_cell_y -= 1; break;
    }

    Tex_Coords.x = float (atlas_cell_x) / float (atlas_size);
    Tex_Coords.y = 1 - float (atlas_cell_y) / float (atlas_size);
}
)""";

const char *GL_Block_Shader_Fragment = R"""(
#version 330 core

in vec3 Normal;
in vec2 Tex_Coords;

out vec4 Frag_Color;

uniform sampler2D u_Texture_Atlas;

void main ()
{
    vec3 light_direction = normalize (vec3 (1, 1, 1));
    vec4 sampled = texture (u_Texture_Atlas, Tex_Coords);
    Frag_Color = sampled;// * max (dot (light_direction, Normal), 0.1);
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
        "stone.png",
        "bedrock.png",
    };

    static const int Texture_Count = array_size (Texture_Names);

    int atlas_cell_size = cast (int) ceil (sqrt (Texture_Count + 1));
    g_texture_atlas_size = 16 * atlas_cell_size;
    u32 *atlas_data = mem_alloc_typed (u32, g_texture_atlas_size * g_texture_atlas_size, heap_allocator ());
    defer (mem_free (atlas_data, heap_allocator ()));

    for_range (i, 0, Texture_Count)
    {
        int w, h;
        auto data = cast (u32 *) stbi_load (fcstring (frame_allocator, "%s/%s", textures_dirname, Texture_Names[i]), &w, &h, null, 4);

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

        int block_id = i + 1;   // Leave one for air
        int cell_x = block_id % atlas_cell_size;
        int cell_y = block_id / atlas_cell_size;

        for_range (row, 0, 16)
        {
            memcpy (atlas_data + (cell_y * 16 + row) * g_texture_atlas_size + cell_x * 16, data + row * 16, 16 * sizeof (s32));
        }
    }

    glGenTextures (1, &g_texture_atlas);
    glBindTexture (GL_TEXTURE_2D, g_texture_atlas);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, g_texture_atlas_size, g_texture_atlas_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas_data);
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

void chunk_draw (Chunk *chunk, Camera *camera)
{
    if (chunk->vertex_count == 0)
        return;

    glBindVertexArray (chunk->opengl_is_stupid_vao);
    glBindBuffer (GL_ARRAY_BUFFER, chunk->gl_vbo);

    glDrawArrays (GL_TRIANGLES, 0, chunk->vertex_count);

    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindVertexArray (0);
}

void world_draw_chunks (World *world, Camera *camera)
{
    glActiveTexture (0);
    glBindTexture (GL_TEXTURE_2D, g_texture_atlas);

    glUseProgram (g_block_shader);

    auto loc = glGetUniformLocation (g_block_shader, "u_View_Projection_Matrix");
    glUniformMatrix4fv (loc, 1, GL_TRUE, camera->view_projection_matrix.comps);

    loc = glGetUniformLocation (g_block_shader, "u_Texture_Atlas");
    glUniform1i (loc, 0);

    g_drawn_vertex_count = 0;
    for_hash_map (it, world->all_loaded_chunks)
    {
        for_range (i, 0, Max_Chunk_Y - Min_Chunk_Y)
        {
            auto chunk = (*it.value)[i];
            if (!chunk)
                continue;

            Vec3f world_chunk_pos = {cast (f32) chunk->x * Chunk_Size, cast (f32) chunk->y * Chunk_Size, cast (f32) chunk->z * Chunk_Size};

            if (distance (world_chunk_pos, camera->position) < cast (f64) g_render_distance * Chunk_Size)
            {
                chunk_generate_mesh_data (chunk);
                chunk_draw (chunk, &g_camera);
                g_drawn_vertex_count += chunk->vertex_count;
            }
        }
    }
}
