#pragma once

#include "config.h"
#include "log.h"
#include "types.h"
#include "macros.h"
#include "util.h"

#include <cglm/struct.h>
#include "ivec2s.h"

#include <math.h>

#define MKV2I2(_x, _y) ((ivec2s) {{ (_x), (_y) }})
#define MKV2I1(_s) ({ __typeof__(_s) __s = (_s); ((ivec2s) {{ __s, __s }}); })
#define MKV2I0() ((ivec2s) {{ 0, 0 }})
#define IVEC2(...) (VFUNC(MKV2I, __VA_ARGS__))

#define MKV3I3(_x, _y, _z) ((ivec3s) {{ (_x), (_y), (_z) }})
#define MKV3I1(_s) ({ __typeof__(_s) __s = (_s); ((ivec3s) {{ __s, __s, __s }}); })
#define MKV3I0() ((ivec3s) {{ 0, 0 }})
#define IVEC3(...) VFUNC(MKV3I, __VA_ARGS__)

#define MKV22(_x, _y) ((vec2s) {{ (_x), (_y) }})
#define MKV21(_s) ({ __typeof__(_s) __s = (_s); ((vec2s) {{ __s, __s }}); })
#define MKV20() ((vec2s) {{ 0, 0 }})
#define VEC2(...) VFUNC(MKV2, __VA_ARGS__)

#define MKV33(_x, _y, _z) ((vec3s) {{ (_x), (_y), (_z) }})
#define MKV32(_v2, _z) ({ __typeof__(_v2) _v = (_v2); ((vec3s) {{ _v.x, _v.y, (_z) }}); })
#define MKV31(_s) ({ __typeof__(_s) __s = (_s); ((vec3s) {{ __s, __s, __s }}); })
#define MKV30() ((vec3s) {{ 0, 0 }})
#define VEC3(...) VFUNC(MKV3, __VA_ARGS__)

#define MKV44(_x, _y, _z, _w) ((vec4s) {{ (_x), (_y), (_z), (_w) }})
#define MKV42(_v3, _w) ({ __typeof__(_v3) _v = (_v3); ((vec4s) {{ _v.x, _v.y, _v.z, (_w) }}); })
#define MKV41(_s) ({ __typeof__(_s) __s = (_s); ((vec4s) {{ __s, __s, __s, __s }}); })
#define MKV40() ((vec4s) {{ 0, 0 }})
#define VEC4(...) VFUNC(MKV4, __VA_ARGS__)

#define SIVEC2(_s) (_s).x, (_s).y
#define SVEC2(_s) (_s).x, (_s).y
#define SIVEC3(_s) (_s).x, (_s).y, (_s).z
#define SVEC3(_s) (_s).x, (_s).y, (_s).z
#define SVEC4(_s) (_s).x, (_s).y, (_s).z, (_s).w

// use like PRIu**, fx.
// printf("%" PRIvec2s, FMTvec2s(vector));
#define PRIv2 "c%.3f, %.3f)"
#define PRIv3 "c%.3f, %.3f, %.3f)"
#define PRIv4 "c%.3f, %.3f, %.3f, %.3f)"

#define SCNv2 "*c%.3f, %.3f)"
#define SCNv3 "*c%.3f, %.3f, %.3f)"
#define SCNv4 "*c%.3f, %.3f, %.3f, %.3f)"

#define FMTv2(_v) '(', SVEC2(_v)
#define FMTv3(_v) '(', SVEC3(_v)
#define FMTv4(_v) '(', SVEC4(_v)

#define PSCNv2(_v) &(_v).x, &(_v).y
#define PSCNv3(_v) &(_v).x, &(_v).y, &(_v).z
#define PSCNv4(_v) &(_v).x, &(_v).y, &(_v).z, &(_v).w

// use like PRIu**, fx.
// printf("%" PRIvec2si, FMTvec2si(vector));
#define PRIv2i "c%d, %d)"
#define PRIv3i "c%d, %d, %d)"
#define PRIv4i "c%d, %d, %d, %d)"

#define SCNv2i "*c%d, %d)"
#define SCNv3i "*c%d, %d, %d)"
#define SCNv4i "*c%d, %d, %d, %d)"

#define FMTv2i(_v) '(', SVEC2(_v)
#define FMTv3i(_v) '(', SVEC3(_v)
#define FMTv4i(_v) '(', SVEC4(_v)

#define PSCNv2i(_v) &(_v).x, &(_v).y
#define PSCNv3i(_v) &(_v).x, &(_v).y, &(_v).z
#define PSCNv4i(_v) &(_v).x, &(_v).y, &(_v).z, &(_v).w

#define PRIm4                     \
    "c[ %.3f %.3f %.3f %.3f ] \n" \
    " [ %.3f %.3f %.3f %.3f ] \n" \
    " [ %.3f %.3f %.3f %.3f ] \n" \
    " [ %.3f %.3f %.3f %.3f ]]\n" \

#define FMTm4(_m) '[',              \
    _m.m00, _m.m10, _m.m20, _m.m30, \
    _m.m01, _m.m12, _m.m21, _m.m31, \
    _m.m02, _m.m12, _m.m22, _m.m32, \
    _m.m03, _m.m13, _m.m23, _m.m33  \

#define PI 3.14159265359f
#define TAU (2.0f * PI)
#define PI_2 (PI / 2.0f)
#define PI_3 (PI / 3.0f)
#define PI_4 (PI / 4.0f)
#define PI_6 (PI / 6.0f)

#define IVEC_TO_V(_v) _Generic((_v),\
    ivec2s: ({ TYPEOF(_v) __v = (_v); VEC2(__v.raw[0], __v.raw[1]); }), \
    ivec3s: ({ TYPEOF(_v) __v = (_v); VEC3(__v.raw[0], __v.raw[1], __v.raw[2]); }))

#define VEC_TO_I(_v) _Generic((_v),\
    vec2s: ({ TYPEOF(_v) __v = (_v); IVEC2(__v.raw[0], __v.raw[1]); }), \
    vec3s: ({ TYPEOF(_v) __v = (_v); IVEC3(__v.raw[0], __v.raw[1], __v.raw[2]); }))

#define deg2rad(_d) ((_d) * (PI / 180.0f))
#define rad2deg(_d) ((_d) * (180.0f / PI))

// -1, 0, 1 depending on _f < 0, _f == 0, _f > 0
#define sign(_f) ({                                                          \
    TYPEOF(_f) __f = (_f);                                                   \
    (TYPEOF(_f)) (__f < 0 ? -1 : (__f > 0 ? 1 : 0)); })

#define min(_a, _b) ({                                                       \
        TYPEOF(_a) __a = (_a), __b = (_b);                                   \
        __a < __b ? __a : __b; })

#define max(_a, _b) ({                                                       \
        TYPEOF(_a) __a = (_a), __b = (_b);                                   \
        __a > __b ? __a : __b; })

// clamp _x such that _x is in [_mi.._ma]
#define clamp(_x, _mi, _ma) (min(max(_x, _mi), _ma))

// round _n to nearest multiple of _mult
#define round_up_to_mult(_n, _mult) ({       \
        TYPEOF(_mult) __mult = (_mult);      \
        TYPEOF(_n) _m = (_n) + (__mult - 1); \
        _m - (_m % __mult);                  \
    })

// round _n to nearest multiple of _mult
#define round_up_to_multf(_n, _mult) ({      \
        TYPEOF(_mult) __mult = (_mult);      \
        TYPEOF(_n) _m = (_n) + (__mult - 1); \
        _m - fmodf(_m, __mult);              \
    })

// if abs(_x) < _e, returns _e with correct sign. otherwise returns _x
#define minabsclamp(_x, _e) ({ \
        TYPEOF(_x) __x = (_x), __e = (_e);\
        int __s = sign(__x);\
        __s = __s == 0 ? 1 : __s;\
        fabsf(__x) < __e ? (__s * __e) : (__x);\
    })

// see: https://en.wikipedia.org/wiki/Line–line_intersection

// intersect two infinite lines
#define intersect_lines(_a0, _a1, _b0, _b1) ({                               \
        TYPEOF(_a0) _l00 = (_a0), _l01 = (_a1), _l10 = (_b0), _l11 = (_b1);  \
        TYPEOF(_l00.x) _d =                                                  \
            ((_l00.x - _l01.x) * (_l10.y - _l11.y))                          \
                - ((_l00.y - _l01.y) * (_l10.x - _l11.x)),                   \
            _xl0 = glms_vec2_cross(_l00, _l01),                                        \
            _xl1 = glms_vec2_cross(_l10, _l11);                                        \
        VEC2(                                                      \
            ((_xl0 * (_l10.x - _l11.x)) - ((_l00.x - _l01.x) * _xl1)) / _d,  \
            ((_xl0 * (_l10.y - _l11.y)) - ((_l00.y - _l01.y) * _xl1)) / _d   \
        );                                                                   \
    })

// intersect two line segments, returns (nan, nan) if no intersection exists
#define intersect_segs(_a0, _a1, _b0, _b1) ({                                \
        TYPEOF(_a0) _l00 = (_a0), _l01 = (_a1), _l10 = (_b0), _l11 = (_b1);  \
        TYPEOF(_l00.x)                                                       \
            _d = ((_l00.x - _l01.x) * (_l10.y - _l11.y))                     \
                    - ((_l00.y - _l01.y) * (_l10.x - _l11.x)),               \
            _t =                                                             \
                (((_l00.x - _l10.x) * (_l10.y - _l11.y))                     \
                    - ((_l00.y - _l10.y) * (_l10.x - _l11.x)))               \
                    / _d,                                                    \
            _u =                                                             \
                (((_l00.x - _l10.x) * (_l00.y - _l01.y))                     \
                    - ((_l00.y - _l10.y) * (_l00.x - _l01.x)))               \
                    / _d;                                                    \
        (isnan(_t) || isnan(_u)) ?                                           \
            VEC2(NAN) \
            : ((_t >= 0 && _t <= 1 && _u >= 0 && _u <= 1) ?                  \
                (VEC2(                                                      \
                    _l00.x + (_t * (_l01.x - _l00.x)),                       \
                    _l00.y + (_t * (_l01.y - _l00.y))))                     \
                : VEC2(NAN));                                      \
    })

// true if _p is in triangle _a, _b, _c
#define point_in_triangle(_p, _a, _b, _c) ({                                 \
        TYPEOF(_p) __p = (_p), __a = (_a), __b = (_b), __c = (_c);           \
        const f32                                                            \
            _d = ((__b.y - __c.y) * (__a.x - __c.x)                          \
                  + (__c.x - __b.x) * (__a.y - __c.y)),                      \
            _x = ((__b.y - __c.y) * (__p.x - __c.x)                          \
                  + (__c.x - __b.x) * (__p.y - __c.y)) / _d,                 \
            _y = ((__c.y - __a.y) * (__p.x - __c.x)                          \
                  + (__a.x - __c.x) * (__p.y - __c.y)) / _d,                 \
            _z = 1 - _x - _y;                                                \
        (_x > 0) && (_y > 0) && (_z > 0);                                    \
    })

// -1 right, 0 on, 1 left
#define point_side(_p, _a, _b) ({                                            \
        TYPEOF(_p) __p = (_p), __a = (_a), __b = (_b);                       \
        -(((__p.x - __a.x) * (__b.y - __a.y))                                \
            - ((__p.y - __a.y) * (__b.x - __a.x)));                          \
    })

// returns fractional part of float _f
#define fract(_f) ({ TYPEOF(_f) __f = (_f); __f - ((i64) (__f)); })

// if _x is nan, returns _alt otherwise returns _x
#define ifnan(_x, _alt) ({ TYPEOF(_x) __x = (_x); isnan(__x) ? (_alt) : __x; })

// give alternative values for x if it is nan or inf
#define ifnaninf(_x, _nan, _inf) ({                                          \
        TYPEOF(_x) __x = (_x);                                               \
        isnan(__x) ? (_nan) : (isinf(__x) ? (_inf) : __x);                   \
    })

// lerp from _a -> _b by _t
#define lerp(_a, _b, _t) ({                                                  \
        TYPEOF(_t) __t = (_t);                                               \
        (TYPEOF(_a)) (((_a) * (1 - __t)) + ((_b) * __t));                    \
    })

// get t on _a -> _b where _p is closest
#define point_project_segment_t(_p, _a, _b) ({                               \
        TYPEOF(_p) _q = (_p), _v0 = (_a), _v1 = (_b);                        \
        const f32 _l2 = glms_vec2_norm2(glms_vec2_sub(_v0, _v1));            \
        const f32 _t =                                                       \
            glms_vec2_dot(                                                   \
                (VEC2(_q.x - _v0.x, _q.y - _v0.y)),                          \
                (VEC2(_v1.x - _v0.x, _v1.y - _v0.y ))) / _l2;                \
        _l2 <= 0.000001f ? 0.0f : _t;                                        \
    })

// project point _p onto line _a -> _b
#define point_project_segment(_p, _a, _b) ({                                 \
        TYPEOF(_p) _q = (_p), _v0 = (_a), _v1 = (_b);                        \
        const f32 _l2 = powf(glms_vec2_norm(glms_vec2_sub(_v0, _v1)), 2.0f);                      \
        const f32 _t =                                                       \
            clamp(                                                           \
                glms_vec2_dot(                                                         \
                    (VEC2(_q.x - _v0.x, _q.y - _v0.y)),                   \
                    (VEC2(_v1.x - _v0.x, _v1.y - _v0.y ))) / _l2,          \
                0, 1);                                                       \
        const TYPEOF(_p) _proj = VEC2(                                           \
            _v0.x + _t * (_v1.x - _v0.x),                                    \
            _v0.y + _t * (_v1.y - _v0.y)                                    \
            );                                                                   \
        _l2 <= 0.000001f ? _v0 : _proj;                                      \
    })

// distance from _p to _a -> _b
#define point_to_segment(_p1, _a1, _b1) ({                                   \
        TYPEOF(_p1) _q1 = (_p1);                                             \
        const vec2s _u1 = point_project_segment(_q1, (_a1), (_b1));             \
        glms_vec2_norm(glms_vec2_sub(_q1, _u1));                                                  \
    })

// returns true if point _p is in box _a -> _b
#define point_in_box(_p, _a, _b) ({                                          \
        TYPEOF(_p) __p = (_p), __a = (_a), __b = (_b);                       \
        if (__a.x > __b.x) { swap(__a.x, __b.x); }                           \
        if (__a.y > __b.y) { swap(__a.y, __b.y); }                           \
        __p.x >= __a.x && __p.y >= __a.y && __p.x <= __b.x && __p.y <= __b.y;\
    })

// projects a onto b
#define project_vec2(_a, _b) ({\
        TYPEOF(_a) __a = (_a), __b = (_b);\
        const f32 q = glms_vec2_dot(__a, __b) / glms_vec2_dot(__b, __b);\
        VEC2(q * __b.x, q * __b.y);\
    })

// 2D vector rotation
#define rotate(_vr, _th) ({                                                  \
        TYPEOF(_vr) __vr = (_vr);                                            \
        TYPEOF(_th) __th = (_th), __as = sinf(__th), __ac = cosf(__th);\
        VEC2(\
            (__vr.x * __ac) - (__vr.y * __as),             \
            (__vr.x * __as) + (__vr.y * __ac)             \
        );                                                                   \
    })

// intersect line segment s0 -> s1 with box defined by b0, b1
ALWAYS_INLINE vec2s intersect_seg_box(vec2s s0, vec2s s1, vec2s b0, vec2s b1) {
    if (b0.x > b1.x) { swap(b0.x, b1.x); }
    if (b0.y > b1.y) { swap(b0.y, b1.y); }

    vec2s r;
    if (!isnan((r =
            intersect_segs(
                s0, s1,
                (VEC2(b0.x, b0.y)), (VEC2(b0.x, b1.y)))).x)) {
        return r;
    } else if (
        !isnan((r =
            intersect_segs(
                s0, s1,
                (VEC2(b0.x, b1.y)), (VEC2(b1.x, b1.y)))).x)) {
        return r;
    } else if (
        !isnan((r =
            intersect_segs(
                s0, s1,
                (VEC2(b1.x, b1.y)), (VEC2(b1.x, b0.y)))).x)) {
        return r;
    } else if (
        !isnan((r =
            intersect_segs(
                s0, s1,
                (VEC2(b1.x, b0.y)), (VEC2(b0.x, b0.y)))).x)) {
        return r;
    }

    return VEC2(NAN, NAN);
}

ALWAYS_INLINE bool intersect_planes(vec4s a, vec4s b, vec3s *point, vec3s *dir) {
    *dir = glms_cross(glms_vec3(a), glms_vec3(b));

    const f32 det = glms_vec3_norm2(*dir);
    if (det < 0.001f) {
        // parallel
        WARN("eee");
        return false;
    }

    *point =
        glms_vec3_divs(
            glms_vec3_add(
                glms_vec3_scale(glms_vec3_cross(*dir, glms_vec3(b)), a.w),
                glms_vec3_scale(glms_vec3_cross(glms_vec3(a), *dir), b.w)),
            det);
    return true;
}

ALWAYS_INLINE vec3s coplanar_point(vec4s p) {
    return glms_vec3_scale(glms_vec3(p), -p.w);
}

ALWAYS_INLINE vec3s intersect_3_planes(vec4s a, vec4s b, vec4s c, vec3s *point) {
    const vec3s
        n1 = glms_vec3(a), n2 = glms_vec3(b), n3 = glms_vec3(c),
        x1 = coplanar_point(a), x2 = coplanar_point(b), x3 = coplanar_point(c),
        f1 = glms_vec3_scale(glms_vec3_cross(n2, n3), glms_vec3_dot(x1, n1)),
        f2 = glms_vec3_scale(glms_vec3_cross(n3, n1), glms_vec3_dot(x2, n2)),
        f3 = glms_vec3_scale(glms_vec3_cross(n1, n2), glms_vec3_dot(x3, n3));

    mat3s m;
    m.col[0] = n1;
    m.col[1] = n2;
    m.col[2] = n3;
    const f32 det = glms_mat3_det(m);

    const vec3s sum = glms_vec3_add(f1, glms_vec3_add(f2, f3));
    return glms_vec3_divs(sum, det);
}

ALWAYS_INLINE bool intersect_circle_circle(vec2s p0, f32 r0, vec2s p1, f32 r1) {
    // distance between centers must be between r0 - r1, r0 + r1
    const f32
        mi = fabsf(r0 - r1),
        ma = r0 + r1,
        dx = p1.x - p0.x,
        dy = p1.y - p0.y,
        d = (dx * dx) + (dy * dy);
    return mi <= d && d <= ma;
}

// [pr]0 are static, [pr]1 are dynamic along v
ALWAYS_INLINE bool sweep_circle_circle(
    vec2s p0, f32 r0, vec2s p1, f32 r1, vec2s v, f32 *t) {

    const vec2s
        a = p1,
        b = glms_vec2_add(p1, v),
        d = point_project_segment(p0, a, b);

    if (glms_vec2_norm(glms_vec2_sub(p0, d)) > r0 + r1) {
        return false;
    }

    if (t) {
        *t =
            glms_vec2_norm(glms_vec2_sub(d, a))
                / glms_vec2_norm(glms_vec2_sub(b, a));
    }
    return true;
}

// sweep circle at p with radius r along vector d, collide line segment a -> b
ALWAYS_INLINE bool sweep_circle_line_segment(
    vec2s p,
    f32 r,
    vec2s d,
    vec2s a,
    vec2s b,
    f32 *t_circle,
    f32 *t_segment,
    vec2s *resolved) {
    const f32 t = point_project_segment_t(p, a, b);
    const vec2s closest = glms_vec2_lerp(a, b, t);

    const vec2s closest_to_p = glms_vec2_sub(p, closest);
    const f32 dist_sqr = glms_vec2_norm2(closest_to_p);

    const vec2s normal = glms_vec2_normalize(glms_vec2_sub(closest, p));

    // circle is already touching line
    if (t >= 0.0f && t <= 1.0f && dist_sqr < r * r) {
        *t_circle = 0.0f;
        *t_segment = t;
        *resolved =
            glms_vec2_add(
                p,
                glms_vec2_scale(
                    normal,
                    -(r - sqrtf(dist_sqr))));
        return true;
    }

    const vec2s q = glms_vec2_add(p, d);
    const vec2s hit = intersect_segs(a, b, p, q);

    // circle does not hit but may be touching endpoint
    if (isnan(hit.x)) {
        const f32
            dot_start = glms_vec2_dot(glms_vec2_sub(a, p), d),
            dot_end = glms_vec2_dot(glms_vec2_sub(b, p), d);

        vec2s endpoint;
        f32 ts;

        if (dot_start < 0.0f) {
            if (dot_end < 0.0f) {
                return false;
            }

            ts = 1.0f;
            endpoint = b;
        } else if (dot_end < 0.0f) {
            ts = 0.0f;
            endpoint = a;
        } else {
            endpoint = dot_start < dot_end ? a : b;
            ts = dot_start < dot_end ? 0.0f : 1.0f;
        }

        // collide with the endpoint, a 0-radius circle
        if (intersect_circle_circle(p, r, endpoint, 0.0f)) {
            *t_circle = 0.0f;
            *t_segment = ts;
            *resolved = p;
            return true;
        }

        return false;
    }

    // circle has swept collision with line, but it may stop directly on the it
    const vec2s v_to_line = project_vec2(d, normal);

    const f32
        t_to_line = glms_vec2_norm(v_to_line),
        needed_t_to_line = sqrtf(dist_sqr) - r;

    // check if the actual travel (t_to_line) is less than the necessary travel
    // to touch
    if (t_to_line < needed_t_to_line) {
        return false;
    }

    // circle intersects line, not necessarily segment
    const f32
        t_hit = needed_t_to_line / t_to_line,
        t_end = point_project_segment_t(q, a, b);

    // find distance along line segment at the point of intersection
    const f32 t_int = lerp(t, t_end, t_hit);

    // point is within segment bounds and circle is moving towards line
    if (t_int >= 0.0f
        && t_int <= 1.0f
        && glms_vec2_dot(v_to_line, glms_vec2_sub(closest, p)) >= 0.0f) {
        *t_circle = t_hit;
        *t_segment = t_int;
        *resolved = glms_vec2_add(p, glms_vec2_scale(d, t_hit));
        return true;
    }

    // may have hit an endpoint
    const vec2s endpoint = t > 1.0f ? b : a;

    if (intersect_circle_circle(p, r, endpoint, 0.0f)) {
        *t_circle = t_hit;
        *t_segment = clamp(t, 0.0f, 1.0f);
        *resolved = glms_vec2_add(p, glms_vec2_scale(d, t_hit));
        return true;
    }

    return false;
}

// TODO: doc
ALWAYS_INLINE int intersect_circle_seg(
    vec2s p, f32 r, vec2s s0, vec2s s1, f32 *ts, vec2s *p_resolved) {
    // see stackoverflow.com/questions/1073336
    const vec2s
        d = VEC2(s1.x - s0.x, s1.y - s0.y),
        f = VEC2(s0.x - p.x, s0.y - p.y);

    const f32
        a = glms_vec2_dot(d, d),
        b = 2.0f * glms_vec2_dot(f, d),
        c = glms_vec2_dot(f, f) - (r * r),
        q = (b * b) - (4 * a * c);

    if (q < 0) {
        return 0;
    }

    const f32 r_q = sqrtf(q);
    f32 s, t0, t1;

    if (b >= 0) {
        s = (-b - r_q) / 2;
    } else {
        s = (-b + r_q) / 2;
    }

    t0 = s / a;
    t1 = c / s;

    int n = 0;

    if (t0 >= 0 && t0 <= 1) {
        ts[n++] = 1.0f - t0;
    }

    if (t1 >= 0 && t1 <= 1) {
        ts[n++] = 1.0f - t1;
    }

    if (n == 2 && ts[0] > ts[1]) {
        swap(ts[0], ts[1]);
    }

    if (n != 0 && p_resolved) {
        // resolve circle
        const vec2s q = point_project_segment(p, s0, s1);
        const vec2s q_to_p = glms_vec2_sub(p, q);
        const vec2s d_q_to_p = glms_vec2_normalize(q_to_p);

        *p_resolved =
            glms_vec2_add(
                p,
                glms_vec2_scale(
                    d_q_to_p,
                    r - glms_vec2_norm(q_to_p)));
    }

    return n;
}

ALWAYS_INLINE bool segments_are_colinear(vec2s a, vec2s b, vec2s c, vec2s d) {
    const f32
        dx_ab = b.x - a.x,
        dy_ab = b.y - a.y,
        dx_cd = d.x - c.x,
        dy_cd = d.y - c.y;

    // horizontal case
    if (fabsf(dy_ab) < 0.0001f) {
        return fabsf(a.y - c.y) < 0.0001f && fabsf(a.y - d.y) < 0.0001f;
    } else if (fabsf(dy_cd) < 0.0001f) {
        return fabsf(c.y - a.y) < 0.0001f && fabsf(c.y - b.y) < 0.0001f;
    }

    // vertical case
    if (fabsf(dx_ab) < 0.0001f) {
        return fabsf(a.x - c.x) < 0.0001f && fabsf(a.x - d.x) < 0.0001f;
    } else if (fabsf(dx_cd) < 0.0001f) {
        return fabsf(c.x - a.x) < 0.0001f && fabsf(c.x - b.x) < 0.0001f;
    }

    const f32
        m_ab = dx_ab / dy_ab,
        m_cd = dx_cd / dy_cd,
        yi_ab = a.y - m_ab * a.x,
        yi_cd = c.y - m_cd * c.x;

    return fabsf(m_ab - m_cd) < 0.0001f && fabsf(yi_ab - yi_cd) < 0.0001f;
}

// noramlize angle to 0, TAU
ALWAYS_INLINE f32 wrap_angle(f32 a) {
    a = fmodf(a, TAU);
    return a < 0 ? (a + TAU) : a;
}

// compute inner angle formed by points p1 -> p2 -> p3
ALWAYS_INLINE f32 angle_in_points(vec2s a, vec2s b, vec2s c) {
    // from stackoverflow.com/questions/3486172 and SLADE (doom level editor)
	vec2s ab = VEC2(b.x - a.x, b.y - a.y);
	vec2s cb = VEC2(b.x - c.x, b.y - c.y);

    const f32
        dot = ab.x * cb.x + ab.y * cb.y,
        labsq = ab.x * ab.x + ab.y * ab.y,
        lcbsq = cb.x * cb.x + cb.y * cb.y;

	// square of cosine of the needed angle
	const f32 cossq = dot * dot / labsq / lcbsq;

	// apply trigonometric equality
	// cos(2a) = 2([cos(a)]^2) - 1
	const f32 cos2 = 2 * cossq - 1;

    // cos2 = cos(2a) -> a = arccos(cos2) / 2;
	f32 alpha = ((cos2 <= -1) ? PI : (cos2 >= 1) ? 0 : acosf(cos2)) / 2;

	// negative dot product -> angle is above 90 degrees. normalize.
	if (dot < 0) {
		alpha = PI - alpha;
    }

    // compute sign with determinant
	const f32 det = ab.x * cb.y - ab.y * cb.x;
	if (det < 0) {
		alpha = (2 * PI) - alpha;
    }

	return TAU - alpha;
}

// number of 1 bits in number
#define popcount(_x) __builtin_popcountll((_x))

// number of 0 bits in (unsigned) number
#define invpopcount(_x) (_Generic((_x),                                        \
    u8:  (64 - __builtin_popcountll(0xFFFFFFFFFFFFFF00 | (u64) (_x))),         \
    u16: (64 - __builtin_popcountll(0xFFFFFFFFFFFF0000 | (u64) (_x))),         \
    u32: (64 - __builtin_popcountll(0xFFFFFFFF00000000 | (u64) (_x))),         \
    u64: (64 - __builtin_popcountll(                     (u64) (_x)))))

// (C)ount (L)eading (Z)eros
#define clz(_x) (__builtin_clz((_x)))

// (C)ount (T)railing (Z)eros
#define ctz(_x) (__builtin_ctz((_x)))

// returns true if ts makes a hole inside of vs
bool polygon_is_hole(
    vec2s vs[][2],
    int n_vs,
    vec2s ts[][2],
    int n_ts);

// MATRIX MATH
ALWAYS_INLINE vec4s plane_normalize(vec4s p) {
    const f32 mag = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
    return glms_vec4_divs(p, mag);
}

enum {
    PLANE_LEFT = 0,
    PLANE_RIGHT,
    PLANE_BOTTOM,
    PLANE_TOP,
    PLANE_NEAR,
    PLANE_FAR,
};

ALWAYS_INLINE void extract_view_proj_planes(
    const mat4s *mat,
    vec4s *planes) {
    for (int i = 4; i--; ) planes[0].raw[i] = mat->raw[i][3] + mat->raw[i][0];
    for (int i = 4; i--; ) planes[1].raw[i] = mat->raw[i][3] - mat->raw[i][0];
    for (int i = 4; i--; ) planes[2].raw[i] = mat->raw[i][3] + mat->raw[i][1];
    for (int i = 4; i--; ) planes[3].raw[i] = mat->raw[i][3] - mat->raw[i][1];
    for (int i = 4; i--; ) planes[4].raw[i] = mat->raw[i][3] + mat->raw[i][2];
    for (int i = 4; i--; ) planes[5].raw[i] = mat->raw[i][3] - mat->raw[i][2];

    for (int i = 0; i < 6; i++) {
        planes[i] = plane_normalize(planes[i]);
    }
}

enum {
    VIEW_PROJ_POINTS_NBL = 0,
    VIEW_PROJ_POINTS_NBR,
    VIEW_PROJ_POINTS_NTR,
    VIEW_PROJ_POINTS_NTL,
    VIEW_PROJ_POINTS_FBL,
    VIEW_PROJ_POINTS_FBR,
    VIEW_PROJ_POINTS_FTR,
    VIEW_PROJ_POINTS_FTL,
};

ALWAYS_INLINE void extract_view_proj_points(
    const mat4s *mat,
    vec3s points[8]) {
    const vec3s unit[8] = {
        [VIEW_PROJ_POINTS_NBL] = VEC3(-1, -1, -1),
        [VIEW_PROJ_POINTS_NBR] = VEC3(+1, -1, -1),
        [VIEW_PROJ_POINTS_NTR] = VEC3(+1, +1, -1),
        [VIEW_PROJ_POINTS_NTL] = VEC3(-1, +1, -1),
        [VIEW_PROJ_POINTS_FBL] = VEC3(-1, -1, +1),
        [VIEW_PROJ_POINTS_FBR] = VEC3(+1, -1, +1),
        [VIEW_PROJ_POINTS_FTR] = VEC3(+1, +1, +1),
        [VIEW_PROJ_POINTS_FTL] = VEC3(-1, +1, +1),
    };
    const mat4s inv = glms_mat4_inv(*mat);
    for (int i = 0; i < 8; i++) {
        const vec4s v = glms_mat4_mulv(inv, glms_vec4(unit[i], 1.0f));
        points[i] = glms_vec3_divs(glms_vec3(v), v.w);
    }
}

ALWAYS_INLINE mat4s remove_pitch(const mat4s *view_proj) {
    mat3s rot = glms_mat4_pick3(*view_proj);
    rot.m10 = 0.0f;
    rot.m11 = 1.0f;
    rot.m12 = 0.0f;

    mat4s res = *view_proj;
    glm_mat4_ins3(rot.raw, res.raw);
    return res;
}

ALWAYS_INLINE void extract_view_proj_planes_2d(
    const mat4s *view_proj, vec2s lines[4][2]) {
    vec3s points[8];
    extract_view_proj_points(view_proj, points);

    const mat3s m = glms_mat4_pick3(*view_proj);
    const f32 pitch = atan2f(m.m21, m.m22);

    vec2s near[2], far[2];
    if (pitch < PI_2) {
        near[0] = glms_vec2(points[VIEW_PROJ_POINTS_NBL]);
        near[1] = glms_vec2(points[VIEW_PROJ_POINTS_NBR]);
        far[0] = glms_vec2(points[VIEW_PROJ_POINTS_FTL]);
        far[1] = glms_vec2(points[VIEW_PROJ_POINTS_FTR]);
    } else {
        near[0] = glms_vec2(points[VIEW_PROJ_POINTS_NTL]);
        near[1] = glms_vec2(points[VIEW_PROJ_POINTS_NTR]);
        far[0] = glms_vec2(points[VIEW_PROJ_POINTS_FBL]);
        far[1] = glms_vec2(points[VIEW_PROJ_POINTS_FBR]);
    }

    lines[0][0] = near[0]; lines[0][1] = near[1]; // ln -> rn
    lines[1][0] = near[1]; lines[1][1] = far[1];  // rn -> rf
    lines[2][0] = far[1];  lines[2][1] = far[0];  // rf -> lf
    lines[3][0] = far[0];  lines[3][1] = near[0]; // lf -> ln
}

typedef struct aabbf aabbf_t;

aabbf_t extract_view_proj_aabb_2d(const mat4s *view_proj);

ALWAYS_INLINE f32 plane_classify(vec4s plane, vec3s point) {
    return plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w;
}

ALWAYS_INLINE vec3s plane_project(vec4s plane, vec3s point) {
    const vec3s n = glms_vec3(plane);
    const f32 t = (glms_vec3_dot(point, n) + plane.w) / glms_vec3_dot(n, n);
    return glms_vec3_sub(point, glms_vec3_scale(n, t));
}

// GLM EXTENSIONS
ALWAYS_INLINE vec2s glms_vec2_round(vec2s v) {
    return VEC2(roundf(v.x), roundf(v.y));
}

ALWAYS_INLINE ivec2s glms_ivec2_min(ivec2s v, ivec2s u) {
    return IVEC2(min(v.x, u.x), min(v.y, u.y));
}

ALWAYS_INLINE ivec2s glms_ivec2_max(ivec2s v, ivec2s u) {
    return IVEC2(max(v.x, u.x), max(v.y, u.y));
}

ALWAYS_INLINE ivec2s glms_ivec2_clamp(ivec2s v, ivec2s mi, ivec2s ma) {
    return IVEC2(clamp(v.x, mi.x, ma.x), clamp(v.y, mi.y, ma.y));
}
