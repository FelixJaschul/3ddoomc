#pragma once

#include "level/level_defs.h"
#include "defs.h"

ALWAYS_INLINE bool sect_line_eq(const sect_line_t *a, const sect_line_t *b) {
    return
        (glms_vec2_eqv_eps(a->a->pos, b->a->pos)
         && glms_vec2_eqv_eps(a->b->pos, b->b->pos))
        || (glms_vec2_eqv_eps(a->b->pos, b->a->pos)
            && glms_vec2_eqv_eps(a->a->pos, b->b->pos));
}

// create a sector
// copies properties from "like" if non-NULL
sector_t *sector_new(
    level_t *level,
    const sector_t *like);
void sector_delete(level_t *level, sector_t *sector);
void sector_delete_with_sides(level_t *level, sector_t *sector);

void sector_tick(level_t *level, sector_t *sector);

// see sector_traverse_neighbors
typedef bool (*sector_traverse_neighbors_f)(level_t*, sector_t*, void*);

// traverse sector neighbors, calling "callback for each"
void sector_traverse_neighbors(
    level_t *level,
    sector_t *sector,
    sector_traverse_neighbors_f callback,
    void *userdata);

// true if vertex is part of sector
bool sector_contains_vertex(
    const level_t *level,
    const sector_t *sector,
    const vertex_t *vertex);

// true if point is inside of sector
bool sector_contains_point(
    const sector_t *sector,
    vec2s point);

// clamp point to be inside of sector
vec2s sector_clamp_point(
    const sector_t *sector,
    vec2s point);

// true if sector intersect (or contains!) lines
bool sector_intersects_line(
    const sector_t *sector,
    vec2s a,
    vec2s b);

// get sides which form sector, n_sides must be >= sector->n_sides
// returns number of sides written to sides array
int sector_get_sides(
    level_t *level,
    sector_t *sector,
    side_t **sides,
    int n_sides);

// update sector's PVS
void sector_compute_visibility(level_t *level, sector_t *sector);

// recalculates fields after update to sides, etc.
void sector_recalculate(level_t *level, sector_t *sect);

// find subsector of point in sector, NULL if not found
subsector_t *sector_find_subsector(sector_t *sector, vec2s point);

// adds a side to a sector
void sector_add_side(
    level_t *level,
    sector_t *sector,
    side_t *side);

// removes a side from a sector
void sector_remove_side(
    level_t *level,
    sector_t *sector,
    side_t *side);

// compute light level of a sector
u8 sector_light(
    const level_t *level,
    const sector_t *sector);

// compute current texture offsets for sector
ivec2s sector_tex_offset(
    const level_t *level,
    const sector_t *sector,
    plane_type plane);
