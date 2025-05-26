#include "level/sector.h"
#include "editor/editor.h"
#include "level/block.h"
#include "level/decal.h"
#include "level/level.h"
#include "level/lptr.h"
#include "level/particle.h"
#include "level/side.h"
#include "level/tag.h"
#include "util/hash.h"
#include "util/map.h"
#include "util/rand.h"
#include "state.h"

#include <glutess.h>

#define SUBSECTOR_ID_INVALID -1

static subsector_t sect_tri_to_sub(const sect_tri_t *tri) {
    subsector_t s = { .id = SUBSECTOR_ID_INVALID };
    *dynlist_push(s.lines) = (sect_line_t) { .a = tri->a, .b = tri->b };
    *dynlist_push(s.lines) = (sect_line_t) { .a = tri->b, .b = tri->c };
    *dynlist_push(s.lines) = (sect_line_t) { .a = tri->c, .b = tri->a };
    return s;
}

// returns index of adjacent line in a if one is found, otherwise returns -1
static int subsectors_find_adjacent(
    const subsector_t *a,
    const subsector_t *b,
    int *other_index) {
    // adjacent if any one line segment is equal to another on the other
    // subsector
    dynlist_each(a->lines, it_a) {
        dynlist_each(b->lines, it_b) {
            if (sect_line_eq(it_a.el, it_b.el)) {
                if (other_index) { *other_index = it_b.i; }
                return it_a.i;
            }
        }
    }

    return -1;
}

static void combine_subsectors(
    const subsector_t *a, const subsector_t *b, subsector_t *out) {
    ASSERT(a != b);

    subsector_t s = { .id = SUBSECTOR_ID_INVALID };

    // add all lines except shared ones
    dynlist_each(a->lines, it_a) {
        dynlist_each(b->lines, it_b) {
            if (sect_line_eq(it_a.el, it_b.el)) {
                // tag shared side
                it_a.el->tag = true;
                it_b.el->tag = true;
            }
        }
    }

    dynlist_each(a->lines, it) {
        if (!it.el->tag) {
            *dynlist_push(s.lines) = *it.el;
        }
        it.el->tag = false;
    }

    dynlist_each(b->lines, it) {
        if (!it.el->tag) {
            *dynlist_push(s.lines) = *it.el;
        }
        it.el->tag = false;
    }

    ASSERT(dynlist_size(s.lines) >= 3);

    *out = s;
}

static bool subsector_traverse_block_neighbors(
    level_t *level,
    block_t *block,
    ivec2s,
    subsector_t *sub) {
    // check adjacency with other subsectors in block
    dynlist_each(block->subsectors, it) {
        if (*it.el == sub->id) { continue; }

        subsector_t *other = level->subsectors[*it.el];

        // check that we are not already neighbors
        bool found = false;
        dynlist_each(sub->neighbors, it_n) {
            if (it_n.el->id == other->id) {
                found = true;
                break;
            }
        }

        if (found) {
            continue;
        }

        int line = -1, other_line = -1;
        if ((line = subsectors_find_adjacent(sub, other, &other_line)) != -1) {
            *dynlist_push(sub->neighbors) =
                (subsector_neighbor_t) {
                    .id = other->id,
                    .line = &sub->lines[line]
                };

            *dynlist_push(other->neighbors) =
                (subsector_neighbor_t) {
                    .id = sub->id,
                    .line = &other->lines[other_line]
                };
        }
    }

    return true;
}

static void subsector_finalize(
    level_t *level, sector_t *sector, subsector_t *s) {
    s->parent = sector;

    // try and find free ID
    int id;
    if ((id =
            bitmap_find(
                level->subsector_ids,
                dynlist_size(level->subsectors),
                0, false)) == INT_MAX) {
        // expand table, take most recently added ID
        id = dynlist_size(level->subsectors);

        // expand by 32 elements
        dynlist_resize(level->subsectors, dynlist_size(level->subsectors) + 32);
        level->subsector_ids =
            bitmap_realloc_zero(
                level->subsector_ids,
                id,
                dynlist_size(level->subsectors));
    }

    s->id = id;
    bitmap_set(level->subsector_ids, id);
    level->subsectors[id] = s;

    // calc min/max
    s->min = s->lines[0].a->pos;
    s->max = s->lines[0].b->pos;
    dynlist_each(s->lines, it) {
        s->min = glms_vec2_minv(s->min, it.el->a->pos);
        s->min = glms_vec2_minv(s->min, it.el->b->pos);
        s->max = glms_vec2_maxv(s->max, it.el->a->pos);
        s->max = glms_vec2_maxv(s->max, it.el->b->pos);
    }

    // update in blocks
    level_set_subsector_blocks(level, s);
}

static void subsector_destroy(level_t *level, subsector_t *s) {
    // remove this subsector from its neighbors
    dynlist_each(s->neighbors, it) {
        subsector_t *sub = level->subsectors[it.el->id];
        DYNLIST(subsector_neighbor_t) *ns = &sub->neighbors;

        bool found = false;
        dynlist_each(*ns, it_n) {
            if (it_n.el->id == s->id) {
                dynlist_remove_it(*ns, it_n);
                found = true;
                break;
            }
        }

        ASSERT(found);
    }

    // remove from blocks
    level_blocks_remove_subsector(level, s);

    dynlist_free(s->lines);
    dynlist_free(s->neighbors);

    if (s->id != SUBSECTOR_ID_INVALID) {
        ASSERT(level->subsectors[s->id] == s);
        bitmap_clr(level->subsector_ids, s->id);
        level->subsectors[s->id] = NULL;
    }

    // check if we can shrink entire level's subsector ID list
    if (dynlist_size(level->subsectors) > 32
        && bitmap_find(
            level->subsector_ids,
            dynlist_size(level->subsectors),
            dynlist_size(level->subsectors) - 32,
            true) == INT_MAX) {
        // TODO: evt. check that all remaining entries in list are NULL ?
        dynlist_resize(level->subsectors, dynlist_size(level->subsectors) - 32);
        level->subsector_ids =
            bitmap_realloc(
                level->subsector_ids,
                dynlist_size(level->subsectors));
    }
}

enum {
    TRACE_LINES_NONE = 0,
    TRACE_LINES_CONVEX_ONLY = 1 << 0
};

static bool trace_lines(
    const DYNLIST(sect_line_t) *lines,
    const sect_line_t *start,
    DYNLIST(const sect_line_t*) *out,
    int flags) {
    // sect_line_t* -> void*
    map_t visited;
    map_init(
        &visited,
        map_hash_id,
        NULL,
        NULL,
        NULL,
        map_cmp_id,
        NULL,
        NULL,
        NULL);

    bool res = true;

    const sect_line_t *line = start;
    while (line) {
        if (map_find(sect_line_t*, &visited, line)) {
            res = line == start;
            goto done;
        }

        if (out) { *dynlist_push(*out) = line; }
        map_insert(&visited, line, NULL);

        f32 angle_min = TAU;
        const sect_line_t *next = NULL;

        for (int j = 0; j < dynlist_size(*lines); j++) {
            const sect_line_t *other = &(*lines)[j];

            // ignore current
            if (other == line) { continue; }

            // check only for lines where other start == line end
            if (!glms_vec2_eqv_eps(other->a->pos, line->b->pos)) {
                continue;
            }

            // find angle between this line and next line
            const f32 angle =
                angle_in_points(
                    line->a->pos,
                    other->a->pos,
                    other->b->pos);

            if (angle < angle_min) {
                next = other;
                angle_min = angle;
            }
        }

        if ((flags & TRACE_LINES_CONVEX_ONLY) && angle_min > (PI + 0.001f)) {
            res = false;
            goto done;
        }

        if (!next) { res = false; goto done; }
        line = next;
    }

done:
    map_destroy(&visited);
    return res;
}

static bool subsector_is_convex(const subsector_t *s) {
    return trace_lines(&s->lines, &s->lines[0], NULL, TRACE_LINES_CONVEX_ONLY);
}

typedef struct {
    int id;
    subsector_t sub;
} convexify_key_t;

static hash_t convexify_key_hash(const convexify_key_t *k, void*) {
    return k->id;
}

static int convexify_key_cmp(
    const convexify_key_t *a,
    const convexify_key_t *b,
    void*) {
    return a->id - b->id;
}

// combine triangles until a reasonable convex subset has been made (shapes)
static bool convexify_tris(
    level_t *level,
    DYNLIST(sect_tri_t) *tris,
    DYNLIST(subsector_t) *out) {
    // convexify_key_t* -> void*
    map_t working_set;
    map_init(
        &working_set,
        (f_map_hash) convexify_key_hash,
        NULL, NULL, NULL,
        (f_map_cmp) convexify_key_cmp,
        NULL, NULL, NULL);

    bool res = true;

    // convert all triangles -> shapes
    int id = 0;
    dynlist_each(*tris, it) {
        convexify_key_t *k = malloc(sizeof(convexify_key_t));
        *k = (convexify_key_t) { id++, sect_tri_to_sub(it.el) };
        map_insert(&working_set, k, 0);
    }

    // sanity check
    // TODO: reenable if we ever doubt glutess
    /* map_each(convexify_key_t*, int, &working_set, it) { */
    /*     if (!subsector_is_convex(&it.key->sub)) { */
    /*         WARN("non-convex!"); */
    /*         res = false; */
    /*         goto done; */
    /*     } */
    /* } */

    while (map_size(&working_set) != 0) {
        convexify_key_t *key = NULL;
        map_each(convexify_key_t*, int, &working_set, it) {
            key = it.key;
            map_remove(&working_set, it.key);
            break;
        }

        bool merged = false;

        // find adjacent shapes until we can merge
        map_each(convexify_key_t*, int, &working_set, it) {
            const int line =
                subsectors_find_adjacent(&it.key->sub, &key->sub, NULL);
            if (line == -1) {
                continue;
            }

            // check if a merger would be convex
            subsector_t merger = { .id = SUBSECTOR_ID_INVALID };
            combine_subsectors(&key->sub, &it.key->sub, &merger);

            if (subsector_is_convex(&merger)) {
                subsector_destroy(level, &key->sub);
                subsector_destroy(level, &it.key->sub);
                map_remove(&working_set, it.key);
                free(key);
                free(it.key);

                convexify_key_t *k = malloc(sizeof(convexify_key_t));
                *k = (convexify_key_t) { id++, merger };
                map_insert(&working_set, k, 0);

                // shape has been consumed
                merged = true;
                break;
            }
        }

        // not possible to merge -> final sub!
        if (!merged) {
            *dynlist_push(*out) = key->sub;
            free(key);
        }
    }

/* done: */
    map_destroy(&working_set);
    return res;
}

typedef struct {
    GLdouble data[3];
    vertex_t *v;
} tess_vertex_t;

typedef struct {
    GLenum primitive;
    DYNLIST(vertex_t*) vertices;
    DYNLIST(sect_tri_t) tris;
} tess_ctx_t;

static tess_ctx_t tess_ctx;

void tess_begin(GLenum which, void*) {
    tess_ctx.primitive = which;
    dynlist_resize(tess_ctx.vertices, 0);
}

void tess_vertex(const tess_vertex_t *tv) {
    *dynlist_push(tess_ctx.vertices) = tv->v;
}

void tess_end() {
    switch (tess_ctx.primitive) {
    case GL_TRIANGLES: {
        ASSERT(dynlist_size(tess_ctx.vertices) % 3 == 0);
        for (int i = 0; i < dynlist_size(tess_ctx.vertices); i += 3) {
            *dynlist_push(tess_ctx.tris) = (sect_tri_t) {
                .vs = {
                tess_ctx.vertices[i + 0],
                tess_ctx.vertices[i + 1],
                tess_ctx.vertices[i + 2],
                }
            };
        }
    } break;
    case GL_TRIANGLE_STRIP: {
        for (int i = 0; i < dynlist_size(tess_ctx.vertices) - 2; i++) {
            if (i % 2 == 0) {
                *dynlist_push(tess_ctx.tris) = (sect_tri_t) {
                .vs = {
                    tess_ctx.vertices[i + 1],
                    tess_ctx.vertices[i + 2],
                    tess_ctx.vertices[i + 0],
                }
                };
            } else {
                *dynlist_push(tess_ctx.tris) = (sect_tri_t) {
                .vs = {
                    tess_ctx.vertices[i + 2],
                    tess_ctx.vertices[i + 1],
                    tess_ctx.vertices[i + 0],
                }
                };
            }
        }
    } break;
    case GL_TRIANGLE_FAN: {
        vertex_t *base = tess_ctx.vertices[0];
        for (int i = 1; i < dynlist_size(tess_ctx.vertices) - 1; i++) {
            *dynlist_push(tess_ctx.tris) = (sect_tri_t) {
                .vs = {
                    base,
                    tess_ctx.vertices[i + 0],
                    tess_ctx.vertices[i + 1],
                }
            };
        }
    } break;
    }
}

sector_t *sector_new(
    level_t *level,
    const sector_t *like) {
    sector_t *s = level_alloc(level, level->sectors);
    s->version = 1;

    if (like) {
        memcpy(s->planes, like->planes, sizeof(s->planes));
        s->base_light = like->base_light;
        s->cos_type = like->cos_type;
        s->func_type = like->func_type;
        s->flags = like->flags;
        s->mat = like->mat;
    } else {
        s->mat = level->sectmats[SECTMAT_NOMAT];
    }

    return s;
}

static bool traverse_compute_visible(level_t *level, sector_t *sector, void*) {
    *dynlist_push(level->dirty_vis_sectors) = LPTR_FROM(sector);
    return true;
}

void sector_delete(level_t *level, sector_t *s) {
    // mark so that sector is not deleted from side removal
    s->level_flags |= LF_MARK;

#ifdef MAPEDITOR
    if (state->mode == GAMEMODE_EDITOR) {
        editor_close_for_ptr(state->editor, LPTR_FROM(s));
    }
#endif //ifdef MAPEDITOR

    // remove from blocks before min, max are recalculated
    level_blocks_remove_sector(level, s);

    // TODO
    for (int by = level->blocks.offset.y;
         by < level->blocks.offset.y + level->blocks.size.y;
         by++) {
        for (int bx = level->blocks.offset.y;
             bx < level->blocks.offset.y + level->blocks.size.y;
             bx++) {
            block_t *block = level_get_block(level, IVEC2(bx, by));
            if (!block) { continue; } // TODO:!!!

            dynlist_each(block->sectors, it) {
                ASSERT(
                    *it.el != s,
                    "failed to remove sector %d from blocks",
                    s->index);
            }
        }
    }

    // unlink sector from all sides
    if (s->n_sides > 0) {
        side_t *sides[s->n_sides];
        const int n_sides = sector_get_sides(level, s, sides, s->n_sides);

        for (int i = 0; i < n_sides; i++) {
            sides[i]->level_flags |= LF_DO_NOT_RECALC;
        }

        for (int i = 0; i < n_sides; i++) {
            sector_remove_side(level, s, sides[i]);
        }

        for (int i = 0; i < n_sides; i++) {
            sides[i]->level_flags &= ~LF_DO_NOT_RECALC;
            side_recalculate(level, sides[i]);
        }
    }

    // delete decals
    llist_each(node, &s->decals, it) {
        decal_delete(level, it.el);
    }

    // unlink sector objects
    dlist_each(sector_list, &s->objects, it) {
        dlist_remove(sector_list, &s->objects, it.el);
        it.el->sector = NULL;
    }

    dynlist_each(s->subs, it) {
        subsector_destroy(level, it.el);
    }
    dynlist_free(s->subs);

    // force neighbors to recalcualte visibility on next update
    sector_traverse_neighbors(level, s, traverse_compute_visible, NULL);

    // force neighbors to recalc
    dynlist_each(s->neighbors, it) {
        sector_recalculate(level, *it.el);
    }
    dynlist_free(s->neighbors);

    // TODO: will break because delete modifies list
    dynlist_each(s->particles, it) {
        particle_delete(level, &level->particles[*it.el]);
    }
    dynlist_free(s->particles);

    tag_set(level, LPTR_FROM(s), TAG_NONE);
    level_free(level, level->sectors, s);
}

void sector_tick(level_t *level, sector_t *sector) {
    if ((sector->flags & SECTOR_FLAG_ANY_FLASH)) {
        sector->flags =
            (sector->flags & ~SECTOR_FLAG_ANY_FLASH)
            | (((int) ((u32) sector->flags >> 1)) & SECTOR_FLAG_ANY_FLASH);
    }

    llist_each(node, &sector->decals, it) {
        decal_tick(level, it.el);
    }

    llist_each(sector_sides, &sector->sides, it) {
        llist_each(node, &it.el->decals, it_d) {
            decal_tick(level, it_d.el);
        }
    }
}

bool sector_contains_vertex(
    const level_t *level,
    const sector_t *sector,
    const vertex_t *vertex) {
    llist_each(sector_sides, &sector->sides, it) {
        vertex_t *vs[2];
        side_get_vertices(it.el, vs);
        if (vertex == vs[0] || vertex == vs[1]) { return true; }
    }
    return false;
}

bool sector_contains_point(
    const sector_t *sector,
    vec2s p) {
    if (p.x < sector->min.x
        || p.x > sector->max.x
        || p.y < sector->min.y
        || p.y > sector->max.y) {
        return false;
    }

    // find subsector
    dynlist_each(sector->subs, it) {
        subsector_t *sub = it.el;
        if (p.x >= sub->min.x
            && p.x <= sub->max.x
            && p.y >= sub->min.y
            && p.y <= sub->max.y) {
            dynlist_each(sub->lines, it_l) {
                if (point_side(p, it_l.el->a->pos, it_l.el->b->pos) < 0) {
                    goto next;
                }
            }
            return true;
        }
next:
    }

    return false;
}

vec2s sector_clamp_point(const sector_t *sector, vec2s point) {
    // clamp to each subsector, pick nearest
    vec2s p = point;
    f32 d_p = 1e10;

    dynlist_each(sector->subs, it) {
        subsector_t *sub = it.el;

        bool inside = true;
        dynlist_each(sub->lines, it_l) {
            // clamp to this line
            const vec2s q =
                point_project_segment(
                    point, it_l.el->a->pos, it_l.el->b->pos);
            const f32 d_q = glms_vec2_norm(glms_vec2_sub(q, point));
            if (d_q < d_p) {
                p = q;
                d_p = d_q;
            }

            if (point_side(point, it_l.el->a->pos, it_l.el->b->pos) < 0) {
                inside = false;
            }
        }

        if (inside) {
            return point;
        }
    }

    return p;
}

bool sector_intersects_line(
    const sector_t *sector,
    vec2s a,
    vec2s b) {
    if (sector_contains_point(sector, a)) { return true; }
    else if (sector_contains_point(sector, b)) { return true; }

    dynlist_each(sector->subs, it_s) {
        dynlist_each(it_s.el->lines, it_l) {
            if (
                !isnan(
                    intersect_segs(a, b, it_l.el->a->pos, it_l.el->b->pos).x)) {
                return true;
            }
        }
    }

    return false;
}

int sector_get_sides(
    struct level *level,
    struct sector *sector,
    struct side **sides,
    int n_sides) {
    ASSERT(n_sides >= sector->n_sides);

    int i = 0;
    llist_each(sector_sides, &sector->sides, it) {
        sides[i++] = it.el;
    }
    ASSERT(i == sector->n_sides);
    return i;
}

void sector_delete_with_sides(level_t *level, sector_t *s) {
    // mark so that sector is not deleted from side_delete
    s->level_flags |= LF_MARK;

    // remove all sides for this sector (this will also update portals)
    if (s->n_sides > 0) {
        const int n_sides = s->n_sides;

        side_t *sides[n_sides];
        sector_get_sides(level, s, sides, n_sides);

        for (int i = 0; i < n_sides; i++) {
            side_delete(level, sides[i]);
        }
    }

    sector_delete(level, s);
}

void sector_traverse_neighbors(
    level_t *level,
    sector_t *sector,
    sector_traverse_neighbors_f callback,
    void *userdata) {
    map_t visited;
    map_init(
        &visited, map_hash_id, NULL, NULL, NULL, map_cmp_id, NULL, NULL, NULL);

    DYNLIST(sector_t*) queue = NULL;
    *dynlist_push(queue) = sector;

    while (dynlist_size(queue) != 0) {
        sector_t *s = dynlist_pop(queue);
        map_insert(&visited, s, NULL);

        if (!callback(level, s, userdata)) {
            return;
        }

        dynlist_each(s->neighbors, it) {
            if (!map_find(&visited, *it.el)) {
                *dynlist_push(queue) = *it.el;
            }
        }
    }

    dynlist_free(queue);
    map_destroy(&visited);
}

// #define DO_DEBUG_VIS

#ifdef DO_DEBUG_VIS
#define DEBUG_VIS(...) LOG(__VA_ARGS__)
#else
#define DEBUG_VIS(...)
#endif // ifdef DO_DEBUG_VIS

// TODO: doc and cleanup
static void calculate_splitter(
    const sect_line_t *source,
    const sect_line_t *pass,
    vec2s *a, vec2s *b) {
    bool found = false;
    const vec2s
        ms = glms_vec2_lerp(source->a->pos, source->b->pos, 0.5f),
        mp = glms_vec2_lerp(pass->a->pos, pass->b->pos, 0.5f);

    UNUSED int lcase = -1, picked = -1;
    bool colinear[4] = { 0 };

    if (glms_vec2_eqv_eps(source->a->pos, pass->a->pos)) {
        lcase = 0;
        *a = source->a->pos;
        *b = glms_vec2_lerp(source->b->pos, pass->b->pos, 0.5f);
        if (point_side(pass->b->pos, *a, *b) < 0) { swap(*a, *b); }
        found = true;
        goto done;
    } else if (glms_vec2_eqv_eps(source->b->pos, pass->a->pos)) {
        lcase = 1;
        *a = source->b->pos;
        *b = glms_vec2_lerp(source->a->pos, pass->b->pos, 0.5f);
        if (point_side(pass->b->pos, *a, *b) < 0) { swap(*a, *b); }
        found = true;
        goto done;
    } else if (glms_vec2_eqv_eps(source->a->pos, pass->b->pos)) {
        lcase = 2;
        *a = source->a->pos;
        *b = glms_vec2_lerp(source->b->pos, pass->a->pos, 0.5f);
        if (point_side(pass->a->pos, *a, *b) < 0) { swap(*a, *b); }
        found = true;
        goto done;
    } else if (glms_vec2_eqv_eps(source->b->pos, pass->b->pos)) {
        lcase = 3;
        *a = source->b->pos;
        *b = glms_vec2_lerp(source->a->pos, pass->a->pos, 0.5f);
        if (point_side(pass->a->pos, *a, *b) < 0) { swap(*a, *b); }
        found = true;
        goto done;
    }

    // find potential splitter between source and pass
    const vec2s ls[4][2] = {
        { source->a->pos, pass->a->pos, },
        { source->a->pos, pass->b->pos, },
        { source->b->pos, pass->a->pos, },
        { source->b->pos, pass->b->pos, },
    };

    for (int i = 0; i < 4; i++) {
        // don't pick splitters which are colinear with one of our lines
        if (segments_are_colinear(
                ls[i][0], ls[i][1], source->a->pos, source->b->pos)
            || segments_are_colinear(
                ls[i][0], ls[i][1], pass->a->pos, pass->b->pos)) {
            colinear[i] = true;
            continue;
        }

        const f32
            ss = point_side(ms, ls[i][0], ls[i][1]),
            sp = point_side(mp, ls[i][0], ls[i][1]);

        if (fabsf(sign(ss) - sign(sp)) > GLM_FLT_EPSILON) {
            picked = i;

            *a = ls[i][0];
            *b = ls[i][1];

            if (sp < 0) {
                // swap such that "pass" is positive
                swap(*a, *b);
            }

            found = true;
            break;
        }
    }

done:
    if (!found
        || point_side(ms, *a, *b) > 0
        || point_side(mp, *a, *b) < 0) {
        DEBUG_VIS("could not split");
        DEBUG_VIS(
            "\n%" PRIv2 ", %" PRIv2 " \n"
            "%" PRIv2 ", %" PRIv2 " \n"
            "%" PRIv2 ", %" PRIv2 " \n"
            "case: %d\n"
            "colienar: %d %d %d %d\n"
            "picked: %d",
            FMTv2(source->a->pos), FMTv2(source->b->pos),
            FMTv2(pass->a->pos), FMTv2(pass->b->pos),
            FMTv2(*a), FMTv2(*b),
            lcase,
            colinear[0], colinear[1], colinear[2], colinear[3],
            picked);
    }
}

// check if target is visible through "through" from start
static bool is_line_visible_via(
    const sect_line_t *source,
    const sect_line_t *pass,
    const sect_line_t *target,
    vec2s a, vec2s b) {
    // "target" is visible iff  either of its points lie to the left of the
    // splitter a -> b (same side as pass)
    return point_side(target->a->pos, a, b) >= 0
        || point_side(target->b->pos, a, b) >= 0;
}

void sector_compute_visibility(level_t *level, sector_t *sector) {
    if (dynlist_size(sector->subs) == 0) { return; }

    {
        // ensure capacity of visibility matrix
        const int
            n_old = level->visibility.n,
            n_new = round_up_to_mult(dynlist_size(level->sectors), 64);

        if (n_old != n_new) {
            u8 *old = level->visibility.matrix;

            level->visibility.matrix =
                calloc(1, n_new * BITMAP_SIZE_TO_BYTES(n_new));
            level->visibility.n = n_new;

            if (old) {
                // copy rows again
                for (int i = 0; i < n_old; i++) {
                    memcpy(
                        &level->visibility.matrix[
                            i * BITMAP_SIZE_TO_BYTES(n_new)],
                        &old[i * BITMAP_SIZE_TO_BYTES(n_old)],
                        BITMAP_SIZE_TO_BYTES(n_old));
                }
            }

            if (old) {
                free(old);
            }
        }
    }

    BITMAP *bits =
        &level->visibility.matrix[
            sector->index * BITMAP_SIZE_TO_BYTES(level->visibility.n)];
    bitmap_fill(bits, dynlist_size(level->sectors), false);

    // sectors are always visible from themselves
    bitmap_set(bits, sector->index);

    // check if sector is visible via source through "through"
    typedef struct queue_entry {
        int depth;

        // subsector to test outgoing portals of
        subsector_t *sub;

        // origin line
        sect_line_t *source;

        // line through which "sub" was entered
        sect_line_t *through;
    } queue_entry_t;

    DYNLIST(queue_entry_t) queue = NULL;

    // start from an arbitrary subsector, enqueue all of its neighbors
    dynlist_each(sector->subs[0].neighbors, it) {
        level->subsectors[it.el->id]->from = NULL;
        *dynlist_push(queue) = (queue_entry_t) {
            .depth = 1,
            .sub = level->subsectors[it.el->id],
            .source = it.el->line,
            .through = it.el->line
        };
    }

    while (dynlist_size(queue) != 0) {
        queue_entry_t entry = dynlist_pop(queue);

        // this entry is visible
        bitmap_set(bits, entry.sub->parent->index);

        vec2s a = VEC2(0), b = VEC2(0);

        if (entry.depth > 2) {
            calculate_splitter(entry.source, entry.through, &a, &b);
        }

        // check neighbors for visibility
        dynlist_each(entry.sub->neighbors, it) {
            // check that we did not come from this neighbor
            subsector_t *s = entry.sub;
            bool found = false;
            while (s) {
                if (s == level->subsectors[it.el->id]) {
                    found = true;
                }
                s = s->from;
            }
            if (found) { continue; }

            if (sect_line_eq(entry.source, it.el->line)) {
                continue;
            }

            // if early depth (< 2) always enqueue
            // check if the outgoing line to this neighbor is visible via the
            // entry line.
            if (entry.depth > 2
                && !is_line_visible_via(
                        entry.source,
                        entry.through,
                        it.el->line,
                        a, b)) {
                continue;
            }

            // if so, enqueue it
            level->subsectors[it.el->id]->from = entry.sub;
            *dynlist_push(queue) = (queue_entry_t) {
                .depth = entry.depth + 1,
                .sub = level->subsectors[it.el->id],
                .source = entry.source,
                .through = it.el->line
            };
        }
    }

    dynlist_free(queue);
}

void sector_recalculate(level_t *level, sector_t *sector) {
    if (sector->level_flags & LF_DO_NOT_RECALC) { return; }
    level->version++;
    sector->version++;

    // TODO: clamp all decals

    ASSERT(!(sector->level_flags & LF_MARK));

    vec2s p_min = VEC2(1e10), p_max = VEC2(0);
    int n_sides = 0;

    bool failed = false;

    dynlist_resize(sector->neighbors, 0);

    // validate sides, calculate neighbors
    llist_each(sector_sides, &sector->sides, it) {
        side_t *s = it.el;
        ASSERT(s->sector == sector);
        ASSERT(!(s->level_flags & LF_MARK));

        wall_t *w = s->wall;
        const vec2s w_a = w->v0->pos, w_b = w->v1->pos;

        if (!w) { WARN("wall-less side %d", w->index); return; }
        p_min.x = min(p_min.x, min(w_a.x, w_b.x));
        p_min.y = min(p_min.y, min(w_a.y, w_b.y));
        p_max.x = max(p_max.x, max(w_a.x, w_b.x));
        p_max.y = max(p_max.y, max(w_a.y, w_b.y));
        n_sides++;

        // additionally - any sectors we border need to have version-based data
        // invalidated now that their portals have (possibly) changed size
        if (s->portal && s->portal->sector) {
            s->portal->sector->version++;

            // add to neighbors if not marked and mark
            if (!(s->portal->sector->level_flags & LF_MARK)
                && !(s->flags & SIDE_FLAG_DISCONNECT)) {
                s->portal->sector->level_flags |= LF_MARK;
                *dynlist_push(sector->neighbors) = s->portal->sector;
            }
        }
    }

    // un-mark neighbors
    dynlist_each(sector->neighbors, it) {
        (*it.el)->level_flags &= ~LF_MARK;
    }

    if (n_sides == 0) {
        LOG("removing empty sector %d", sector->index);
        sector_delete(level, sector);
        return;
    }

    sector->n_sides = n_sides;

    // sort sides along traces
    int n_sorted = 0;
    side_t *sides[sector->n_sides], *sorted_sides[sector->n_sides];
    sector_get_sides(level, sector, sides, sector->n_sides);

    tess_vertex_t tvs[sector->n_sides * 2];

    // re-tesselate
    dynlist_resize(sector->tris, 0);

    // remove sector subs from level lists
    dynlist_each(sector->subs, it) {
        subsector_destroy(level, it.el);
    }
    dynlist_resize(sector->subs, 0);

    dynlist_resize(tess_ctx.vertices, 0);
    dynlist_resize(tess_ctx.tris, 0);

    GLUtesselator *ts = gluNewTess();
    gluTessProperty(ts, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE);
    gluTessCallback(ts, GLU_TESS_BEGIN, (GLvoid (*)()) &tess_begin);
    gluTessCallback(ts, GLU_TESS_VERTEX, (GLvoid (*)()) &tess_vertex);
    gluTessCallback(ts, GLU_TESS_END, (GLvoid (*)()) &tess_end);
    gluTessNormal(ts, 0.0f, 0.0f, 1.0f);
    gluTessBeginPolygon(ts, NULL);

    int count = 0;

    while (n_sorted != sector->n_sides) {
        // find non-sorted (non-marked) side
        side_t *start = NULL;
        for (int i = 0; i < sector->n_sides; i++) {
            if (!(sides[i]->level_flags & LF_MARK)) {
                start = sides[i];
                break;
            }
        }

        // TODO: maybe break
        ASSERT(start);

        DYNLIST(side_t*) trace = NULL;
        if (!level_trace_sides(level, start, &trace)) {
            WARN(
                "failure to construct coherent sector (trace starting from %d)",
                start->index);
            failed = true;
            goto done_sorting;
        }

        if (n_sorted + dynlist_size(trace) > sector->n_sides) {
            WARN(
                "too many traced sides for sector (have %d cannot add %d max %d)",
                n_sorted,
                dynlist_size(trace),
                sector->n_sides);
            failed = true;
            goto done_sorting;
        }

        // insert trace into sorted sides in order while marking AND inserting
        // as contour
        gluTessBeginContour(ts);
        dynlist_each(trace, it) {
            ASSERT((*it.el)->sector == sector);

            if ((*it.el)->level_flags & LF_MARK) {
                WARN("traced into already found side %d", (*it.el)->index);
                failed = true;
                goto done_sorting;
            }

            (*it.el)->level_flags |= LF_MARK;
            sorted_sides[n_sorted++] = *it.el;

            // insert into contour
            vertex_t *vs[2];
            side_get_vertices(*it.el, vs);

            tvs[count] = (tess_vertex_t) {
                .data = {
                    vs[0]->pos.x,
                    vs[0]->pos.y,
                    0.0
                },
                .v = vs[0]
            };
            gluTessVertex(ts, tvs[count].data, &tvs[count]);
            count++;
        }
        gluTessEndContour(ts);

        dynlist_free(trace);
    }

done_sorting:
    if (failed) {
        // unmark all
        for (int i = 0; i < sector->n_sides; i++) {
            sides[i]->level_flags &= ~LF_MARK;
        }

        gluDeleteTess(ts);

        WARN("failed to tesselate sector %d", sector->index);
        return;
    }

    // insert in reverse order, as we are prepending
    llist_init(&sector->sides);
    int m = 0;
    for (int i = n_sorted - 1; i >= 0; i--) {
        llist_init_node(&sorted_sides[i]->sector_sides);
        llist_prepend(sector_sides, &sector->sides, sorted_sides[i]);
        sorted_sides[i]->level_flags &= ~LF_MARK;
        m++;
    }

    ASSERT(m == sector->n_sides);

    // sides are now sorted and tesselated
    gluTessEndPolygon(ts);
    gluDeleteTess(ts);

    // copy tris -> sector tris
    dynlist_each(tess_ctx.tris, it) {
        *dynlist_push(sector->tris) = (sect_tri_t) {
            .vs = {
                it.el->a,
                it.el->b,
                it.el->c,
            }
        };
    }

    // convexify
    convexify_tris(level, &sector->tris, &sector->subs);

    if (n_sides == 0) {
        WARN("no sides?");
        sector->n_sides = 0;
        sector->min = VEC2(NAN);
        sector->max = VEC2(NAN);
        return;
    }

    const bool change =
        !glms_vec2_eqv_eps(sector->min, p_min)
        || !glms_vec2_eqv_eps(sector->max, p_max);

    sector->n_sides = n_sides;
    sector->min = p_min;
    sector->max = p_max;

    const vec2s old_min = sector->min, old_max = sector->max;

    if (change) {
        // TODO: this can be done more effeciently, check if current and previous
        // bounds are entirely contained within existing boundaries
        // recalculate overall level bounds on change
        level->bounds.min = IVEC2(U16_MAX, U16_MAX);
        level->bounds.max = IVEC2(0, 0);
        level_dynlist_each(level->sectors, it) {
            sector_t *s = *it.el;
            level->bounds.min.x = min(level->bounds.min.x, s->min.x);
            level->bounds.min.y = min(level->bounds.min.y, s->min.y);
            level->bounds.max.x = max(level->bounds.max.x, s->max.x);
            level->bounds.max.y = max(level->bounds.max.y, s->max.y);
        }

        level_resize_blocks(level);
    }

    if (!glms_vec2_eqv_eps(sector->min, old_min)
        || !glms_vec2_eqv_eps(sector->max, old_max)) {
        level_update_sector_blocks(
            level,
            sector,
            &old_min,
            &old_max);
    }

    // assign subsectors IDs, assign into block map
    dynlist_each(sector->subs, it) {
        subsector_finalize(level, sector, it.el);
    }

    // calculate subsector neighbors
    dynlist_each(sector->subs, it) {
        // find neighbors
        level_traverse_block_area(
            level,
            glms_vec2_sub(it.el->min, VEC2(0.1f)),
            glms_vec2_add(it.el->max, VEC2(0.1f)),
            (traverse_blocks_f) subsector_traverse_block_neighbors,
            it.el);
    }

    // this sector + all possible neighbors must have visibility recalculated
    sector_traverse_neighbors(
        level, sector, traverse_compute_visible, NULL);

    // enqueue sides for update
    llist_each(sector_sides, &sector->sides, it) {
        *dynlist_push(level->dirty_sides) = LPTR_FROM(it.el);
    }
}

subsector_t *sector_find_subsector(sector_t *sector, vec2s point) {
    dynlist_each(sector->subs, it) {
        dynlist_each(it.el->lines, it_l) {
            // must be left or on (>= 1) to be inside
            if (point_side(point, it_l.el->a->pos, it_l.el->b->pos) < 0) {
                goto nextsub;
            }
        }

        return it.el;
nextsub:
    }

    return NULL;
}

void sector_add_side(
    level_t *level,
    sector_t *sector,
    side_t *side) {
    ASSERT(!side->sector);

    llist_prepend(sector_sides, &sector->sides, side);

    side->sector = sector;
    sector->n_sides++;

    side_recalculate(level, side);
    sector_recalculate(level, sector);
}

void sector_remove_side(
    level_t *level,
    sector_t *sector,
    side_t *side) {
    ASSERT(side->sector == sector);

    llist_remove(sector_sides, &sector->sides, side);

    side->sector = NULL;

    sector->n_sides--;

    // sectors are marked during deletion
    if (!(sector->level_flags & LF_MARK)) {
        if (sector->n_sides == 0) {
            sector_delete(level, sector);
        } else {
            sector_recalculate(level, sector);
        }
    }

    side_recalculate(level, side);
}

u8 sector_light(
    const level_t*,
    const sector_t *sector) {
    // TODO: SF_SKY should use some level-level brightness
    u8 res = sector->base_light;

    if (sector->flags & (SECTOR_FLAG_FULLBRIGHT | SECTOR_FLAG_SKY)) {
        res = LIGHT_MAX;
        goto done;
    }

    // expected blinks/second
    f32 bps = 0.0f;
    switch (sector->cos_type) {
    case SCCT_BLINK_RAND_LOFREQ:
        bps = 0.4f;
        break;
    case SCCT_BLINK_RAND_MDFREQ:
        bps = 1.4f;
        break;
    case SCCT_BLINK_RAND_HIFREQ:
        bps = 1.9f;
        break;
    default:
    }

    if (bps > 0.0f) {
        const int
            blink_ticks = 6,
            blink_div = TICKS_PER_SECOND / blink_ticks;

        rand_t r =
            rand_create((state->time.tick / blink_ticks) + sector->index);

        res =
            rand_chance(&r, bps / blink_div) ?
                (sector->base_light > (LIGHT_MAX / 2) ?
                    (sector->base_light - (LIGHT_MAX / 3))
                    : (sector->base_light + (LIGHT_MAX / 3)))
                : sector->base_light;
    }

done:
    if (sector->flags & SECTOR_FLAG_ANY_FLASH) {
        res += 4;
    }

    return res;
}

ivec2s sector_tex_offset(
    const level_t*,
    const sector_t*sector,
    plane_type plane) {
    return sector->planes[plane].offsets;
}

typedef struct {
    sector_t *sect;
    u16 start, end;
    f32 speed, z;
    bool reverse;
} scft_diff_data_t;

/* static void act_scft_diff(level_t*, actor_t *, scft_diff_data_t *) { */
    /* TYPEOF(data->sector->funcdata.diff) *funcdata = &data->sector->funcdata.diff; */

    /* u16 *pz = &data->sector->planes[funcdata->plane].z; */
    /* const u16 z = *pz; */

    /* if (data->reverse && *pz == data->start) { */
    /*     actor->flags |= AF_DONE; */
    /* } else if (*pz != (data->reverse ? data->start : data->end)) { */
    /*     data->z += (data->reverse ? -1 : 1) * data->speed; */
    /*     data->z = */
    /*         clamp( */
    /*             data->z, */
    /*             min(data->start, data->end), */
    /*             max(data->start, data->end)); */
    /*     *pz = data->z; */
    /* } */

    /* if (z != *pz) { */
    /*     // TODO: not necessary? */
    /*     /1* l_sector_recalculate(data->sect); *1/ */
    /* } */
/* } */

static void scft_diff_tagchange(
    level_t *,
    sector_t *) {
    /* scft_t_diff_data *data; */

    /* TYPEOF(sector->funcdata.diff) *funcdata = &sector->funcdata.diff; */

    /* // if there is already an actor, reverse it */
    /* if (sector->actor) { */
    /*     data = sector->actor->param; */
    /*     data->reverse = !l_get_tag_val(level, sector->tag); */
    /* } else { */
    /*     data = malloc(sizeof(scft_t_diff_data)); */
    /*     *data = (scft_t_diff_data) { */
    /*         .sect = sector, */
    /*         .start = sector->planes[funcdata->plane].z, */
    /*         .end = ((int) sector->planes[funcdata->plane].z) + funcdata->diff, */
    /*         .speed = funcdata->diff / (f32) funcdata->ticks, */
    /*         .z = sector->planes[funcdata->plane].z, */
    /*         .reverse = false, */
    /*     }; */

    /*     l_add_actor( */
    /*         level, */
    /*         (actor_t) { */
    /*             .param = data, */
    /*             .act = (f_actor) act_scft_diff, */
    /*             .flags = AF_PARAM_OWNED */
    /*         }, */
    /*         LPTR_FROM(sector)); */
    /* } */
}

sector_func_type_t SECTOR_FUNC_TYPE[SCFT_COUNT] = {
    [SCFT_NONE] = {},
    [SCFT_DIFF] = {
        .tag_change = scft_diff_tagchange
    }
};
