#include "config.h"
#include "util/aabb.h"

#define SOKOL_IMPL
#define SOKOL_DEBUG
#define SOKOL_TRACE_HOOKS

#ifdef EMSCRIPTEN
#   define SOKOL_GLES3
#else
#   define SOKOL_GLCORE33
#endif // ifdef EMSCRIPTEN

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

#include <sokol_gfx.h>
#include <sokol_gp.h>
#include <util/sokol_gfx_imgui.h>
#include <util/sokol_gl.h>
#include "gfx/sokol_gfx_ext.h"

#define SOKOL_IMGUI_IMPL
#define SOKOL_IMGUI_NO_SOKOL_APP
#include <util/sokol_imgui.h>

#include "util/math.h"

void sgp_draw_thick_line(
    float ax,
    float ay,
    float bx,
    float by,
    float thickness) {
    /* if (bx < ax) { swap(ax, bx); } */
    /* if (by < ay) { swap(ay, by); } */

    const vec2s
        normal = glms_vec2_normalize(VEC2(ay - by, bx - ax)),
        tnorm = glms_vec2_scale(normal, 0.5f * thickness),
        itnorm = glms_vec2_scale(tnorm, -1.0f);
    const vec2s ps[4] = {
        glms_vec2_add(VEC2(ax, ay), tnorm),
        glms_vec2_add(VEC2(ax, ay), itnorm),
        glms_vec2_add(VEC2(bx, by), itnorm),
        glms_vec2_add(VEC2(bx, by), tnorm),
    };
    sgp_draw_filled_triangle(SVEC2(ps[0]), SVEC2(ps[1]), SVEC2(ps[2]));
    sgp_draw_filled_triangle(SVEC2(ps[2]), SVEC2(ps[3]), SVEC2(ps[0]));
}

void sgp_draw_filled_aabbf(aabbf_t aabb) {
    sgp_draw_filled_rect(aabb.min.x, aabb.min.y, aabb.max.x - aabb.min.x, aabb.max.y - aabb.min.y);
}

void sgp_draw_aabbf(aabbf_t aabb, float thickness) {
    sgp_draw_thick_line(aabb.min.x, aabb.min.y, aabb.max.x, aabb.min.y, thickness);
    sgp_draw_thick_line(aabb.max.x, aabb.min.y, aabb.max.x, aabb.max.y, thickness);
    sgp_draw_thick_line(aabb.max.x, aabb.max.y, aabb.min.x, aabb.max.y, thickness);
    sgp_draw_thick_line(aabb.min.x, aabb.max.y, aabb.min.x, aabb.min.y, thickness);
}
