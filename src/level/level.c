#include "level/level.h"
#include "level/lptr.h"
#include "level/object.h"
#include "level/particle.h"
#include "level/portal.h"
#include "level/sectmat.h"
#include "level/sector.h"
#include "level/side.h"
#include "level/sidemat.h"
#include "level/wall.h"
#include "level/block.h"
#include "reload.h"
#include "state.h"
#include "util/macros.h"
#include "util/math.h"
#include "util/bitmap.h"

// userdata for level dynlists
typedef struct level_dynlist_data {
    // free list bitmap
    BITMAP *bits;

    // size of free list bitmap in bytes
    int sizebytes;

    // generation list
    DYNLIST(u8) gen;

    // number of used bits
    int count;
} level_dynlist_data_t;

static const char *type_to_str(u8 type) {
    return
        ((const char *[]) {
            [0] = "VERTEX",
            [1] = "WALL",
            [2] = "SIDE",
            [3] = "SIDEMAT",
            [4] = "SECTOR",
            [5] = "SECTMAT",
            [6] = "DECAL",
            [7] = "OBJECT",
        })[ctz(type)];
}

int level_types_to_str(char *dst, usize n, u8 tag) {
    char *p = &dst[0], *end = &dst[n];

    if (n != 0) {
        *p = '\0';
    }

    int i = 0;

    for (int j = 0; j < T_COUNT; j++) {
        if (!(tag & (1 << j))) { continue; }

        const char *s = type_to_str(1 << j);
        int n = snprintf(p, p - end, "%s%s", p == &dst[0] ? "" : ", ", s);
        p += n;
        i += n;
    }

    return i;
}

DYNLIST(void*) *level_get_list(const level_t *level, int type) {
    switch (type) {
    case T_VERTEX:  return (DYNLIST(void*)*) &level->vertices;
    case T_WALL:    return (DYNLIST(void*)*) &level->walls;
    case T_SIDE:    return (DYNLIST(void*)*) &level->sides;
    case T_SIDEMAT: return (DYNLIST(void*)*) &level->sidemats;
    case T_SECTOR:  return (DYNLIST(void*)*) &level->sectors;
    case T_SECTMAT: return (DYNLIST(void*)*) &level->sectmats;
    case T_DECAL:   return (DYNLIST(void*)*) &level->decals;
    case T_OBJECT:  return (DYNLIST(void*)*) &level->objects;
    default: ASSERT(false, "%d", type);
    }

    return NULL;
}

int level_get_list_count(const level_t *level, int type) {
    DYNLIST(void*) *list = level_get_list(level, type);
    const level_dynlist_data_t *data = *dynlist_userdata_ptr(*list);
    return data->count;
}

u8 level_get_gen(const level_t *level, int type, int i) {
    const level_dynlist_data_t *data =
        *dynlist_userdata_ptr(*level_get_list(level, type));
    return data->gen[i];
}

void *_level_alloc_impl(
    level_t*, DYNLIST(void*) *list, int tsize, u8 *gen, u16 *index) {
    level_dynlist_data_t *data = *dynlist_userdata_ptr(*list);

    // find first free bit
    int i = bitmap_find(data->bits, dynlist_size(*list), 0, false);

    if (i == INT_MAX) {
        // no free bits - need to extend
        // check if we need to extend free list bitmap
        if ((data->sizebytes * 8) < dynlist_size(*list) + 1) {
            const int
                oldbytes = data->sizebytes,
                newbytes = data->sizebytes * 2;

            data->bits = realloc(data->bits, newbytes);

            memset(
                &data->bits[oldbytes],
                0,
                newbytes - oldbytes);

            data->sizebytes = newbytes;
        }

        i = dynlist_size(*list);
        *dynlist_push(*list) = NULL;
        *dynlist_push(data->gen) = 0;
    } else {
    }

    void *p = calloc(1, tsize);
    (*list)[i] = p;
    *gen = ++data->gen[i];
    *index = i;
    bitmap_set(data->bits, i);

    data->count++;
    return p;
}

void _level_free_impl(level_t*, DYNLIST(void*) *list, void *ptr, int index) {
    // shrink free map
    level_dynlist_data_t *data = *dynlist_userdata_ptr(*list);

    free(ptr);
    (*list)[index] = NULL;
    bitmap_clr(data->bits, index);

    if (index == dynlist_size(*list)) {
        while (
            dynlist_size(*list) != 1
            && !(*list)[dynlist_size(*list) - 1]) {
            dynlist_pop(*list);
            dynlist_pop(data->gen);
        }

        // TODO: loop bad
        while (dynlist_size(*list) <= (data->sizebytes / 2) * 8) {
            data->sizebytes = max(data->sizebytes / 2, 1);
            data->bits = realloc(data->bits, data->sizebytes);
        }
    }

    data->count--;
}

static void level_dynlist_init(
    level_t*,
    DYNLIST(void*) *list,
    bool addbad) {
    level_dynlist_data_t *data = malloc(sizeof(level_dynlist_data_t));

    data->sizebytes = BITMAP_SIZE_TO_BYTES(max(dynlist_capacity(*list), 16));
    data->bits = calloc(1, data->sizebytes);
    data->gen = NULL;
    data->count = 0;

    *dynlist_userdata_ptr(*list) = data;

    if (addbad) {
        bitmap_set(data->bits, 0);
        *dynlist_push(*list) = NULL;
        *dynlist_push(data->gen) = 0;
    }
}

void level_init(level_t *level) {
    memset(level, 0, sizeof(*level));

    // version 0 indicates no version, so start with some (arbitrary) value
    level->version = 1;

    // allocate all level dynlists, assign userdata pointer
    dynlist_alloc(level->vertices);
    level_dynlist_init(level, (DYNLIST(void*)*) &level->vertices, true);
    dynlist_alloc(level->walls);
    level_dynlist_init(level, (DYNLIST(void*)*) &level->walls, true);
    dynlist_alloc(level->sides);
    level_dynlist_init(level, (DYNLIST(void*)*) &level->sides, true);
    dynlist_alloc(level->sidemats);
    level_dynlist_init(level, (DYNLIST(void*)*) &level->sidemats, false);
    dynlist_alloc(level->sectors);
    level_dynlist_init(level, (DYNLIST(void*)*) &level->sectors, true);
    dynlist_alloc(level->sectmats);
    level_dynlist_init(level, (DYNLIST(void*)*) &level->sectmats, false);
    dynlist_alloc(level->decals);
    level_dynlist_init(level, (DYNLIST(void*)*) &level->decals, true);
    dynlist_alloc(level->objects);
    level_dynlist_init(level, (DYNLIST(void*)*) &level->objects, true);

    // create 0-index materials (NOMAT)
    sidemat_t *sidemat = sidemat_new(level);
    snprintf(sidemat->name.name, sizeof(sidemat->name.name), "%s", "NOMAT");

    sectmat_t *sectmat = sectmat_new(level);
    snprintf(sectmat->name.name, sizeof(sectmat->name.name), "%s", "NOMAT");
}

void level_destroy(level_t *level) {
    if (level->visibility.matrix) {
        free(level->visibility.matrix);
    }

    for (int i = 0; i < TAG_MAX; i++) {
        if (level->tag_lists[i]) {
            dynlist_free(level->tag_lists[i]);
        }
    }

    // destroy actors
    dlist_each(listnode, &level->actors, it) {
#ifdef RELOADABLE
        if (g_reload_host) {
            g_reload_host->del_fn((void**) &it.el->act);
        }
#endif //ifdef RELOADABLE

        dlist_remove(listnode, &level->actors, it.el);
        if (it.el->flags & AF_PARAM_OWNED) {
            free(it.el->param);
        }
        free(it.el);
    }

#ifdef MAPEDITOR
    level_dynlist_each(level->vertices, it) {
        dynlist_free((*it.el)->walls);
    }
#endif //ifdef MAPEDITOR

    // TODO: this definitely does not get everything, maybe add *_free functions
    // for all level types?
    // destroy dynlists + data
#define DESTROY_DYNLIST(_t, _list, ...) do {                     \
        level_dynlist_each(_list, it) {                              \
            free(*it.el);                                        \
        }                                                        \
        level_dynlist_data_t *d = *dynlist_userdata_ptr(_list); \
        dynlist_free(d->gen);                                    \
        free(d->bits);                                           \
        free(d);                                                 \
        dynlist_free(_list);                                     \
    } while (0);

    LEVEL_FOR_ALL_LISTS(DESTROY_DYNLIST, level);

#undef DESTROY_DYNLIST

    bitmap_free(level->subsector_ids);
    dynlist_free(level->subsectors);

    bitmap_free(level->particle_ids);
    dynlist_free(level->particles);
}

void level_update(level_t *level, f32 dt) {
    // update sectors for dirty sides
    while (dynlist_size(level->dirty_sides) != 0) {
        lptr_t ptr = dynlist_pop(level->dirty_sides);
        if (!lptr_is_valid(level, ptr)) { continue; }

        ASSERT(LPTR_TYPE(ptr) == T_SIDE);
        side_t *s = LPTR_SIDE(level, ptr);

        level_update_side_sector(level, s);
    }

    dynlist_resize(level->dirty_sides, 0);

    // recompute for dirty vis sectors
    dynlist_each(level->dirty_vis_sectors, it) {
        if (!lptr_is_valid(level, *it.el)) { continue; }
        ASSERT(LPTR_TYPE(*it.el) == T_SECTOR);
        sector_t *s = LPTR_SECTOR(level, *it.el);
        if (s->level_flags & LF_MARK) { continue; }
        s->level_flags |= LF_MARK;
        sector_compute_visibility(level, s);
    }

    dynlist_each(level->dirty_vis_sectors, it) {
        if (lptr_is_valid(level, *it.el)) {
            LPTR_SECTOR(level, *it.el)->level_flags &= ~LF_MARK;
        }
    }

    dynlist_resize(level->dirty_vis_sectors, 0);

    level_dynlist_each(level->objects, it) {
        object_t *o = *it.el;

        if (o->sector) {
            object_update(level, o, dt);
        }
    }
}

void level_tick(level_t *level) {
    dlist_each(listnode, &level->actors, it) {
        actor_t *actor = it.el;

        if (actor->flags & AF_DONE) {
            if (actor->storage) {
                *actor->storage = NULL;
            }

            dlist_remove(listnode, &level->actors, actor);

            if (actor->flags & AF_PARAM_OWNED) {
                free(actor->param);
            }

#ifdef RELOADABLE
            if (g_reload_host) {
                g_reload_host->del_fn((void**) &it.el->act);
            }
#endif //ifdef RELOADABLE
            free(actor);
            continue;
        } else {
            actor->act(level, actor, actor->param);
        }
    }

    // tick particles
    int i = -1;
    while (
        (i = bitmap_find(
                level->particle_ids,
                dynlist_size(level->particles),
                i + 1,
                true))
           != INT_MAX) {
        particle_tick(level, &level->particles[i]);
    }

    // tick sectors
    level_dynlist_each(level->sectors, it) {
        sector_tick(level, *it.el);
    }

    // done once per tick in a single location so that references aren't
    // invalidated mid-tick/mid-frame
    dynlist_each(level->destroy_objects, it) {
        object_delete(level, *it.el);
    }

    dynlist_resize(level->destroy_objects, 0);
}

// trace sides from a start side, picking sides which form the least angle
int level_trace_sides(
    level_t *level,
    side_t *startside,
    DYNLIST(side_t*) *sides) {
// #define DO_DEBUG_TRACE_SIDES

#ifdef DO_DEBUG_TRACE_SIDES
#define DEBUG_TRACE_SIDES(...) LOG(__VA_ARGS__)
#else
#define DEBUG_TRACE_SIDES(...)
#endif // ifdef DO_DEBUG_TRACE_SIDES

    ASSERT(startside);
    ASSERT(sides);

    // number of sides written out
    int n = 0;

    // number of sides to return
    int count = 0;

    side_t *side = startside;
    DEBUG_TRACE_SIDES("STARTING TRACE", side->index);
    while (side) {
        DEBUG_TRACE_SIDES("side: %d", side->index);

        if (side->level_flags & LF_FAILTRACE) {
            DEBUG_TRACE_SIDES("hit failtrace %d", side->index);
            // if side is failtrace then this is a bad trace
            count = 0;
            goto done;
        } else if (side->level_flags & LF_TRACED) {
            // if side is marked then it already exists
            // is this the start side? trace success. otherwise mark indicates
            // failure
            if (side == startside) {
                DEBUG_TRACE_SIDES("hit start %d", side->index);
                count = n;
            } else {
                DEBUG_TRACE_SIDES("hit other %d", side->index);
                count = 0;
            }

            goto done;
        }

        // write side and mark
        *dynlist_push(*sides) = side;
        n++;
        side->level_flags |= LF_TRACED;

        // move along wall from v0 -> v1
        vertex_t *vs[2];
        side_get_vertices(side, vs);

        // minimum found angle and corresponding side
        f32 amin = TAU;
        side_t *next = NULL;

        // going from end of side line, try to find next side by finding wall
        // side with minimal angle
        dynlist_each(vs[1]->walls, it) {
            wall_t *vwall = *it.el;

            // ignore current wall
            if (vwall == side->wall) { continue; }

            for (int i = 0; i < 2; i++) {
                side_t *vside = vwall->sides[i];

                if (!vside) { continue; }

                // find angle between this side and next side
                vertex_t *us[2];
                side_get_vertices(vside, us);

                // side is not continuous from this side
                if (us[0] != vs[1]) { continue; }

                // get angle between sides vs[0] -> vs[1], us[0] -> us[1] where
                // vs[1] == us[0]
                const f32 a =
                    angle_in_points(vs[0]->pos, vs[1]->pos, us[1]->pos);

                if (a < amin) {
                    amin = a;
                    next = vside;
                }
            }
        }

        if (!next) {
            DEBUG_TRACE_SIDES("no next after %d", side->index);
            // failure
            count = 0;
            goto done;
        }

        side = next;
    }

done:
    // clear all marks
    for (int i = 0; i < n; i++) {
        (*sides)[i]->level_flags &= ~LF_TRACED;
    }

    if (dynlist_size(*sides) == 0) {
        dynlist_free(*sides);
    }

    DEBUG_TRACE_SIDES("END TRACE", side->index);
    return count;
}

side_t *level_find_near_side(
    const level_t *level,
    const vertex_t *vertex,
    const sector_t *preferred_sector) {
    side_t *res = NULL;

    dynlist_each(vertex->walls, it) {
        for (int i = 0; i < 2; i++) {
            side_t *side = (*it.el)->sides[i];

            if (side) {
                if (preferred_sector && side->sector == preferred_sector) {
                    return side;
                } else if (!res) {
                    res = side;
                }
            }
        }
    }

    return res;
}

sector_t *level_find_near_sector(
    const level_t *level,
    vec2s point) {
    f32 dist = 1e30;
    sector_t *found = NULL;

    level_dynlist_each(level->sectors, it_sect) {
        sector_t *sect = *it_sect.el;

        // check if distance to any side is lesser
        llist_each(sector_sides, &sect->sides, it_side) {
            side_t *side = it_side.el;

            const f32 d =
                point_to_segment(
                    point,
                    side->wall->v0->pos,
                    side->wall->v1->pos);

            if (d < dist) {
                dist = d;
                found = sect;
                break;
            }
        }
    }

    return found;
}

// #define DO_DEBUG_UPDATE_SIDES

#ifdef DO_DEBUG_UPDATE_SIDES
#define DEBUG_UPDATE_SIDES(...) LOG(__VA_ARGS__)
#else
#define DEBUG_UPDATE_SIDES(...)
#endif // ifdef DO_DEBUG_UPDATE_SIDES

// checks if sides form a hole in the specified sector
static bool sides_form_sector_hole(
    level_t *level,
    side_t **sides,
    int n_sides,
    sector_t *sect) {
    if (n_sides == 0) { return false; }

    side_t *sector_sides[sect->n_sides];
    int nsector_sides =
        sector_get_sides(level, sect, sector_sides, sect->n_sides);

    // keep backup of all sides for un-flagging
    side_t *tsector_sides[sect->n_sides];
    memcpy(tsector_sides, sector_sides, nsector_sides * sizeof(side_t*));

    // remove all sides from sector_sides which share a wall with any sides in our
    // side list. this means all the sides in "sides" are removed as well as
    // those with which they share a wall, should they be in the sector
    //
    // mark removed sides for trace failure as well
    wall_t *sidewalls[n_sides];
    for (int i = 0; i < n_sides; i++) {
        sidewalls[i] = sides[i]->wall;
    }

    for (int i = 0; i < nsector_sides;) {
        bool remove = false;

        for (int j = 0; j < n_sides; j++) {
            if (sector_sides[i]->wall == sidewalls[j]) {
                sector_sides[i]->level_flags |= LF_FAILTRACE;
                remove = true;
                break;
            }
        }

        if (remove) {
            memmove(
                &sector_sides[i],
                &sector_sides[i + 1],
                (nsector_sides - i - 1) * sizeof(side_t*));
            nsector_sides--;
        } else {
            i++;
        }
    }

    bool res;

    vec2s sectlines[max(nsector_sides, 1)][2];
    vec2s sidelines[n_sides][2];

    if (nsector_sides == 0) {
        res = false;
        goto done;
    }

    // check that all remaining sides still form a trace
    // TODO: optimize
    for (int i = 0; i < nsector_sides; i++) {
        DYNLIST(side_t*) trace = NULL;
        if (!level_trace_sides(level, sector_sides[i], &trace)) {
            DEBUG_UPDATE_SIDES(
                "failed because sector_sides don't form a trace now");
            res = false;
            goto done;
        }
        dynlist_free(trace);
    }

    sides_to_vertices(sector_sides, nsector_sides, sectlines, nsector_sides);
    sides_to_vertices(sides, n_sides, sidelines, n_sides);
    res = polygon_is_hole(sectlines, nsector_sides, sidelines, n_sides);

done:
    // un-FAILTRACE all tsector_sides
    for (int i = 0; i < sect->n_sides; i++) {
        tsector_sides[i]->level_flags &= ~LF_FAILTRACE;
    }
    return res;
}

// "breaks" a sector by taking "sides", a subset of sides in the specified
// sector, and moving them to a new sector
// returns the new sector
static sector_t *break_sector(
    level_t *level,
    sector_t *sector,
    side_t **sides,
    int n_sides) {
    ASSERT(sector);
    ASSERT(n_sides != 0);
    sector_t
        *near_sect =
            level_find_near_sector(level, wall_midpoint(sides[0]->wall)),
        *new_sect = sector_new(level, near_sect);

    // move sides to new sector
    near_sect->level_flags |= LF_DO_NOT_RECALC;
    new_sect->level_flags |= LF_DO_NOT_RECALC;
    for (int i = 0; i < n_sides; i++) {
        sector_remove_side(level, sector, sides[i]);
        sector_add_side(level, new_sect, sides[i]);
    }
    near_sect->level_flags &= ~LF_DO_NOT_RECALC;
    new_sect->level_flags &= ~LF_DO_NOT_RECALC;

    sector_recalculate(level, near_sect);
    sector_recalculate(level, new_sect);

    DEBUG_UPDATE_SIDES(
        "moved %d sides to new sector %d",
        n_sides,
        lptr_to_index(level, LPTR_FROM(newsect)));
    return new_sect;
}

// return true if all sides have same NON-NULL sector
static bool sides_have_same_sector(
    side_t **sides,
    int n) {
    if (!sides[0]->sector) { return false; }

    for (int i = 1; i < n; i++) {
        if (sides[i]->sector != sides[0]->sector) {
            return false;
        }
    }

    return true;
}

sector_t *level_update_side_sector(
    level_t *level,
    side_t *side) {
    // result, NULL if no new sector is created
    sector_t *res = NULL;

    // trace starting at side
    DYNLIST(side_t*) trace = NULL;
    const int ntrace = level_trace_sides(level, side, &trace);

    // normal sector for each traced side
    DYNLIST(sector_t*) normal_sects = NULL;

    // check if all sides are in the same sector
    const bool samesect = sides_have_same_sector(trace, ntrace);

    // no trace
    if (ntrace == 0) {
        DEBUG_UPDATE_SIDES("no trace for side %d", side->index);
        if (side->sector) {
            DEBUG_UPDATE_SIDES("removing bad sector %d", side->sector->index);
            sector_delete(level, side->sector);
        }

        res = NULL;
        goto done;
    }

    // must stop here, trace contains double
    side_t *double_side = NULL;
    if ((double_side = level_sides_find_double(level, trace, ntrace))) {
        // try to fix this by splitting the sides up into new sectors, if their
        // traces are different but entirely parts of the same sector
        side_t *double_other = side_other(double_side);
        ASSERT(double_other);
        ASSERT(side_other(double_other) == double_side);

        DYNLIST(side_t*)
            atrace = NULL,
            btrace = NULL;

        const int
            natrace = level_trace_sides(level, double_side, &atrace),
            nbtrace = level_trace_sides(level, double_other, &btrace);

        if (natrace == 0 || nbtrace == 0) {
            DEBUG_UPDATE_SIDES("trace has double but one trace does not complete");
            res = NULL;
            goto double_done;
        }

        // check that traces are all in the same sector
        if (!sides_have_same_sector(atrace, natrace)
            || !sides_have_same_sector(btrace, nbtrace)
            || atrace[0]->sector != btrace[0]->sector) {
            DEBUG_UPDATE_SIDES("trace has double but not same sector, can't fix");
            res = NULL;
            goto double_done;
        }

        bool separate = true;
        dynlist_each(atrace, it_a) {
            dynlist_each(btrace, it_b) {
                if (*it_a.el == *it_b.el) {
                    separate = false;
                    break;
                }
            }
        }

        if (!separate) {
            DEBUG_UPDATE_SIDES("trace has double but not separate, can't fix");
            res = NULL;
            goto double_done;
        }

        // traces are separate! break the sector
        DEBUG_UPDATE_SIDES("traces are separate, breaking sector");
        res = break_sector(level, double_side->sector, btrace, nbtrace);

double_done:
        dynlist_free(atrace);
        dynlist_free(btrace);
        return res;
    }

    // check if sides form a hole in the sector on any of the normal points of
    // the sides
    dynlist_each(trace, it) {
        sector_t *normal_sect =
            level_find_point_sector(level, side_normal_point(*it.el), NULL);

        if (normal_sect) {
            *dynlist_push(normal_sects) = normal_sect;
            normal_sect->level_flags |= LF_MARK;
        }
    }

    sector_t *holesect = NULL;

    dynlist_each(normal_sects, it) {
        if (!((*it.el)->level_flags & LF_MARK)) {
            continue;
        }

        if (!holesect
            && sides_form_sector_hole(level, trace, ntrace, *it.el)) {
            holesect = *it.el;
        }

        (*it.el)->level_flags &= ~LF_MARK;
    }

    if (holesect) {
        DEBUG_UPDATE_SIDES(
            "sides are hole to sector %d",
            lptr_to_index(level, LPTR_FROM(holesect)));

        dynlist_each(trace, it) {
            DEBUG_UPDATE_SIDES("  %d", (*it.el)->index);
        }

        // all sides are already in the hole sector, don't move them
        if (samesect && holesect == side->sector) {
            DEBUG_UPDATE_SIDES("  not moving");
            res = NULL;
            goto done;
        }

        DEBUG_UPDATE_SIDES("samsect: %d, holesect: %d, side->sector: %d",
            samesect,
            lptr_to_index(level, LPTR_FROM(holesect)),
            lptr_to_index(level, LPTR_FROM(side->sector)));

        // move sides to hole sector, update portals
        holesect->level_flags |= LF_DO_NOT_RECALC;
        dynlist_each(trace, it) {
            side_t *side = *it.el;

            if (side->sector == holesect) {
                // already in sector, no need to move
                continue;
            }

            if (side->sector) {
                sector_remove_side(level, side->sector, side);
            }

            sector_add_side(level, holesect, side);
        }

        holesect->level_flags &= ~LF_DO_NOT_RECALC;
        sector_recalculate(level, holesect);

        DEBUG_UPDATE_SIDES("  moved");
        res = NULL;
        goto done;
    }

    // if side is not in a sector, form a new sector if all other sides do not
    // have a sector. otherwise we cannot form a sector
    if (!side->sector) {
        dynlist_each(trace, it) {
            if ((*it.el)->sector) {
                DEBUG_UPDATE_SIDES("no sector AND no match");
                res = NULL;
                goto done;
            }
        }

        // form a new sector with these sides
        sector_t *newsect =
            sector_new(
                level,
                level_find_near_sector(
                    level,
                    wall_midpoint(side->wall)));

        dynlist_each(trace, it) {
            if ((*it.el)->sector == newsect) {
                continue;
            }

            ASSERT(!(*it.el)->sector);
            sector_add_side(level, newsect, *it.el);
        }

        DEBUG_UPDATE_SIDES("made new sector from empty");
        res = newsect;
        goto done;
    }

    // if side sector has any double sides, it must be removed
    if (level_sector_find_double_side(level, side->sector)) {
        DEBUG_UPDATE_SIDES("invalid sector with double sides, but nothing can be done");
        res = NULL;
        goto done;
    }

    // if the sector is coherent, there is nothing to update
    if (level_sector_is_coherent(level, side->sector)) {
        DEBUG_UPDATE_SIDES("sector is coherent");
        res = NULL;
        goto done;
    }

    // create a sector out of these traced sides, using the selected side's
    // sector as the "dominant" sector
    if (!samesect) {
        UNUSED int nmove = 0;

        // collect list of opposite sides which are portal to this sector
        dynlist_each(trace, it) {
            side_t *traceside = *it.el;
            if (traceside->sector && traceside->sector != side->sector) {
                sector_remove_side(level, traceside->sector, traceside);
                nmove++;
            }
        }

        DEBUG_UPDATE_SIDES("moving %d sides", nmove);

        // add all other sides to side->sector
        dynlist_each(trace, it) {
            side_t *traceside = *it.el;
            if (!traceside->sector) {
                sector_add_side(level, side->sector, traceside);
            }
        }

        // no new sector
        DEBUG_UPDATE_SIDES("no new sector but not samesect");
        res = NULL;
        goto done;
    }

    // it is ensured that all sides are in the same sector now

    // these sides form a hole but are not oriented such that they point out
    // into the rest of the sector - form a new sector with them

    // form a new sector with these sides
    res = break_sector(level, side->sector, trace, ntrace);

done:
    dynlist_free(normal_sects);
    dynlist_free(trace);
    return res;
}

int level_intersect_walls_on_line(
    level_t *level,
    vec2s p0,
    vec2s p1,
    wall_t **walls,
    side_t ***sides,
    int n) {

    // normalized direction of p0 -> p1
    const vec2s dir = glms_vec2_normalize(glms_vec2_sub(p1, p0));

    int i = 0;

    level_dynlist_each(level->walls, it) {
        wall_t *w = *it.el;

        const vec2s hit =
            intersect_segs(
                p0,
                p1,
                w->v0->pos,
                w->v1->pos);

        if (isnan(hit.x)) { continue; }

        if (i == n) {
            WARN("out of space in l_intersect_walls_on_line");
            return 0;
        }

        if (walls) { walls[i] = w; }

        if (sides) {
            // find "exit" side (that whose normal "dir" lies most on)
            const f32
                dotr = glms_vec2_dot(dir, w->normal),
                dotl = glms_vec2_dot(dir, VEC2(-w->normal.x, -w->normal.y));

            sides[i] = &w->sides[dotr > dotl ? 0 : 1];
        }

        i++;
    }

    return i;
}

side_t *level_sides_find_double(
    level_t *level,
    side_t **sides,
    int n) {
    for (int i = 0; i < n; i++) {
        if (!sides[i]->sector) { continue; }

        side_t *other = side_other(sides[i]);
        if (other && other->sector == sides[i]->sector) {
            return sides[i];
        }
    }
    return NULL;
}

side_t *level_sector_find_double_side(
    level_t *level,
    sector_t *sector) {
    const int n_sides = sector->n_sides;
    side_t *sides[n_sides];
    sector_get_sides(level, sector, sides, n_sides);
    return level_sides_find_double(level, sides, n_sides);
}

bool level_sector_is_coherent(
    level_t *level,
    sector_t *sector) {
    const int n_sides = sector->n_sides;
    side_t *sides[n_sides];
    sector_get_sides(level, sector, sides, n_sides);

    // an "outline" is a trace of sides in the sector which do *not* form a hole
    // a coherent sector can, by definition, only have 0 outlines (all sides
    // form a single trace) or 1 outline (all holes are within this 1 outline)
    int noutlines = 0;

    // keep track of sides which have already been traced
    DYNLIST(side_t*) traced = NULL;
    dynlist_ensure(traced, n_sides);

    // current list of traced sides
    DYNLIST(side_t*) trace = NULL;

    bool res = true;

    // trace each (unmarked) side
    for (int i = 0; i < n_sides; i++) {
        // check if side has been traced already
        bool found = false;
        dynlist_each(traced, it) {
            if (sides[i] == *it.el) {
                found = true;
                break;
            }
        }

        if (found) { continue; }

        ASSERT(!trace);
        const int ntrace = level_trace_sides(level, sides[i], &trace);

        // are all sides in trace also part of this sector?
        for (int j = 0; j < ntrace; j++) {
            if (trace[j]->sector != sides[i]->sector) {
                DEBUG_UPDATE_SIDES("incorrect trace");
                res = false;
                goto done;
            }

            // no need to check this side again
            *dynlist_push(traced) = trace[j];
        }

        if (ntrace != n_sides) {
            DEBUG_UPDATE_SIDES("ntrace %d, n_sides %d", ntrace, n_sides);

            // we have gotten a smaller portion of the sector - is it a hole?
            if (sides_form_sector_hole(level, trace, ntrace, sides[i]->sector)) {
                // OK
            } else {
                // this must be an outline
                noutlines++;

                if (noutlines >= 2) {
                    // only 0/1 outlines possible in coherent sector
                    DEBUG_UPDATE_SIDES("multiple outlines");
                    res = false;
                    goto done;
                }
            }
        }

        dynlist_free(trace);
    }

done:
    // if res (success), all sides ought to be in "traced"
    ASSERT(!res || dynlist_size(traced) == n_sides);

    dynlist_free(trace);
    dynlist_free(traced);
    return res;
}

side_t *level_nearest_side(
    const level_t *level,
    vec2s point) {
    // TODO: more effecient search, use blocks?
    const side_t *side = NULL;
    f32 dist = 1e30;
    level_dynlist_each(level->sides, it) {
        side_t *s = *it.el;

        const vec2s p =
            point_project_segment(
                point,
                s->wall->v0->pos,
                s->wall->v1->pos);

        const f32 d = glms_vec2_norm(glms_vec2_sub(point, p));
        const int sgn =
            (int) sign(
                glms_vec2_dot(
                    side_normal(s),
                    glms_vec2_sub(point, p)));

        // take nearer side *or* side on same wall but which actually points to
        // point
        if (d < dist || (side && side == side_other(s) && sgn >= 0)) {
            dist = d;
            side = s;
        }
    }

    return (side_t*) side;
}

vertex_t *level_nearest_vertex(
    const level_t *level,
    vec2s point,
    f32 *dist) {
    vertex_t *v = NULL;
    f32 d = 1e10;

    // TODO: search via block
    level_dynlist_each(level->vertices, it) {
        const f32 d_it = glms_vec2_norm2(glms_vec2_sub(point, (*it.el)->pos));
        if (v == NULL || d_it < d) {
            v = *it.el;
            d = d_it;
        }
    }

    if (dist) { *dist = d; }
    return v;
}

sector_t *level_find_point_sector(
    level_t *level,
    vec2s point,
    sector_t *sector) {
    if (sector && sector_contains_point(sector, point)) {
        return sector;
    }

    if (!level->blocks.arr) {
        // use only current sector data, cannot rely on blocks
        if (!sector) {
            // fall back to simple linear search
            level_dynlist_each(level->sectors, it) {
                sector_t *s = *it.el;

                if (point.x < s->min.x
                    || point.x > s->max.x
                    || point.y < s->min.y
                    || point.y > s->max.y) {
                    continue;
                }

                if (sector_contains_point(s, point)) {
                    return (sector_t*) s;
                }
            }

            return NULL;
        } else {
            // BFS neighbors in a circular queue, point is likely to be in one
            // of the neighboring sectors
            enum { QUEUE_MAX = 64 };
            sector_t *queue[QUEUE_MAX] = { sector };
            int i = 0, n = 1;

            // keep track of which sectors have been traversed
            const int nsectors = dynlist_size(level->sectors);
            BITMAP_DECL(hits, nsectors);
            bitmap_fill(hits, nsectors, false);

            while (n != 0) {
                // get front of queue and advance to next
                sector_t *sect = queue[i];
                i = (i + 1) % (QUEUE_MAX);
                n--;

                if (bitmap_get(hits, sect->index)) {
                    continue;
                }

                bitmap_set(hits, sect->index);

                if (sector_contains_point(sect, point)) {
                    return sect;
                }

                // check neighbors
                llist_each(sector_sides, &sect->sides, it) {
                    if (it.el->portal
                        && it.el->portal->sector
                        && !bitmap_get(hits, it.el->portal->sector->index)) {
                        if (n == QUEUE_MAX) {
                            WARN("l_find_point_sector out of queue space!");
                            return NULL;
                        }

                        queue[(i + n) % QUEUE_MAX] = it.el->portal->sector;
                        n++;
                    }
                }
            }
        }
    } else {
        // block data is OK, use it
        block_t *block =
            level_get_block(level, level_pos_to_block(point));

        if (!block) {
            return NULL;
            LOG("no block for find point");
            DUMPTRACE();
        }

        // TODO: better
        dynlist_each(block->sectors, it) {
            if (sector_contains_point(*it.el, point)) {
                return *it.el;
            }
        }
    }

    return NULL;
}

void level_nearest_side_pos(
    const level_t *level,
    vec2s pos,
    side_t **pside,
    vec2s *pw,
    f32 *px) {
    side_t *nearest = level_nearest_side(level, pos);
    if (nearest) {
        vertex_t *vs[2];
        side_get_vertices(nearest, vs);
        const vec2s
            spos =
                point_project_segment(
                    pos,
                    vs[0]->pos,
                    vs[1]->pos),
            p = glms_vec2_sub(spos, vs[0]->pos);

        if (pside) { *pside = nearest; }
        if (pw) { *pw = spos; }
        if (px) { *px = glms_vec2_norm(p); }
    } else {
        if (pside) { *pside = NULL; }
        if (pw) { *pw = VEC2(0); }
        if (px) { *px = 0; }
    }
}

// NOTE: can return NULL when level is loading
static BITMAP *get_visibility_map(level_t *level, sector_t *s) {
    if (!level->visibility.matrix) { return NULL; }

    return
        &level->visibility.matrix[
            s->index * BITMAP_SIZE_TO_BYTES(level->visibility.n)];
}

bool level_is_sector_visible_from(level_t *level, sector_t *a, sector_t *b) {
    BITMAP *bits = get_visibility_map(level, a);
    return bits && bitmap_get(bits, b->index);
}

// ORs visible sectors from sector s with bitmap contents
static void or_visible_sectors(
    level_t *level,
    sector_t *s,
    BITMAP *bits) {
    BITMAP *sect_bits = get_visibility_map(level, s);

    if (!sect_bits) { return; }

    int i = -1;
    while (
        (i = bitmap_find(
                sect_bits,
                dynlist_size(level->sectors),
                i + 1, true)) != INT_MAX) {
        // check if sector agrees
        sector_t *other = level->sectors[i];
        if (bitmap_get(get_visibility_map(level, other), s->index)) {
            bitmap_set(bits, i);
        }
    }
}

int level_get_visible_sectors(
    level_t *level,
    sector_t *s,
    DYNLIST(sector_t*) *out,
    int flags) {
    // TODO: add "excluding via": sectors which are excluded by portal iff
    // their only connection is thorough that portal
    const int n_sectors = dynlist_size(level->sectors);
    BITMAP_DECL(bits, n_sectors);
    bitmap_fill(bits, n_sectors, 0);

    // fill with s
    or_visible_sectors(level, s, bits);

    if (flags & LEVEL_GET_VISIBLE_SECTORS_PORTALS) {
        // add any sectors visible from portals
        llist_each(sector_sides, &s->sides, it) {
            if ((it.el->flags & SIDE_FLAG_DISCONNECT)
                && it.el->portal->sector) {
                or_visible_sectors(level, it.el->portal->sector, bits);
            }
        }
    }

    int n = 0;
    int i = -1;
    while ((i = bitmap_find(bits, n_sectors, i + 1, true)) != INT_MAX) {
        *dynlist_push(*out) = level->sectors[i];
        n++;
    }

    return n;
}

typedef struct {
    vec2s pos;
    f32 r;
    DYNLIST(wall_t*) *out;
} walls_in_radius_data_t;

static bool traverse_walls_in_radius(
    level_t *level,
    block_t *block,
    ivec2s,
    walls_in_radius_data_t *data) {
    dynlist_each(block->walls, it) {
        if (point_to_segment(data->pos, (*it.el)->v0->pos, (*it.el)->v1->pos)
                <= data->r) {
            *dynlist_push(*data->out) = *it.el;
        }
    }
    return true;
}

int level_walls_in_radius(
    level_t *level, vec2s pos, f32 r, DYNLIST(wall_t*) *out) {
    walls_in_radius_data_t data = {
        .pos = pos,
        .r = r,
        .out = out
    };

    const int n_start = dynlist_size(*out);
    level_traverse_block_area(
        level,
        glms_vec2_sub(pos, VEC2(r)),
        glms_vec2_add(pos, VEC2(r)),
        (traverse_blocks_f) traverse_walls_in_radius,
        &data);
    return dynlist_size(*out) - n_start;
}

typedef struct {
    vec2s pos;
    f32 r;
    DYNLIST(sector_t*) *out;
    map_t visited;
} sectors_in_radius_data_t;

static bool traverse_sectors_in_radius(
    level_t *level,
    block_t *block,
    ivec2s,
    sectors_in_radius_data_t *data) {
    typedef struct {
        sector_t *ptr;
        vec2s pos;
    } queue_entry_t;

    DYNLIST(queue_entry_t) queue = NULL;

    dynlist_each(block->sectors, it) {
        if (!map_find(&data->visited, *it.el)) {
            *dynlist_push(queue) = (queue_entry_t) {
                .ptr = *it.el,
                .pos =  data->pos
            };
        }
    }

    while (dynlist_size(queue) != 0) {
        queue_entry_t entry = dynlist_pop(queue);
        sector_t *s = entry.ptr;

        if (map_find(&data->visited, s)) {
            continue;
        }

        const vec2s pos = entry.pos;
        map_insert(&data->visited, s, NULL);

        const vec2s c = sector_clamp_point(s, pos);
        if (glms_vec2_norm2(glms_vec2_sub(pos, c)) < data->r * data->r) {
            *dynlist_push(*data->out) = s;

            // enqueue non-visited immediate neighbors
            llist_each(sector_sides, &s->sides, it) {
                if (it.el->portal
                    && it.el->portal->sector
                    && !map_find(&data->visited, it.el->portal->sector)) {
                    const vec2s entry_pos =
                        (it.el->flags & SIDE_FLAG_DISCONNECT) ?
                            portal_transform(
                                    level,
                                    it.el,
                                    it.el->portal,
                                    data->pos)
                            : data->pos;

                    *dynlist_push(queue) =
                        (queue_entry_t) {
                            .ptr = it.el->portal->sector,
                            .pos = entry_pos
                        };
                }
            }
        }
    }

    dynlist_free(queue);
    return true;
}

int level_sectors_in_radius(
    level_t *level, vec2s pos, f32 r, DYNLIST(sector_t*) *out) {
    sectors_in_radius_data_t data = {
        .pos = pos,
        .r = r,
        .out = out
    };

    map_init(
        &data.visited,
        map_hash_id, NULL, NULL, NULL, map_cmp_id, NULL, NULL, NULL);

    const int n_start = dynlist_size(*out);
    level_traverse_block_area(
        level,
        glms_vec2_sub(pos, VEC2(r)),
        glms_vec2_add(pos, VEC2(r)),
        (traverse_blocks_f) traverse_sectors_in_radius,
        &data);

    map_destroy(&data.visited);
    return dynlist_size(*out) - n_start;
}
