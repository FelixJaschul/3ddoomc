#pragma once

#include "util/math.h"
#include "defs.h"

// create new side optionally like another
side_t *side_new(level_t *level, const side_t *like);

// remove side
void side_delete(level_t *level, side_t *side);

// recalculate side fields
void side_recalculate(level_t *level, side_t *side);

// true if side is left for its wall
bool side_is_left(const side_t *side);

// get index of side on wall
// returns -1 if side is not on wall
int side_index(const side_t *side);

// get side (possibly NULL) opposite of side
side_t *side_other(const side_t *side);

// get vertices for the wall side such that the side is on the right side of the
// line formed by the vertices
void side_get_vertices(
    const side_t *side,
    vertex_t *vs[2]);

// gets point which is slightly off of the side wall in the direction of the
// side normal
vec2s side_normal_point(const side_t *side);

// get wall normal corresponding to side
vec2s side_normal(const side_t *side);

// convert a list of sides [(v0.x, v0.y) -> (v1.x, v1.y) ...] into a list of the
// vertices of each side in order
// fails if n < nsides
void sides_to_vertices(
    side_t **sides,
    int nsides,
    vec2s vs[][2],
    int n);

// get visible segments for side
int side_get_segments(const side_t *side, side_segment_t *segs);

// get wall a -> b, but with this side's sign
vec2s side_a_to_b(const side_t *side);

// travel direction of side a -> b
vec2s side_dir(const side_t *side);
