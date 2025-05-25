#ifndef RENDERER_TYPES_H
#define RENDERER_TYPES_H

#define RENDERER_DATA_IMG_LENGTH 2048
#define RENDERER_DATA_IMG_WIDTH_VEC4S 8
#define RENDERER_DATA_IMG_WIDTH_BYTES 128
#define RENDERER_VFLAG_NONE         (0)
#define RENDERER_VFLAG_IS_CEIL      (1 << 0)
#define RENDERER_VFLAG_SEG_BOTTOM   (1 << 1)
#define RENDERER_VFLAG_SEG_MIDDLE   (1 << 2)
#define RENDERER_VFLAG_SEG_TOP      (1 << 3)
#define RENDERER_VFLAG_PORTAL       (1 << 4)

#ifdef __STDC__
#include "util/math.h"

#define RENDER_DATA_BEGIN() union { struct {
#define RENDER_DATA_END() }; u8 _padding[RENDERER_DATA_IMG_WIDTH_BYTES]; };

#define STD140_VEC2 __attribute__((aligned(8))) vec2s
#define STD140_VEC3 __attribute__((aligned(16))) vec3s
#define STD140_VEC4 __attribute__((aligned(16))) vec4s
#define STD140_FLOAT __attribute__((aligned(4))) f32

#else

#define RENDER_DATA_BEGIN()
#define RENDER_DATA_END()

#define vec2s vec2
#define vec3s vec3
#define vec4s vec4
#define f32 float

#define STD140_VEC2 vec2
#define STD140_VEC3 vec3
#define STD140_VEC4 vec4
#define STD140_FLOAT float
#endif

struct side_render_data_t {
    RENDER_DATA_BEGIN()
    STD140_FLOAT z_floor;
    STD140_FLOAT z_ceil;
    STD140_FLOAT nz_floor;
    STD140_FLOAT nz_ceil;
    STD140_VEC2  origin;
    STD140_VEC2  offsets;
    STD140_FLOAT len;
    STD140_FLOAT split_bottom;
    STD140_FLOAT split_top;
    STD140_FLOAT tex_low;
    STD140_FLOAT tex_mid;
    STD140_FLOAT tex_high;
    STD140_FLOAT stflags;
    STD140_FLOAT sector_index;
    STD140_VEC2  normal;
    RENDER_DATA_END()
};

#ifndef __STDC__
void side_render_data_unpack(
    in vec4 vs[RENDERER_DATA_IMG_WIDTH_VEC4S],
    out side_render_data_t d) {
    d.z_floor      = vs[0].x;
    d.z_ceil       = vs[0].y;
    d.nz_floor     = vs[0].z;
    d.nz_ceil      = vs[0].w;
    d.origin       = vs[1].xy;
    d.offsets      = vs[1].zw;
    d.len          = vs[2].x;
    d.split_bottom = vs[2].y;
    d.split_top    = vs[2].z;
    d.tex_low      = vs[2].w;
    d.tex_mid      = vs[3].x;
    d.tex_high     = vs[3].y;
    d.stflags      = vs[3].z;
    d.sector_index = vs[3].w;
    d.normal       = vs[4].xy;
}
#endif // ifndef __STDC__

struct sector_render_data_t {
    RENDER_DATA_BEGIN()
    STD140_FLOAT tex_floor;
    STD140_FLOAT tex_ceil;
    STD140_FLOAT light;
    RENDER_DATA_END()
};

#ifndef __STDC__
void sector_render_data_unpack(
    in vec4 vs[RENDERER_DATA_IMG_WIDTH_VEC4S],
    out sector_render_data_t d) {
    d.tex_floor = vs[0].x;
    d.tex_ceil  = vs[0].y;
    d.light     = vs[0].z;
}
#endif // ifndef __STDC__

// TODO: sector_index should be per-instance
struct sprite_render_data_t {
    RENDER_DATA_BEGIN()
    STD140_FLOAT tex;
    STD140_FLOAT sector_index;
    RENDER_DATA_END()
};

#ifndef __STDC__
void sprite_render_data_unpack(
    in vec4 vs[RENDERER_DATA_IMG_WIDTH_VEC4S],
    out sprite_render_data_t d) {
    d.tex = vs[0].x;
    d.sector_index = vs[0].y;
}
#endif // ifndef __STDC__

struct decal_render_data_t {
    RENDER_DATA_BEGIN()
    STD140_FLOAT tex;
    STD140_FLOAT sector_index;
    STD140_FLOAT side_index;
    STD140_FLOAT plane_type;
    RENDER_DATA_END()
};

#ifndef __STDC__
void decal_render_data_unpack(
    in vec4 vs[RENDERER_DATA_IMG_WIDTH_VEC4S],
    out decal_render_data_t d) {
    d.tex = vs[0].x;
    d.sector_index = vs[0].y;
    d.side_index = vs[0].z;
    d.plane_type = vs[0].w;
}
#endif // ifndef __STDC__

struct side_frag_data_t {
    STD140_VEC2  pos;
    STD140_FLOAT _pad0;
    STD140_FLOAT id;
};

#ifndef __STDC__
vec4 side_frag_data_pack(side_frag_data_t d) {
    return vec4(d.pos, 0, d.id);
}
#endif // ifndef __STDC__

struct sector_frag_data_t {
    STD140_VEC2  pos;
    STD140_FLOAT is_ceil;
    STD140_FLOAT id;
};

#ifndef __STDC__
vec4 sector_frag_data_pack(sector_frag_data_t d) {
    return vec4(d.pos, d.is_ceil, d.id);
}
#endif // ifndef __STDC__

struct sprite_frag_data_t {
    STD140_VEC3  _pad0;
    STD140_FLOAT id;
};

#ifndef __STDC__
vec4 sprite_frag_data_pack(sprite_frag_data_t d) {
    return vec4(vec3(0), d.id);
}
#endif // ifndef __STDC__

struct decal_frag_data_t {
    STD140_VEC2  pos;
    STD140_FLOAT is_ceil;
    STD140_FLOAT id;
};

#ifndef __STDC__
vec4 decal_frag_data_pack(decal_frag_data_t d) {
    return vec4(d.pos, d.is_ceil, d.id);
}
#endif // ifndef __STDC__

struct generic_frag_data_t {
    STD140_VEC3  _pad0;
    STD140_FLOAT id;
};

#ifdef __STDC__
typedef struct side_render_data_t   side_render_data_t;
typedef struct sector_render_data_t sector_render_data_t;
typedef struct sprite_render_data_t sprite_render_data_t;
typedef struct decal_render_data_t decal_render_data_t;
typedef struct side_frag_data_t    side_frag_data_t;
typedef struct sector_frag_data_t  sector_frag_data_t;
typedef struct sprite_frag_data_t  sprite_frag_data_t;
typedef struct decal_frag_data_t  decal_frag_data_t;
typedef struct generic_frag_data_t generic_frag_data_t;

typedef union {
    vec4s raw;
    side_frag_data_t side;
    sector_frag_data_t sector;
    sprite_frag_data_t object;
    decal_frag_data_t decal;
    generic_frag_data_t generic;
} frag_data_t;

#include "util/macros.h"
STATIC_ASSERT(
    sizeof(side_render_data_t)
        <= RENDERER_DATA_IMG_WIDTH_VEC4S * sizeof(vec4s));
STATIC_ASSERT(
    sizeof(sector_render_data_t)
        <= RENDERER_DATA_IMG_WIDTH_VEC4S * sizeof(vec4s));
STATIC_ASSERT(
    sizeof(sprite_render_data_t)
        <= RENDERER_DATA_IMG_WIDTH_VEC4S * sizeof(vec4s));
STATIC_ASSERT(
    sizeof(decal_render_data_t)
        <= RENDERER_DATA_IMG_WIDTH_VEC4S * sizeof(vec4s));
#endif

#endif // ifndef RENDERER_TYPES_H
