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

const int Atlas_Cell_Size_No_Border = 16;
const int Atlas_Cell_Size = Atlas_Cell_Size_No_Border + 2;

const int Block_Face_East  = 0; // +X
const int Block_Face_West  = 1; // -X
const int Block_Face_Above = 2; // +Y
const int Block_Face_Below = 3; // -Y
const int Block_Face_North = 4; // +Z
const int Block_Face_South = 5; // -Z

const int Block_Corner_Top_Left     = 0;
const int Block_Corner_Top_Right    = 1;
const int Block_Corner_Bottom_Left  = 2;
const int Block_Corner_Bottom_Right = 3;

out vec3 Normal;
out vec2 Tex_Coords;

uniform mat4 u_View_Projection_Matrix;
uniform sampler2D u_Texture_Atlas;

void main ()
{
    gl_Position = u_View_Projection_Matrix * vec4 (a_Position, 1);

    switch (a_Face)
    {
    case Block_Face_East:  Normal = vec3 ( 1, 0, 0); break;
    case Block_Face_West:  Normal = vec3 (-1, 0, 0); break;
    case Block_Face_Above: Normal = vec3 (0,  1, 0); break;
    case Block_Face_Below: Normal = vec3 (0, -1, 0); break;
    case Block_Face_North: Normal = vec3 (0, 0,  1); break;
    case Block_Face_South: Normal = vec3 (0, 0, -1); break;
    }

    int atlas_size = textureSize (u_Texture_Atlas, 0).x / Atlas_Cell_Size;    // Assume width == height
    int atlas_cell_x = a_Block_Id % atlas_size;
    int atlas_cell_y = a_Block_Id / atlas_size;
    int atlas_tex_x = atlas_cell_x * Atlas_Cell_Size + 1;
    int atlas_tex_y = atlas_cell_y * Atlas_Cell_Size + 1;

    switch (a_Block_Corner)
    {
    case Block_Corner_Top_Left:     break;
    case Block_Corner_Top_Right:    atlas_tex_x += Atlas_Cell_Size_No_Border; break;
    case Block_Corner_Bottom_Left:  atlas_tex_y += Atlas_Cell_Size_No_Border; break;
    case Block_Corner_Bottom_Right: atlas_tex_x += Atlas_Cell_Size_No_Border; atlas_tex_y += Atlas_Cell_Size_No_Border; break;
    }

    Tex_Coords.x = float (atlas_tex_x) / float (atlas_size * Atlas_Cell_Size);
    Tex_Coords.y = float (atlas_tex_y) / float (atlas_size * Atlas_Cell_Size);
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
    vec3 light_direction = normalize (vec3 (0.5, 1, 0.2));
    vec4 sampled = texture (u_Texture_Atlas, Tex_Coords);
    Frag_Color = sampled * max (dot (Normal, light_direction), 0.25);
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

static const int Atlas_Cell_Size = 18;
static const int Atlas_Cell_Size_No_Border = Atlas_Cell_Size - 2;

void copy_row_into_texture_atlas (u32 *dest, u32 *src, int dest_row, int dest_x, int src_row, int src_width)
{
    dest[dest_row * g_texture_atlas_size + dest_x] = src[src_row * src_width];
    memcpy (dest + dest_row * g_texture_atlas_size + dest_x + 1, src + src_row * src_width, src_width * sizeof (s32));
    dest[dest_row * g_texture_atlas_size + dest_x + Atlas_Cell_Size - 1] = src[src_row * src_width + src_width - 1];
}

bool load_texture_atlas (const char *textures_dirname)
{
    static const char *Texture_Names[] = {
        "dirt.png",
        "stone.png",
        "bedrock.png",
    };

    static const int Texture_Count = array_size (Texture_Names);

    int atlas_cell_count = cast (int) ceil (sqrt (Texture_Count + 1));
    // We add 2 pixels to apply a border to prevent seams from appearing
    // when we render the textured blocks
    g_texture_atlas_size = Atlas_Cell_Size * atlas_cell_count;
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

        if (w != Atlas_Cell_Size_No_Border || h != Atlas_Cell_Size_No_Border)
        {
            println ("Error: texture %s dimensions are invalid. All textures must be %d by %d", Texture_Names[i], Atlas_Cell_Size_No_Border, Atlas_Cell_Size_No_Border);
            return false;
        }

        int block_id = i + 1;   // Leave one for air
        int cell_x = block_id % atlas_cell_count;
        int cell_y = block_id / atlas_cell_count;
        int tex_x = cell_x * Atlas_Cell_Size;
        int tex_y = cell_y * Atlas_Cell_Size;

        copy_row_into_texture_atlas (atlas_data, data, tex_y, tex_x, 0, w);

        for_range (row, 0, h)
            copy_row_into_texture_atlas (atlas_data, data, tex_y + 1 + row, tex_x, row, w);

        copy_row_into_texture_atlas (atlas_data, data, tex_y + Atlas_Cell_Size - 1, tex_x, h - 1, w);
    }

    glGenTextures (1, &g_texture_atlas);
    glBindTexture (GL_TEXTURE_2D, g_texture_atlas);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, g_texture_atlas_size, g_texture_atlas_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas_data);
    glBindTexture (GL_TEXTURE_2D, 0);

    return true;
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
        auto chunk = *it.value;

        Vec2f camera_planar_pos = {camera->position.x, camera->position.z};
        Vec2f world_chunk_pos = {cast (f32) chunk->x * Chunk_Size, cast (f32) chunk->z * Chunk_Size};

        if (distance (world_chunk_pos, camera_planar_pos) < cast (f64) g_render_distance * Chunk_Size)
        {
            chunk_generate_mesh_data (chunk);
            chunk_draw (chunk, &g_camera);
            g_drawn_vertex_count += chunk->vertex_count;
        }
    }
}
