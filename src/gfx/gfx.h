#pragma once

#include "gfx/sokol.h"
#include "util/math.h"
#include "util/map.h"
#include "util/dynlist.h"
#include "reload.h"
#include "util/resource.h"

#ifndef SOKOL_GFX_INCLUDED
#include <sokol_gfx.h>
#endif // ifndef SOKOL_GFX_INCLUDED

enum {
    GFX_NO_FLAGS   = 0,
    GFX_FLIP_X     = 1 << 0,
    GFX_FLIP_Y     = 1 << 1,
    GFX_ROTATE_CW  = 1 << 2,
    GFX_ROTATE_CCW = 1 << 3,
};

typedef struct {
    vec3s a, b;
    vec4s color;
    int frames;
} gfx_debug_line_t;

typedef struct {
    vec3s p;
    vec4s color;
    int frames;
} gfx_debug_point_t;

typedef struct gfx_state {
#ifdef RELOADABLE
    // sg_shader* -> shader_reload_t*
    map_t shader_reload_map;
#endif // ifdef RELOADABLE

    struct {
        sg_shader shader;
        sg_pipeline pipeline;
        sg_buffer ibuf, vbuf;
    } screenquad, batch;

    DYNLIST(gfx_debug_line_t) debug_lines;
    DYNLIST(gfx_debug_point_t) debug_points;
} gfx_state_t;

typedef struct {
    resource_t res;
    vec2s pos;
    vec2s scale;
    vec4s color;
    f32 z;
    int flags;
} gfx_sprite_t;

typedef struct {
    vec2s offset;
    vec2s scale;
    vec2s uv_min;
    vec2s uv_max;
    int layer;
    vec4s color;
    f32 z;
    f32 flags;
} gfx_batcher_entry_t;

typedef struct {
    DYNLIST(gfx_batcher_entry_t) entries;
    sg_buffer instance_data;
} gfx_batcher_t;

void gfx_state_init(gfx_state_t *gs);
void gfx_state_destroy(gfx_state_t *gs);

typedef void (*gfx_shader_reload_f)(sg_shader*, void*);

void gfx_load_shader(
    sg_shader *shader,
    const sg_shader_desc *(desc_fn)(sg_backend),
    gfx_shader_reload_f callback,
    void *userdata);

void gfx_unload_shader(sg_shader *shader);

int gfx_load_image(const char *resource, sg_image *out);
void gfx_screenquad(sg_image image, bool is_stencil);

void gfx_batcher_init(gfx_batcher_t *batcher);
void gfx_batcher_destroy(gfx_batcher_t *batcher);

// push sprite to render from atlas
void gfx_batcher_push_sprite(
    gfx_batcher_t *batcher,
    const gfx_sprite_t *sprite);

/* void gfx_batcher_push_image( */
/*     gfx_batcher_t *batcher, */
/*     sg_image image, */
/*     vec2s pos, */
/*     vec4s color, */
/*     f32 z, */
/*     int flags); */

/* void gfx_batcher_push_subimage( */
/*     gfx_batcher_t *batcher, */
/*     sg_image image, */
/*     vec2s pos, */
/*     vec4s color, */
/*     f32 z, */
/*     int flags, */
/*     ivec2s offset, */
/*     ivec2s size); */

void gfx_batcher_draw(
    gfx_batcher_t *batcher,
    const mat4s *proj,
    const mat4s *view);

void gfx_draw_debug(
    gfx_state_t *gs,
    const mat4s *proj,
    const mat4s *view);

void gfx_finish(gfx_state_t *gs);

void gfx_debug_line(const gfx_debug_line_t *line);

void gfx_debug_point(const gfx_debug_point_t *point);
