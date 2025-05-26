#pragma once

#include "util/macros.h"
#include "util/types.h"
#include "util/math.h"

#define AABB_TO_F(_a)                                                      \
    ({ TYPEOF(_a) __a = (_a);                                           \
       (aabbf_t) { IVEC_TO_V(__a.min), glms_vec2_add(IVEC_TO_V(__a.max), VEC2(1)) }; })

#define AABBF_TO_I(_a)                                                      \
    ({ TYPEOF(_a) __a = (_a);                                           \
       (aabb_t) { VEC_TO_I(__a.min), VEC_TO_I(__a.max) }; })

// 2D float aabb
typedef struct aabbf {
    vec2s min, max;
} aabbf_t;

// 2D int aabb
typedef struct aabb {
    ivec2s min, max;
} aabb_t;

#define AABBF_MM(_mi, _ma) ((aabbf_t) { (_mi), (_ma) })
#define AABB_MM(_mi, _ma) ((aabb_t) { (_mi), (_ma) })

#define AABBF_M(_ma) ((aabbf_t) { VEC2S(0), (_ma) })
#define AABB_M(_ma) ((aabb_t) { IVEC2S(0), (_ma) })

#define PRIaabbf "c%" PRIv2 " %" PRIv2 "]"
#define FMTaabbf(_v) '[', FMTv2((_v).min), FMTv2((_v).max)

#define PRIaabb "c%" PRIv2i " %" PRIv2i "]"
#define FMTaabb(_v) '[', FMTv2i((_v).min), FMTv2i((_v).max)

#define AABBF_PS(_pos, _size) ({                                      \
        const vec2s __pos = (_pos);                                     \
        ((aabbf_t) { __pos, glms_vec2_add(__pos, (_size)) });                    \
    })

#define AABB_PS(_pos, _size) ({                                       \
        const ivec2s __pos = (_pos);                                    \
        ((aabb_t) { __pos, glms_ivec2_add(__pos, glms_ivec2_sub((_size), IVEC2(1))) }); \
    })

#define AABBF_CH(_center, _half) ({                                   \
        const vec2s __c = (_center), __h = (_half);                     \
        ((aabbf_t) { vec2s_sub(__c, __h), vec2s_add(__c, __h)});              \
    })

#define AABB_CH(_center, _half) ({                                    \
        const ivec2s __c = (_center), __h = (_half);                     \
        ((aabb_t) { glms_ivec2_sub(__c, __h), glms_ivec2_add(__c, __h)});             \
    })

// returns true if aabb contains p
#define _aabb_contains_impl(_name, _T, _t, _V, _Q, _U, _s)                   \
    ALWAYS_INLINE bool _name(_T aabb, _V p) {                            \
        return p.x >= aabb.min.x                                         \
            && p.x <= aabb.max.x                                         \
            && p.y >= aabb.min.y                                         \
            && p.y <= aabb.max.y;                                        \
    }

// returns true if a and b collide
#define _aabb_collides_impl(_name, _T, _t, _V, _Q, _U, _s)                   \
    ALWAYS_INLINE bool _name(_T a, _T b) {                              \
        UNROLL(2)                                                       \
        for (usize i = 0; i < 2; i++) {                                 \
            if (a.min.raw[i] >= b.max.raw[i]                            \
                || a.max.raw[i] <= b.min.raw[i]) {                      \
                return false;                                           \
            }                                                           \
        }                                                               \
                                                                        \
        return true;                                                    \
    }

// returns overlap of a and b
#define _aabb_intersect_impl(_name, _T, _t, _V, _Q, _U, _s)                  \
    ALWAYS_INLINE _T _name(_T a, _T b) {                                \
        _T r;                                                           \
                                                                        \
        UNROLL(2)                                                       \
        for (usize i = 0; i < 2; i++) {                                 \
            r.min.raw[i] =                                              \
                a.min.raw[i] < b.min.raw[i] ?                           \
                    b.min.raw[i] : a.min.raw[i];                        \
            r.max.raw[i] =                                              \
                a.max.raw[i] > b.max.raw[i] ?                           \
                    b.max.raw[i] : a.max.raw[i];                        \
        }                                                               \
                                                                        \
        return r;                                                       \
    }

// returns center of aabb
#define _aabb_center_impl(_name, _T, _t, _V, _Q, _U, _s)                     \
    ALWAYS_INLINE _V _name(_T a) {                                      \
        return glms_##_Q##_add(glms_##_Q##_divs(glms_##_Q##_sub(a.max, a.min), 2), a.min);   \
    }

// translates aabb by v
#define _aabb_translate_impl(_name, _T, _t, _V, _Q, _U, _s)                  \
    ALWAYS_INLINE _T _name(_T a, _V v) {                                \
        return (_T) { glms_##_Q##_add(a.min, v), glms_##_Q##_add(a.max, v) };         \
    }

// scales aabb by v, keeping center constant
#define _aabb_scale_center_impl(_name, _T, _t, _V, _Q, _U, _s)               \
    ALWAYS_INLINE _T _name(_T a, _V v) {                                \
        _V                                                              \
            c = _t##_center(a),                                         \
            d = glms_##_Q##_sub(a.max, a.min),                                 \
            h = glms_##_Q##_divs(d, 2),                                        \
            e = glms_##_Q##_mul(h, v);                                         \
        return (_T) { glms_##_Q##_sub(c, e), glms_##_Q##_add(c, e) };                 \
    }

// scales aabb by v, keeping min constant
#define _aabb_scale_min_impl(_name, _T, _t, _V, _Q, _U, _s)                  \
    ALWAYS_INLINE _T _name(_T a, _V v) {                                \
        _V d = glms_##_Q##_sub(a.max, a.min);                                  \
        return (_T) { a.min, glms_##_Q##_add(a.min, glms_##_Q##_mul(d, v)) };         \
    }


// scales aabb by v, both min and max
#define _aabb_scale_impl(_name, _T, _t, _V, _Q, _U, _s)                      \
    ALWAYS_INLINE _T _name(_T a, _V v) {                                \
        return (_T) { glms_##_Q##_mul(a.min, v), glms_##_Q##_mul(a.max, v) };         \
    }

// get points of aabb
#define _aabb_points_impl(_name, _T, _t, _V, _Q, _U, _s)                     \
    ALWAYS_INLINE void _name(_T a, _V ps[4]) {                          \
        ps[0] = _U(a.min.x, a.min.y);                                   \
        ps[1] = _U(a.max.x, a.min.y);                                   \
        ps[2] = _U(a.max.x, a.max.y);                                   \
        ps[3] = _U(a.min.x, a.max.y);                                   \
    }

// get "half" (half size) of aabb
#define _aabb_half_impl(_name, _T, _t, _V, _Q, _U, _s)                       \
    ALWAYS_INLINE _V _name(_T a) {                                      \
        return glms_##_Q##_sub(_t##_center(a), a.min);                         \
    }

// center aabb on new point
#define _aabb_center_on_impl(_name, _T, _t, _V, _Q, _U, _s)                  \
    ALWAYS_INLINE _T _name(_T a, _V c) {                                \
        _V half = _t##_half(a);                                         \
        return (_T) { glms_##_Q##_sub(c, half), glms_##_Q##_add(c, half)};            \
    }

// center aabb in another aabb
#define _aabb_center_in_impl(_name, _T, _t, _V, _Q, _U, _s)                  \
    ALWAYS_INLINE _T _name(_T a, _T b) {                                \
        return _t##_center_on(a, _t##_center(b));                       \
    }

// "sort" aabb min/max
#define _aabb_sort_impl(_name, _T, _t, _V, _Q, _U, _s)                  \
    ALWAYS_INLINE _T _name(_T a) {                                \
        return (_T) {\
            .min = _U(min(a.min.x, a.max.x), min(a.min.y, a.max.y)),\
            .max = _U(max(a.min.x, a.max.x), max(a.min.y, a.max.y)),\
        };\
    }

#define _aabb_define_functions(_prefix, _T, _t, _V, _Q, _U, _S)              \
    _aabb_contains_impl(_prefix##_contains, _T, _t, _V, _Q, _U, _S)          \
    _aabb_collides_impl(_prefix##_collides, _T, _t, _V, _Q, _U, _S)          \
    _aabb_intersect_impl(_prefix##_intersect, _T, _t, _V, _Q, _U, _S)        \
    _aabb_center_impl(_prefix##_center, _T, _t, _V, _Q, _U, _S)              \
    _aabb_translate_impl(_prefix##_translate, _T, _t, _V, _Q, _U, _S)        \
    _aabb_scale_center_impl(_prefix##_scale_center, _T, _t, _V, _Q, _U, _S)  \
    _aabb_scale_min_impl(_prefix##_scale_min, _T, _t, _V, _Q, _U, _S)        \
    _aabb_scale_impl(_prefix##_scale, _T, _t, _V, _Q, _U, _S)                \
    _aabb_half_impl(_prefix##_half, _T, _t, _V, _Q, _U, _s)                  \
    _aabb_center_on_impl(_prefix##_center_on, _T, _t, _V, _Q, _U, _s)        \
    _aabb_center_in_impl(_prefix##_center_in, _T, _t, _V, _Q, _U, _s)        \
    _aabb_points_impl(_prefix##_points, _T, _t, _V, _Q, _U, _S)              \
    _aabb_sort_impl(_prefix##_sort, _T, _t, _V, _Q, _U, _S)              \

// returns size of aabb
ALWAYS_INLINE vec2s aabbf_size(aabbf_t a) {
    return glms_vec2_sub(a.max, a.min);
}

// returns size of aabb
ALWAYS_INLINE ivec2s aabb_size(aabb_t a) {
    return glms_ivec2_add(glms_ivec2_sub(a.max, a.min), IVEC2(1));
}

_aabb_define_functions(aabbf, aabbf_t, aabbf, vec2s, vec2, VEC2, f32)
_aabb_define_functions(aabb, aabb_t, aabb, ivec2s, ivec2, IVEC2, int)
