#pragma once

#include "util/math.h"
#include "defs.h"
#include "config.h"

// level pos -> block pos
ALWAYS_INLINE ivec2s level_pos_to_block(vec2s point) {
    return IVEC2(point.x / BLOCK_SIZE, point.y / BLOCK_SIZE);
}

// get block position from block
ivec2s level_block_to_blockpos(level_t *level, block_t *block);

// get block at blockpos, NULL if out of bounds/not found
block_t *level_get_block(level_t *level, ivec2s blockpos);

// remove a sector from block data
void level_blocks_remove_sector(level_t *level, sector_t *sector);

// remove a wall from block data
void level_blocks_remove_wall(level_t *level, wall_t *wall);

// remove an object from block data
void level_blocks_remove_object(level_t *level, object_t *object);

// recalculate/reset all blocks
void level_reset_blocks(level_t *level);

// resize for current level bounds
void level_resize_blocks(level_t *level);

typedef bool (*traverse_blocks_f)(level_t*, block_t*, ivec2s, void*);

void level_traverse_blocks(
    level_t *level,
    vec2s from,
    vec2s to,
    traverse_blocks_f callback,
    void *userdata);

void level_traverse_block_area(
    level_t *level,
    vec2s mi,
    vec2s ma,
    traverse_blocks_f callback,
    void *userdata);

void level_update_wall_blocks(
    level_t *level,
    wall_t *wall,
    const vec2s *old_v0,
    const vec2s *old_v1);

void level_update_sector_blocks(
    level_t *level,
    sector_t *sector,
    const vec2s *old_min,
    const vec2s *old_max);

void level_set_subsector_blocks(
    level_t *level,
    subsector_t *sub);

void level_blocks_remove_subsector(
    level_t *level,
    subsector_t *sub);
