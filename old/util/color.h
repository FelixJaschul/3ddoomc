#pragma once

#include "util/math.h"

#define ABGR_TO_VEC4(_argb) ({ \
    u32 __argb = (_argb);\
    VEC4(\
         ((__argb >>  0) & 0xFF) / 255.0f,\
         ((__argb >>  8) & 0xFF) / 255.0f,\
         ((__argb >> 16) & 0xFF) / 255.0f,\
         ((__argb >> 24) & 0xFF) / 255.0f);\
    })

ALWAYS_INLINE vec4s color_scale_rgb(vec4s rgba, f32 s) {
    return glms_vec4(glms_vec3_scale(glms_vec3(rgba), s), rgba.a);
}
