#include "Minecraft.hpp"

GLuint g_block_shader;
GLuint g_texture_atlas;
s64 g_texture_atlas_size;
s64 g_atlas_cell_count;

static const int Atlas_Cell_Size_No_Border = 16;
static const int Atlas_Cell_Border_Size = 4;
static const int Atlas_Cell_Size = Atlas_Cell_Size_No_Border + Atlas_Cell_Border_Size * 2;

const char *GL_Block_Shader_Header = R"""(
#version 330 core

const int Atlas_Cell_Size_No_Border = %d;
const int Atlas_Cell_Border_Size = %d;
const int Atlas_Cell_Size = Atlas_Cell_Size_No_Border + Atlas_Cell_Border_Size * 2;
const int Atlas_Cell_Count = %d;
)""";

const char *GL_Block_Shader_Vertex = R"""(
layout (location = 0) in vec3 a_Position;
layout (location = 1) in int a_Face;
layout (location = 2) in int a_Block_Id;
layout (location = 3) in int a_Block_Corner;

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
centroid out vec2 Tex_Coords;

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

    int atlas_cell_x = a_Block_Id % Atlas_Cell_Count;
    int atlas_cell_y = a_Block_Id / Atlas_Cell_Count;
    int atlas_tex_x = atlas_cell_x * Atlas_Cell_Size + Atlas_Cell_Border_Size;
    int atlas_tex_y = atlas_cell_y * Atlas_Cell_Size + Atlas_Cell_Border_Size;

    switch (a_Block_Corner)
    {
    case Block_Corner_Top_Left:     break;
    case Block_Corner_Top_Right:    atlas_tex_x += Atlas_Cell_Size_No_Border; break;
    case Block_Corner_Bottom_Left:  atlas_tex_y += Atlas_Cell_Size_No_Border; break;
    case Block_Corner_Bottom_Right: atlas_tex_x += Atlas_Cell_Size_No_Border; atlas_tex_y += Atlas_Cell_Size_No_Border; break;
    }

    ivec2 atlas_size = textureSize (u_Texture_Atlas, 0);
    Tex_Coords.x = float (atlas_tex_x) / float (atlas_size.x);
    Tex_Coords.y = float (atlas_tex_y) / float (atlas_size.y);
}
)""";

const char *GL_Block_Shader_Fragment = R"""(
in vec3 Normal;
centroid in vec2 Tex_Coords;

out vec4 Frag_Color;

uniform sampler2D u_Texture_Atlas;

void main ()
{
    vec3 light_direction = normalize (vec3 (0.5, 1, 0.2));
    vec4 sampled = texture (u_Texture_Atlas, Tex_Coords);
    // vec4 sampled = textureLod (u_Texture_Atlas, Tex_Coords, 1);
    Frag_Color.rgb = sampled.rgb * max (dot (Normal, light_direction), 0.25);
    Frag_Color.a = sampled.a;
}
)""";

bool render_init (const char *textures_dirname)
{
    if (!load_texture_atlas (textures_dirname))
    {
        println ("Error: could not load textures");
        return false;
    }

    GLuint vertex_shader = glCreateShader (GL_VERTEX_SHADER);
    defer (glDeleteShader (vertex_shader));

    GLuint fragment_shader = glCreateShader (GL_FRAGMENT_SHADER);
    defer (glDeleteShader (fragment_shader));

    int status;
    char info_log[4096];

    const char *shader_header = fcstring (frame_allocator, GL_Block_Shader_Header, Atlas_Cell_Size_No_Border, Atlas_Cell_Border_Size, g_atlas_cell_count);
    const char *vertex_shader_source = fcstring (frame_allocator, "%s\n%s", shader_header, GL_Block_Shader_Vertex);
    const char *fragment_shader_source = fcstring (frame_allocator, "%s\n%s", shader_header, GL_Block_Shader_Fragment);

    glShaderSource (vertex_shader, 1, &vertex_shader_source, null);
    glCompileShader (vertex_shader);
    glGetShaderiv (vertex_shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        glGetShaderInfoLog (vertex_shader, sizeof (info_log), null, info_log);
        println ("GL Error: could not compile vertex shader.\n%s", info_log);

        return false;
    }

    glShaderSource (fragment_shader, 1, &fragment_shader_source, null);
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

u32 image_sample_averaged (const Image &tex, int sample_size, f32 x, f32 y)
{
    x = clamp (x, 0.0f, 1.0f);
    y = clamp (y, 0.0f, 1.0f);

    x *= tex.width;
    y *= tex.height;

    int r = 0;
    int g = 0;
    int b = 0;
    int a = 0;
    for_range (sy, 0, sample_size)
    {
        for_range (sx, 0, sample_size)
        {
            int ix = cast (int) x + sx - sample_size / 2;
            int iy = cast (int) y + sy - sample_size / 2;
            ix = clamp (ix, 0, tex.width - 1);
            iy = clamp (iy, 0, tex.height - 1);

            u32 val = tex.data[iy * tex.width + ix];
            int sample_a = cast (f32) ((val >> 24) & 0xff);
            int sample_b = cast (f32) ((val >> 16) & 0xff);
            int sample_g = cast (f32) ((val >>  8) & 0xff);
            int sample_r = cast (f32) ((val >>  0) & 0xff);

            r += sample_r;
            g += sample_g;
            b += sample_b;
            a += sample_a;
        }
    }

    r /= sample_size * sample_size;
    g /= sample_size * sample_size;
    b /= sample_size * sample_size;
    a /= sample_size * sample_size;

    u32 val =
          (cast (u32) a << 24)
        | (cast (u32) b << 16)
        | (cast (u32) g <<  8)
        | (cast (u32) r <<  0);

    return val;
}

inline
u32 image_get_pixel (const Image *img, int x, int y)
{
    assert (x >= 0 && x < img->width, "Texture index out of bounds (got %d, expected [0;%d])", x, img->width - 1);
    assert (y >= 0 && y < img->height, "Texture index out of bounds (got %d, expected [0;%d])", y, img->height - 1);

    return img->data[y * img->width + x];
}

inline
void image_set_pixel (Image *img, int x, int y, u32 val)
{
    assert (x >= 0 && x < img->width, "Texture index out of bounds (got %d, expected [0;%d])", x, img->width - 1);
    assert (y >= 0 && y < img->height, "Texture index out of bounds (got %d, expected [0;%d])", y, img->height - 1);

    img->data[y * img->width + x] = val;
}

void copy_image_to_atlas (Image *atlas, const Image &tex, int level, int tex_x, int tex_y, int cell_size_no_border)
{
    int cell_size = cell_size_no_border + Atlas_Cell_Border_Size * 2;
    int sample_size = cast (int) powf (2, cast (f32) level);

    tex_x += Atlas_Cell_Border_Size;
    tex_y += Atlas_Cell_Border_Size;

    for_range (y, -Atlas_Cell_Border_Size, cell_size_no_border + Atlas_Cell_Border_Size)
    {
        for_range (x, -Atlas_Cell_Border_Size, cell_size_no_border + Atlas_Cell_Border_Size)
        {
            f32 sample_x = x / cast (f32) cell_size_no_border;
            f32 sample_y = y / cast (f32) cell_size_no_border;

            u32 val = image_sample_averaged (tex, sample_size, sample_x, sample_y);
            image_set_pixel (atlas, tex_x + x, tex_y + y, val);
        }
    }
}

void generate_atlas_mipmap (Image *atlas, const Slice<Image> &textures, int level, int cell_size_no_border, int atlas_cell_count)
{
    int cell_size = cell_size_no_border + Atlas_Cell_Border_Size * 2;

    memset (atlas->data, 0, atlas->width * atlas->height * sizeof (u32));

    for_array (i, textures)
    {
        int block_id = i + 1;   // Leave one for air
        int cell_x = block_id % atlas_cell_count;
        int cell_y = block_id / atlas_cell_count;

        int tex_x = cell_x * cell_size;
        int tex_y = cell_y * cell_size;

        copy_image_to_atlas (atlas, textures[i], level, tex_x, tex_y, cell_size_no_border);
    }

    glTexImage2D (GL_TEXTURE_2D, level, GL_RGBA, atlas->width, atlas->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas->data);
}

bool load_texture_atlas (const char *textures_dirname)
{
    static const char *Texture_Names[] = {
        "dirt.png",
        "stone.png",
        "bedrock.png",
        "water.png",
        "five.png",
        "six.png",
        "seven.png",
        "eight.png",
    };

    static const int Texture_Count = array_size (Texture_Names);

    g_atlas_cell_count = cast (int) ceil (sqrt (Texture_Count + 1));
    g_texture_atlas_size = Atlas_Cell_Size * g_atlas_cell_count;

    u32 *atlas_data = mem_alloc_typed (u32, g_texture_atlas_size * g_texture_atlas_size, heap_allocator ());
    defer (mem_free (atlas_data, heap_allocator ()));

    Image textures[Texture_Count] = {};
    defer (
        for_range (i, 0, Texture_Count)
            stbi_image_free (textures[i].data);
    );

    glGenTextures (1, &g_texture_atlas);
    glBindTexture (GL_TEXTURE_2D, g_texture_atlas);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, Atlas_Cell_Border_Size - 1);

    for_range (i, 0, Texture_Count)
    {
        int w, h;
        const char *filename = fcstring (frame_allocator, "%s/%s", textures_dirname, Texture_Names[i]);
        textures[i].data = cast (u32 *) stbi_load (filename, &w, &h, null, 4);
        textures[i].width = w;
        textures[i].height = h;

        if (!textures[i].data)
        {
            println ("Error: could not load texture %s", Texture_Names[i]);
            return false;
        }

        if (w != Atlas_Cell_Size_No_Border || h != Atlas_Cell_Size_No_Border)
        {
            println ("Error: texture %s dimensions are invalid. All textures must be %d by %d", Texture_Names[i], Atlas_Cell_Size_No_Border, Atlas_Cell_Size_No_Border);
            return false;
        }
    }

    Image mipmap;
    mipmap.width  = g_texture_atlas_size;
    mipmap.height = g_texture_atlas_size;
    mipmap.data = atlas_data;
    generate_atlas_mipmap (&mipmap, slice_make (Texture_Count, textures), 0, Atlas_Cell_Size_No_Border, g_atlas_cell_count);

    // It seems auto generating the mipmaps is fine for up to a certain level with a certain border size,
    // so we do that for now. We may manually generate them in the future like we started if it turns out
    // to not work fine.
    glGenerateMipmap (GL_TEXTURE_2D);
    glBindTexture (GL_TEXTURE_2D, 0);

    println ("Texture atlas size: %i cells, %i x %i pixels", g_atlas_cell_count, g_texture_atlas_size, g_texture_atlas_size);

    return true;
}

void chunk_draw (Chunk *chunk, Camera *camera, Chunk_Mesh_Type mesh_type)
{
    if (chunk->vertex_counts[mesh_type] == 0)
        return;

    glBindVertexArray (chunk->opengl_is_stupid_vaos[mesh_type]);
    glBindBuffer (GL_ARRAY_BUFFER, chunk->gl_vbos[mesh_type]);

    glDrawArrays (GL_TRIANGLES, 0, chunk->vertex_counts[mesh_type]);

    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindVertexArray (0);
}

void world_draw_chunks (World *world, Camera *camera)
{
    Array<Chunk *> chunks_to_draw;
    array_init (&chunks_to_draw, frame_allocator);

    g_drawn_vertex_count = 0;
    for_hash_map (it, world->all_loaded_chunks)
    {
        auto chunk = *it.value;

        Vec2f camera_planar_pos = {camera->position.x, camera->position.z};
        Vec2f world_chunk_pos = {cast (f32) chunk->x * Chunk_Size, cast (f32) chunk->z * Chunk_Size};

        if (distance (world_chunk_pos, camera_planar_pos) < cast (f64) g_render_distance * Chunk_Size)
        {
            chunk_generate_mesh_data (chunk);
            array_push (&chunks_to_draw, chunk);
            g_drawn_vertex_count += chunk->total_vertex_count;
        }
    }

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture (0);
    glBindTexture (GL_TEXTURE_2D, g_texture_atlas);

    glUseProgram (g_block_shader);

    auto loc = glGetUniformLocation (g_block_shader, "u_View_Projection_Matrix");
    glUniformMatrix4fv (loc, 1, GL_TRUE, camera->view_projection_matrix.comps);

    loc = glGetUniformLocation (g_block_shader, "u_Texture_Atlas");
    glUniform1i (loc, 0);

    for_range (mesh_type, 0, Chunk_Mesh_Count)
    {
        for_array (i, chunks_to_draw)
            chunk_draw (chunks_to_draw[i], &g_camera, cast (Chunk_Mesh_Type) mesh_type);
    }
}
