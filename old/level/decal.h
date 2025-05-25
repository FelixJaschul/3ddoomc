#pragma once

#include "level/level_defs.h"

decal_t *decal_new(level_t *level);
void decal_delete(level_t *level, decal_t *decal);

void decal_tick(level_t *level, decal_t *decal);

// recalulates decal firelds after adjustment or adjustment to parent
void decal_recalculate(level_t *level, decal_t *decal);

// get decal pos in world space
vec2s decal_worldpos(const decal_t *decal);

// set side decal is on
void decal_set_side(
    level_t *level, decal_t *decal, side_t *side);

// set sector decal is on
void decal_set_sector(
    level_t *level, decal_t *decal, sector_t *sector, plane_type plane);
