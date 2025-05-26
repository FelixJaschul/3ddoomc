#include "level/path.h"
#include "level/level.h"
#include "level/level_defs.h"
#include "level/block.h"
#include "level/lptr.h"
#include "level/portal.h"
#include "level/sector.h"
#include "level/side.h"
#include "level/vertex.h"
#include "level/decal.h"
#include "level/wall.h"
#include "state.h"
#include "util/dynlist.h"
#include "util/sort.h"
/* #include "level/portal.h" */

// total number of allowed RETRY-s
#define RETRY_MAX 4

bool path_trace_resolve_portal(
    level_t *level,
    const path_hit_t *hit,
    vec2s *from,
    vec2s *to,
    int *res,
    f32 *angle,
    int flags) {
    if (!hit->wall.side->portal) {
        // no portal to hit
        return false;
    }

    if (hit->wall.side->portal
        && !(hit->wall.side->flags & SIDE_FLAG_DISCONNECT)) {
        // resolve as normal portal
        *res = PATH_TRACE_CONTINUE;
        *angle = 0.0f;
        return true;
    }

    // check that from -> to actually passes portal before portaling and is in
    // direction of this side
    if (!hit->wall.is_force_portal) {
        const vec2s intersect =
            intersect_segs(
                *from,
                *to,
                hit->wall.wall->v0->pos,
                hit->wall.wall->v1->pos);

        if (isnan(intersect.x)) {
            *res = PATH_TRACE_CONTINUE;
            return true;
        }

        // ignore this hit
        const vec2s dir = glms_vec2_normalize(glms_vec2_sub(*to, *from));
        if (glms_vec2_dot(dir, side_normal(hit->wall.side)) > 0.0f) {
            *res = PATH_TRACE_CONTINUE;
            return true;
        }
    }

    vertex_t *vs[2];
    side_get_vertices(hit->wall.side, vs);

    side_t
        *entry = hit->wall.side,
        *exit = hit->wall.side->portal;
    const f32 u = hit->wall.u;

    vertex_t *pvs[2];
    side_get_vertices(exit, pvs);

    *angle = portal_angle(level, entry, exit);

    vec2s
        phit = glms_vec2_lerp(pvs[0]->pos, pvs[1]->pos, 1.0f - u),
        n = side_normal(exit),
        new_from = phit,
        new_to = portal_transform(level, entry, exit, *to);

    // TODO: only do n modification for camera things
    // move to through portal
    if (flags & PATH_TRACE_RESOLVE_PORTAL_FORCE_AWAY) {
        new_from = glms_vec2_add(new_from, glms_vec2_scale(n, 0.05f));
        new_to = glms_vec2_add(new_to, glms_vec2_scale(n, 0.05f));
    }

    *from = new_from;
    *to = new_to;
    *res = PATH_TRACE_RETRY;
    return true;
}

bool path_trace_3d_resolve_portal(
    level_t *level,
    const path_hit_t *hit,
    vec3s *from,
    vec3s *to,
    int *res,
    f32 *angle,
    int flags) {
    const bool b =
        path_trace_resolve_portal(
            level, hit, (vec2s*) from, (vec2s*) to, res, angle, flags);

    // TODO: hacky way of finding out if things were updated
    if (*res == PATH_TRACE_RETRY) {
        // update z to z_t if moved ad update according to portal
        from->z = lerp(from->z, to->z, hit->t);

        // move z with disconnect
        if (hit->wall.side->flags & SIDE_FLAG_DISCONNECT) {
            const f32
                z_floor = hit->wall.side->sector->floor.z,
                nz_floor = hit->wall.side->portal->sector->floor.z;

            from->z -= z_floor - nz_floor;
            to->z -= z_floor - nz_floor;
        }

        // TODO: fix portal z
        // TODO: update z according to height difference
        /* const f32 d_z = */
        /*     hit->wall.side->portal->sector->floor.z */
        /*         - hit->wall.side->sector->floor.z; */

        /* from->z += d_z; */
        /* to->z += d_z; */
    }
    return b;
}

typedef struct {
    int res, flags;
    vec2s *from, *to;
    f32 radius;
    path_trace_resolve_f resolve;
    void *resolve_userdata;
    bool add_sectors;
} traverse_data_t;

static int path_hit_t_cmp(const path_hit_t *a, const path_hit_t *b, void*) {
    return (int) sign(a->t - b->t);
}

static bool path_trace_traverse(
    level_t *level,
    block_t *block,
    ivec2s,
    traverse_data_t *data) {

    // accumulate hits in list, need to sort by t
    DYNLIST(path_hit_t) hits = NULL;

    const vec2s
        delta = glms_vec2_sub(*data->to, *data->from),
        dir = glms_vec2_normalize(delta);

    dynlist_each(block->walls, it) {
        const wall_t *wall = *it.el;

        const vec2s
            a = wall->v0->pos,
            b = wall->v1->pos;

        vec2s swept_pos, hit;

        if (data->radius == 0.0f) {
            // 0 radius: intersect as point moving along line ("swept" point)
            hit = intersect_segs(*data->from, *data->to, a, b);
            if (isnan(hit.x)) {
                continue;
            }
            swept_pos = hit;
        } else {
            f32 t_circle, t_segment;
            vec2s resolved;
            if (!sweep_circle_line_segment(
                    *data->from,
                    data->radius,
                    delta,
                    a,
                    b,
                    &t_circle,
                    &t_segment,
                    &resolved)) {
                continue;
            }

            hit = glms_vec2_lerp(a, b, t_segment);
            swept_pos = resolved;
                /* glms_vec2_add( */
                    /* *data->from, */
                    /* glms_vec2_scale(delta, t_circle)); */
        }

        // if dot(dir, wall normal) is negative, then this is the left
        // side (wall normal side).
        // otherwise it is right side. this is also NULL when there is no
        // side, which is okay.
        const side_t *side =
            wall->sides[
                ((int) sign(glms_vec2_dot(dir, wall->normal))) <= 0 ? 0 : 1];

        if (!side) {
            continue;
        }

        vertex_t *vs[2];
        side_get_vertices(side, vs);

        const f32 hit_x = glms_vec2_norm(glms_vec2_sub(hit, vs[0]->pos));

        *dynlist_push(hits) = (path_hit_t) {
            .swept_pos = swept_pos,
            .type = T_WALL | T_SIDE,
            .t =
                glms_vec2_norm(glms_vec2_sub(hit, *data->from))
                    / glms_vec2_norm(glms_vec2_sub(*data->to, *data->from)),
            .wall = {
                .pos = hit,
                .u = hit_x / wall->len,
                .x = hit_x,
                .wall = (wall_t*) wall,
                .side = (side_t*) side
            }
        };

        // TODO
        /* if (flags & PATH_TRACE_ADD_DECALS) { */
        /*     // check affected decals */
        /*     llist_each(sidelist, &side->decals, it) { */
        /*         u16 oxmin, oxmax; */
        /*         l_decal_bounds(it.el, &oxmin, &oxmax); */

        /*         if (ox < oxmin || ox > oxmax) { continue; } */
        /*         LOG("got decal!"); */

        /*         path_hit_t decalhit = basehit; */
        /*         decalhit.decal = it.el; */

        /*         const int res = resolve(&decalhit, from, to, userdata); */

        /*         switch (res) { */
        /*         case PATH_TRACE_STOP: */
        /*             goto done; */
        /*         case RETRY: */
        /*             goto retry; */
        /*         case PATH_TRACE_CONTINUE:; */
        /*         } */
        /*     } */
        /* } */
    }

    if (data->add_sectors) {
        // check if line crosses any sectors, add them to hit check
        dynlist_each(block->sectors, it) {
            if (!sector_intersects_line(*it.el, *data->from, *data->to)) {
                continue;
            }

            *dynlist_push(hits) = (path_hit_t) {
                .swept_pos = *data->from,
                .t = 1.0f,
                .type = T_SECTOR,
                .sector = {
                    .ptr = *it.el,
                    .plane = -1
                }
            };
        }
    }

    if (data->flags & PATH_TRACE_ADD_OBJECTS) {
        dlist_each(block_list, &block->objects, it) {
            f32 t = 0.0f;

            if (data->radius == 0.0f) {
                f32 ts[2];
                int n;
                if (!(n = intersect_circle_seg(
                        it.el->pos, it.el->type->radius,
                        *data->to, *data->from,
                        ts, NULL))) {
                    continue;
                }

                t = ts[0];
            } else {
                if (!sweep_circle_circle(
                        it.el->pos, it.el->type->radius,
                        *data->from, data->radius,
                        glms_vec2_sub(*data->to, *data->from),
                        &t)) {
                    continue;
                }
            }

            *dynlist_push(hits) = (path_hit_t) {
                .swept_pos = glms_vec2_lerp(*data->from, *data->to, t),
                .t = t,
                .type = T_OBJECT,
                .object = {
                    .ptr = it.el
                }
            };
        }
    }

    // sort
    sort(
        hits,
        dynlist_size(hits),
        sizeof(hits[0]),
        (f_sort_cmp) path_hit_t_cmp,
        NULL);

    // traverse
    dynlist_each(hits, it) {
        const int res =
            data->resolve(
                level,
                it.el,
                data->from,
                data->to,
                data->resolve_userdata);

        switch (res) {
        case PATH_TRACE_STOP:
            data->res = PATH_TRACE_STOP;
            return false;
        case PATH_TRACE_RETRY:
            data->res = PATH_TRACE_RETRY;
            return false;
        case PATH_TRACE_CONTINUE:;
        }
    }

    dynlist_free(hits);
    return true;
}

void path_trace(
    level_t *level,
    vec2s *from,
    vec2s *to,
    f32 r,
    path_trace_resolve_f resolve,
    void *userdata,
    int flags) {
retry:;
    traverse_data_t data = {
        .flags = flags,
        .from = from,
        .to = to,
        .radius = r,
        .resolve = resolve,
        .resolve_userdata = userdata,
    };

    // force near portals if requested
    if ((flags & PATH_TRACE_FORCE_NEAR_PORTALS)
        && resolve) {
        const vec2s
            delta = glms_vec2_sub(*to, *from),
            dir = glms_vec2_normalize(delta);

        DYNLIST(wall_t*) near_walls = NULL;
        level_walls_in_radius(level, *to, 0.45f, &near_walls);

        dynlist_each(near_walls, it) {
            // get facing side
            side_t *s = NULL;
            for (int i = 0; !s && i < 2; i++) {
                if (!(*it.el)->sides[i]) { continue; }
                side_t *side = (*it.el)->sides[i];

                // only disconnected portals
                if (!(side->flags & SIDE_FLAG_DISCONNECT)) { continue; }

                // must be facing each other
                if (glms_vec2_dot(dir, side_normal(side)) > 0.0f) {
                    s = side;
                }
            }

            if (!s) { continue; }

            vertex_t *vs_in[2], *vs_out[2];
            side_get_vertices(s, vs_in);
            side_get_vertices(s->portal, vs_out);

            // TODO: bad, calculate most of thie elsewhere
            // only very very close (<= znear)
            const mat4s view_proj =
                glms_mat4_mul(state->cam.proj, state->cam.view);
            vec4s p =
                glms_mat4_mulv(
                    glms_mat4_inv(view_proj),
                    VEC4(0.5f, 0.5f, 0.0f, 1.0f));
            p.x /= p.w;
            p.y /= p.w;
            p.z /= p.w;
            const vec2s near_pos = VEC2(p.x, p.y);
            const f32 dist =
                point_to_segment(near_pos, vs_in[0]->pos, vs_in[1]->pos);

            if (dist > 0.05f) {
                continue;
            }

            const vec2s hit =
                intersect_lines(
                    *from,
                    *to,
                    vs_in[0]->pos,
                    vs_in[1]->pos);

            const f32 t_hit =
                glms_vec2_norm(
                    glms_vec2_sub(hit, vs_in[0]->pos))
                    / s->wall->len;

            const int res =
                resolve(
                    level,
                    &(path_hit_t) {
                        .swept_pos = *from,
                        .t =
                            glms_vec2_norm(glms_vec2_sub(hit, *from))
                                / glms_vec2_norm(glms_vec2_sub(*to, *from)),
                        .type = T_WALL | T_SIDE,
                        .wall = {
                            .pos = hit,
                            .u = t_hit,
                            .x =
                                glms_vec2_norm(
                                    glms_vec2_sub(hit, vs_in[0]->pos)),
                            .wall = *it.el,
                            .side = s,
                            .is_force_portal = true,
                        },
                    }, from, to, userdata);

            switch (res) {
            case PATH_TRACE_STOP:
                dynlist_free(near_walls);
                return;
            case PATH_TRACE_RETRY:
                dynlist_free(near_walls);
                goto retry;
            case PATH_TRACE_CONTINUE:;
            }
        }

        dynlist_free(near_walls);
    }

    int n_attempts = 0;
    while (true) {
        n_attempts++;

        if (n_attempts > RETRY_MAX) {
            return;
        }

        data.res = PATH_TRACE_CONTINUE;
        level_traverse_blocks(
            level,
            *from,
            *to,
            (traverse_blocks_f) path_trace_traverse,
            &data);

        switch (data.res) {
        case PATH_TRACE_RETRY: continue;
        case PATH_TRACE_STOP: return;
        }

        break;
    }
}

typedef struct {
    traverse_data_t data_2d;
    vec3s *from, *to;
    path_trace_3d_resolve_f resolve;
    void *resolve_userdata;
} traverse_data_3d_t;

static int path_trace_3d_resolve(
    level_t *level,
    const path_hit_t *hit,
    vec2s *_from,
    vec2s *_to,
    traverse_data_3d_t *data) {
    int res = PATH_TRACE_CONTINUE;

    if (hit->type & T_OBJECT) {
        res =
            data->resolve(
                level, hit, data->from, data->to, data->resolve_userdata);
        goto done;
    }

    sector_t *sector;
    plane_type plane;

    if (hit->type & T_SECTOR) {
        sector = hit->sector.ptr;

        f32 plane_z;

        // check if from -> to crosses each plane
        if (sign(data->from->z - sector->floor.z)
                != sign(data->to->z - sector->floor.z)) {
            plane_z = sector->floor.z;
            plane = PLANE_TYPE_FLOOR;
        } else if (
            sign(data->from->z - sector->ceil.z)
                != sign(data->to->z - sector->ceil.z)) {
            plane = PLANE_TYPE_CEIL;
            plane_z = sector->ceil.z;
        } else {
            // does not cross
            return PATH_TRACE_CONTINUE;
        }

        // does the cross occur inside of the sector?
        const f32 hit_t =
            (plane_z - data->from->z) / (data->to->z - data->from->z);

        const vec2s hit_pos =
            glms_vec2_lerp(
                glms_vec2(*data->from),
                glms_vec2(*data->to),
                hit_t);

        if (!sector_contains_point(sector, hit_pos)) {
            return PATH_TRACE_CONTINUE;
        }
    } else {
        sector = hit->wall.side->sector;
        const f32 z_t = lerp(data->from->z, data->to->z, hit->t);

        if (z_t < sector->floor.z) {
            plane = PLANE_TYPE_FLOOR;
        } else if (z_t > sector->ceil.z) {
            plane = PLANE_TYPE_CEIL;
        } else {
            // resolve regular side hit
            res =
                data->resolve(
                    level, hit, data->from, data->to, data->resolve_userdata);
            goto done;
        }
    }

    const f32
        plane_z = sector->planes[plane].z,
        hit_t = (plane_z - data->from->z) / (data->to->z - data->from->z);

    const path_hit_t plane_hit = {
        .swept_pos =
            glms_vec2_lerp(
                glms_vec2(*data->from),
                glms_vec2(*data->to),
                hit_t),
        .t = hit_t,
        .type = T_SECTOR,
        .sector = {
            .ptr = sector,
            .plane = plane
        }
    };

    res =
        data->resolve(
            level, &plane_hit, data->from, data->to, data->resolve_userdata);

    // resolve original hit if continuing, if it is a "true" (non-sector) hit
    if (res == PATH_TRACE_CONTINUE && !(hit->type & T_SECTOR)) {
        res =
            data->resolve(
                level, hit, data->from, data->to, data->resolve_userdata);
        goto done;
    }

done:
    /* *_from = glms_vec2(*data->from); */
    /* *_to = glms_vec2(*data->to); */
    return res;
}

void path_trace_3d(
    level_t *level,
    vec3s *from,
    vec3s *to,
    f32 r,
    path_trace_3d_resolve_f resolve,
    void *userdata,
    int flags) {
    traverse_data_3d_t data = {
        .data_2d = {
            .flags = flags,
            .from = (vec2s*) from,
            .to = (vec2s*) to,
            .radius = r,
            .resolve = (path_trace_resolve_f) path_trace_3d_resolve,
            .resolve_userdata = NULL,
            .add_sectors = true,
        },
        .resolve = resolve,
        .resolve_userdata = userdata,
        .from = from,
        .to = to
    };

    // path_trace_3d_resolve needs pointer to data
    data.data_2d.resolve_userdata = &data;

    int n_attempts = 0;
    while (true) {
        n_attempts++;

        if (n_attempts > RETRY_MAX) {
            return;
        }

        data.data_2d.res = PATH_TRACE_CONTINUE;
        level_traverse_blocks(
            level,
            glms_vec2(*from),
            glms_vec2(*to),
            (traverse_blocks_f) path_trace_traverse,
            &data.data_2d);

        switch (data.data_2d.res) {
        case PATH_TRACE_RETRY: continue;
        case PATH_TRACE_STOP: return;
        }

        break;
    }
}
