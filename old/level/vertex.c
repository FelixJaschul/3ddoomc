#include "level/vertex.h"
#include "level/level.h"
#include "level/lptr.h"
#include "level/sector.h"
#include "level/wall.h"
#include "state.h"

vertex_t *vertex_new(level_t *level, vec2s pos) {
    vertex_t *v = level_alloc(level, level->vertices);
    v->pos = pos;
    vertex_recalculate(level, v);
    return v;
}

void vertex_delete(level_t *level, vertex_t *v) {
#ifdef MAPEDITOR
    if (state->mode == GAMEMODE_EDITOR) {
        /*TODO:  e_close_for_ptr(&state->editor, LPTR_FROM(v)); */
    }
#endif //ifdef MAPEDITOR

    // prevent modification by l_remove_wall under iteration
    DYNLIST(wall_t*) walls = dynlist_copy(v->walls);

    dynlist_each(walls, it) {
        ASSERT(v == (*it.el)->v0 || v == (*it.el)->v1);
        wall_delete(level, *it.el);
    }

    dynlist_free(v->walls);
    dynlist_free(walls);

    level_free(level, level->vertices, v);
}

void vertex_recalculate(level_t *level, vertex_t *vertex) {
    if (vertex->level_flags & LF_DO_NOT_RECALC) { return; }
    level->version++;
    vertex->version++;

    vertex->pos = glms_vec2_maxv(vertex->pos, VEC2(0));

    // recalculate all walls
    dynlist_each(vertex->walls, it) {
        wall_recalculate(level, *it.el);
    }
}

// set vertex position
void vertex_set(level_t *level, vertex_t *vertex, vec2s pos) {
    vertex->pos = glms_vec2_maxv(pos, VEC2(0));
    vertex_recalculate(level, vertex);
}

/* vertex_t *vertex_find(level_t *level, u16 x, u16 y) { */
/*     // TODO: search by block? */
/*     l_dynlist_each(level->vertices, it) { */
/*         vertex_t *v = *it.el; */
/*         if (v->x == x && v->y == y) { */
/*             return v; */
/*         } */
/*     } */

/*     return NULL; */
/* } */

/* vertex_t *vertex_or_new(level_t *level, u16 x, u16 y) { */
/*     vertex_t *v = vertex_find(level, x, y); */
/*     return v ? v : l_new_vertex(level, x, y); */
/* } */

int vertex_walls(
    level_t *level, vertex_t *v, wall_t **walls, int n) {
    int i = 0;
    dynlist_each(v->walls, it) {
        if (i == n) { break; }
        walls[i++] = *it.el;
    }
    return i;
}

int vertex_sides(
    level_t *level, vertex_t *v, side_t **sides, int n) {
    int i = 0, count = 0;
    dynlist_each(v->walls, it) {
        wall_t *w = *it.el;

        for (int i = 0; count < n && i < 2; i++) {
            if (w->sides[i]) { continue; }

            if (count >= n) {
                goto done;
            }

            sides[count++] = w->sides[i];
        }
    }

done:
    return i;
}

vertex_t *vertex_other(
    level_t *level, vertex_t *v, wall_t *wall) {
    if (v == wall->v0) {
        return wall->v1;
    } else if (v == wall->v1) {
        return wall->v0;
    }

    WARN("vertex_other returnining NULL");
    DUMPTRACE();
    return NULL;
}

wall_t *vertices_shared_wall(
    const level_t *level,
    const vertex_t *v0,
    const vertex_t *v1) {
    dynlist_each(v0->walls, it) {
        wall_t *wall = *it.el;

        if ((v0 == wall->v0 && v1 == wall->v1)
            || (v0 == wall->v1 && v1 == wall->v0)) {
            return wall;
        }
    }

    return NULL;
}

sector_t *vertices_shared_sector(
    const level_t *level,
    const vertex_t *v0,
    const vertex_t *v1) {
    dynlist_each(v0->walls, it) {
        const wall_t *wall = *it.el;

        const sector_t
            *c0 = wall->side0 ? wall->side0->sector : NULL,
            *c1 = wall->side1 ? wall->side1->sector : NULL;

        if (c0 && sector_contains_vertex(level, c0, v0)) {
            return (sector_t*) c0;
        } else if (c1 && sector_contains_vertex(level, c1, v1)) {
            return (sector_t*) c1;
        }
    }

    return NULL;
}
