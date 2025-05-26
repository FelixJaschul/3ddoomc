#pragma once

#include "util/math.h"
#include "util/types.h"
#include "defs.h"

// possible results from path_trace_resolve_f "resolve"
enum {
    PATH_TRACE_STOP,
    PATH_TRACE_CONTINUE,
    PATH_TRACE_RETRY,
};

// flags
enum {
    PATH_TRACE_NONE        = 0,
    PATH_TRACE_ADD_OBJECTS = 1 << 0,
    PATH_TRACE_ADD_DECALS  = 1 << 1,
    PATH_TRACE_FORCE_NEAR_PORTALS = 1 << 2
};

enum {
    PATH_TRACE_RESOLVE_PORTAL_NONE = 0,
    PATH_TRACE_RESOLVE_PORTAL_FORCE_AWAY = 1 << 0
};

// "hit" of some wall/object/plane/decal while pathing
typedef struct path_hit {
    // swept position on hit
    vec2s swept_pos;

    // "t" on from -> to
    f32 t;

    // T_* type
    u8 type;

    union {
        struct {
            vec2s pos;
            f32 u, x;
            wall_t *wall;
            side_t *side;
            bool is_force_portal;
        } wall;

        struct {
            object_t *ptr;
        } object;

        // only for 3D pathing
        struct {
            sector_t *ptr;
            plane_type plane;
        } sector;
    };

    decal_t *decal;
} path_hit_t;

typedef int (*path_trace_resolve_f)(
    level_t*, const path_hit_t*, vec2s*, vec2s*, void*);

typedef int (*path_trace_3d_resolve_f)(
    level_t*, const path_hit_t*, vec3s*, vec3s*, void*);

bool path_trace_resolve_portal(
    level_t *level,
    const path_hit_t *hit,
    vec2s *from,
    vec2s *to,
    int *res,
    f32 *angle,
    int flags);

bool path_trace_3d_resolve_portal(
    level_t *level,
    const path_hit_t *hit,
    vec3s *from,
    vec3s *to,
    int *res,
    f32 *angle,
    int flags);

void path_trace(
    level_t *level,
    vec2s *from,
    vec2s *to,
    f32 r,
    path_trace_resolve_f resolve,
    void *userdata,
    int flags);

void path_trace_3d(
    level_t *level,
    vec3s *from,
    vec3s *to,
    f32 r,
    path_trace_3d_resolve_f resolve,
    void *userdata,
    int flags);
