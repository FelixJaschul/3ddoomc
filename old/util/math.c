#include "util/math.h"
#include "util/aabb.h"

aabbf_t extract_view_proj_aabb_2d(const mat4s *view_proj) {
    vec3s points[8];
    extract_view_proj_points(view_proj, points);

    vec2s mi = glms_vec2(points[0]), ma = glms_vec2(points[0]);
    for (int i = 1; i < 8; i++) {
        mi = glms_vec2_minv(mi, glms_vec2(points[i]));
        ma = glms_vec2_maxv(ma, glms_vec2(points[i]));
    }

    return AABBF_MM(mi, ma);
}

bool polygon_is_hole(
    vec2s vs[][2],
    int n_vs,
    vec2s ts[][2],
    int n_ts) {
    // ts form an inner hole iff the points directly next to them are all inside
    // of the polygon formed by { vs, ts }

    for (int i = 0; i < n_ts; i++) {
        const vec2s p0 = ts[i][0], p1 = ts[i][1];

        // compute q, point on normal of p0 -> p1 (left side normal!)
        const vec2s
            normal = glms_vec2_normalize(VEC2(-(p1.y - p0.y), p1.x - p0.x)),
            midpoint = glms_vec2_lerp(p0, p1, 0.5f),
            q =
                VEC2(
                    midpoint.x + 0.0001f * normal.x,
                    midpoint.y + 0.0001f * normal.y);

        // TODO: not for big polygons
        // compute a point such that q->u is a point outside of the polygon
        const vec2s u = VEC2(q.x - 1e15, q.y);

        // do a point-in-polygon test with q -> u
        int hits = 0;

        // test ts
        for (int j = 0; j < n_ts; j++) {
            if (!isnan(intersect_segs(q, u, ts[j][0], ts[j][1]).x)) {
                hits++;
            }
        }

        // test vs
        for (int j = 0; j < n_vs; j++) {
            if (!isnan(intersect_segs(q, u, vs[j][0], vs[j][1]).x)) {
                hits++;
            }
        }

        // even number of intersections -> outside of polygon, polygon cannot
        // be hole
        if (hits % 2 == 0) {
            return false;
        }
    }

    return true;
}
