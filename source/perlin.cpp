#include "Minecraft.hpp"

// @Note: we might want to randomly generate this
static
const int Perlin_Permutation_Table[512] = {
    151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    // We repeat the numbers to avoid buffer overflows
    151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

inline
f64 perlin_fade (f64 t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

inline
f64 perlin_gradient (int hash, f64 x, f64 y)
{
    switch (hash & 0x3)
    {
    case 0x0: return  x + y;
    case 0x1: return -x + y;
    case 0x2: return -x - y;
    case 0x3: return  x - y;
    default:  return 0;
    }
}

f64 perlin_noise (f64 x, f64 y)
{
#define P Perlin_Permutation_Table

    int xi = (cast (int) floor (x)) & 255;
    int yi = (cast (int) floor (y)) & 255;
    f64 xf = x - floor (x);
    f64 yf = y - floor (y);

    auto u = perlin_fade (xf);
    auto v = perlin_fade (yf);

    int aa = P[P[xi    ] + yi    ];
    int ab = P[P[xi    ] + yi + 1];
    int ba = P[P[xi + 1] + yi    ];
    int bb = P[P[xi + 1] + yi + 1];

    f64 x1, x2;

    x1 = lerp (
        perlin_gradient (aa, xf    , yf),
        perlin_gradient (ba, xf - 1, yf),
        u
    );

    x2 = lerp (
        perlin_gradient (ab, xf    , yf - 1),
        perlin_gradient (bb, xf - 1, yf - 1),
        u
    );

    return lerp (x1, x2, v);

#undef P
}

inline
f64 perlin_gradient (int hash, f64 x, f64 y, f64 z)
{
    switch (hash & 0xf)
    {
    case 0x0: return  x + y;
    case 0x1: return -x + y;
    case 0x2: return  x - y;
    case 0x3: return -x - y;
    case 0x4: return  x + z;
    case 0x5: return -x + z;
    case 0x6: return  x - z;
    case 0x7: return -x - z;
    case 0x8: return  y + z;
    case 0x9: return -y + z;
    case 0xa: return  y - z;
    case 0xb: return -y - z;
    case 0xc: return  y + x;
    case 0xd: return -y + z;
    case 0xe: return  y - x;
    case 0xf: return -y - z;
    default:  return 0;
    }
}

f64 perlin_noise (f64 x, f64 y, f64 z)
{
#define P Perlin_Permutation_Table

    int xi = (cast (int) floor (x)) & 255;
    int yi = (cast (int) floor (y)) & 255;
    int zi = (cast (int) floor (z)) & 255;
    f64 xf = x - floor (x);
    f64 yf = y - floor (y);
    f64 zf = z - floor (z);

    auto u = perlin_fade (xf);
    auto v = perlin_fade (yf);
    auto w = perlin_fade (zf);

    int aaa = P[P[P[xi    ] + yi    ] + zi    ];
    int aba = P[P[P[xi    ] + yi + 1] + zi    ];
    int aab = P[P[P[xi    ] + yi    ] + zi + 1];
    int abb = P[P[P[xi    ] + yi + 1] + zi + 1];
    int baa = P[P[P[xi + 1] + yi    ] + zi    ];
    int bba = P[P[P[xi + 1] + yi + 1] + zi    ];
    int bab = P[P[P[xi + 1] + yi    ] + zi + 1];
    int bbb = P[P[P[xi + 1] + yi + 1] + zi + 1];

    f64 x1, x2, y1, y2;

    x1 = lerp (
        perlin_gradient (aaa, xf    , yf, zf),
        perlin_gradient (baa, xf - 1, yf, zf),
        u
    );

    x2 = lerp (
        perlin_gradient (aba, xf    , yf - 1, zf),
        perlin_gradient (bba, xf - 1, yf - 1, zf),
        u
    );

    y1 = lerp (x1, x2, v);

    x1 = lerp (
        perlin_gradient (aab, xf    , yf, zf - 1),
        perlin_gradient (bab, xf - 1, yf, zf - 1),
        u
    );

    x2 = lerp (
        perlin_gradient (abb, xf    , yf - 1, zf - 1),
        perlin_gradient (bbb, xf - 1, yf - 1, zf - 1),
        u
    );

    y2 = lerp (x1, x2, v);

    return lerp (y1, y2, w);

#undef P
}

static const f64 Perlin_Fractal_Min_Amplitude = 0.00001;

inline
f64 perlin_fractal_max (int octaves, f64 persistance)
{
    f64 result = 0;
    f64 amplitude = 1;
    for (int i = 0; i < octaves && amplitude > Perlin_Fractal_Min_Amplitude; i += 1)
    {
        result += amplitude;
        amplitude *= persistance;
    }

    return result;
}

inline
f64 perlin_fractal_noise (f64 scale, int octaves, Vec2f *offsets, f64 persistance, f64 lacunarity, f64 x, f64 y)
{
    if (octaves > Perlin_Fractal_Max_Octaves)
        octaves = Perlin_Fractal_Max_Octaves;

    f64 result = 0;
    f64 amplitude = 1;
    f64 frequency = 1;
    for (int i = 0; i < octaves && amplitude > Perlin_Fractal_Min_Amplitude; i += 1)
    {
        result += perlin_noise (
            x * scale * frequency + offsets[i].x,
            y * scale * frequency + offsets[i].y
        ) * amplitude;
        amplitude *= persistance;
        frequency *= lacunarity;
    }

    return result;
}

inline
f64 perlin_fractal_noise (Perlin_Fractal_Params params, Vec2f *offsets, f64 x, f64 y)
{
    return perlin_fractal_noise (params.scale, params.octaves, offsets, params.persistance, params.lacunarity, x, y);
}

inline
f64 perlin_fractal_noise (f64 scale, int octaves, Vec3f *offsets, f64 persistance, f64 lacunarity, f64 x, f64 y, f64 z)
{
    if (octaves > Perlin_Fractal_Max_Octaves)
        octaves = Perlin_Fractal_Max_Octaves;

    f64 result = 0;
    f64 amplitude = 1;
    f64 frequency = 1;
    for (int i = 0; i < octaves && amplitude > Perlin_Fractal_Min_Amplitude; i += 1)
    {
        result += perlin_noise (
            x * scale * frequency + offsets[i].x,
            y * scale * frequency + offsets[i].y,
            z * scale * frequency + offsets[i].z
        ) * amplitude;
        amplitude *= persistance;
        frequency *= lacunarity;
    }

    return result;
}

inline
f64 perlin_fractal_noise (Perlin_Fractal_Params params, Vec3f *offsets, f64 x, f64 y, f64 z)
{
    return perlin_fractal_noise (params.scale, params.octaves, offsets, params.persistance, params.lacunarity, x, y, z);
}

void perlin_generate_offsets (LC_RNG *rng, int count, Vec2f *offsets)
{
    for_range (i, 0, count)
    {
        offsets[i].x = random_rangef (rng, -10000, 10000);
        offsets[i].y = random_rangef (rng, -10000, 10000);
    }
}

void perlin_generate_offsets (LC_RNG *rng, int count, Vec3f *offsets)
{
    for_range (i, 0, count)
    {
        offsets[i].x = random_rangef (rng, -10000, 10000);
        offsets[i].y = random_rangef (rng, -10000, 10000);
        offsets[i].z = random_rangef (rng, -10000, 10000);
    }
}
