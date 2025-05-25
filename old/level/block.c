#include "level/block.h"
#include "level/level_defs.h"
#include "level/level.h"

ivec2s level_block_to_blockpos(level_t *level, block_t *block) {
    const int i = (int) (block - level->blocks.arr);
    return IVEC2(i % level->blocks.size.x, i / level->blocks.size.x);
}

block_t *level_get_block(level_t *level, ivec2s block_pos) {
    if (!level->blocks.arr) {
        return NULL;
    }

    if (block_pos.x < level->blocks.offset.x
        || block_pos.y < level->blocks.offset.y
        || block_pos.x >= (level->blocks.offset.x + level->blocks.size.x)
        || block_pos.y >= (level->blocks.offset.y + level->blocks.size.y)) {
        return NULL;
    }

    return &level->blocks.arr[
        (block_pos.y - level->blocks.offset.y) * level->blocks.size.x
            + (block_pos.x - level->blocks.offset.x)];
}

void level_blocks_remove_sector(level_t *level, sector_t *sect) {
    if (!level->blocks.arr) { return; }

    const ivec2s
        bmin = level_pos_to_block(sect->min),
        bmax = level_pos_to_block(sect->max);

    for (int by = bmin.y; by <= bmax.y; by++) {
        for (int bx = bmin.x; bx <= bmax.x; bx++) {
            block_t *block = level_get_block(level, IVEC2(bx, by));

            if (!block) {
                WARN("removing from bad block %d, %d", bx, by);
                continue;
            }

            dynlist_each(block->sectors, it) {
                if (*it.el == sect) {
                    dynlist_remove_it(block->sectors, it);
                    break;
                }
            }
        }
    }
}

void level_blocks_remove_wall(level_t *level, wall_t *wall) {
    if (!level->blocks.arr) { return; }

    const ivec2s
        bmin = level_pos_to_block(wall->v0->pos),
        bmax = level_pos_to_block(wall->v1->pos);

    for (int by = bmin.y; by <= bmax.y; by++) {
        for (int bx = bmin.x; bx <= bmax.x; bx++) {
            block_t *block = level_get_block(level, IVEC2(bx, by));
            ASSERT(block);

            dynlist_each(block->walls, it) {
                if (*it.el == wall) {
                    dynlist_remove_it(block->walls, it);
                    break;
                }
            }
        }
    }
}

void level_blocks_remove_object(level_t *level, object_t *object) {
    if (!level->blocks.arr) { return; }

    if (!object->block) {
        WARN("object %d has no block", object->index);
    } else {
        dlist_remove(block_list, &object->block->objects, object);
    }
}

void level_reset_blocks(level_t *level) {
    level_resize_blocks(level);

    const ivec2s offset = level->blocks.offset, size = level->blocks.size;
    for (int by = offset.y; by < offset.y + size.y; by++) {
        for (int bx = offset.x; bx < offset.x + size.x; bx++) {
            block_t *block = level_get_block(level, IVEC2(bx, by));
            dynlist_free(block->sectors);
            dynlist_free(block->walls);
            dynlist_free(block->subsectors);
            dlist_init(&block->objects);
        }
    }

    level_dynlist_each(level->sectors, it) {
        level_update_sector_blocks(level, *it.el, NULL, NULL);

        dynlist_each((*it.el)->subs, it_s) {
            level_set_subsector_blocks(level, it_s.el);
        }
    }

    level_dynlist_each(level->walls, it) {
        level_update_wall_blocks(level, *it.el, NULL, NULL);
    }

    level_dynlist_each(level->objects, it) {
        dlist_init_node(&(*it.el)->block_list);

        block_t *block =
            level_get_block(level, level_pos_to_block((*it.el)->pos));

        dlist_prepend(block_list, &block->objects, *it.el);
    }
}

void level_resize_blocks(level_t *level) {
    const ivec2s
        old_size = level->blocks.size,
        old_offset = level->blocks.offset,
        new_size = IVEC2(
            max((level->bounds.max.x - level->bounds.min.x) / BLOCK_SIZE, 1) + 1,
            max((level->bounds.max.y - level->bounds.min.y) / BLOCK_SIZE, 1) + 1
        ),
        new_offset = IVEC2(
            level->bounds.min.x / BLOCK_SIZE,
            level->bounds.min.y / BLOCK_SIZE
        );

    if (level->blocks.arr
        && new_size.x == old_size.x
        && new_size.y == old_size.y
        && new_offset.x == old_offset.x
        && new_offset.y == old_offset.y) {
        // nothing to do
        return;
    }

    // realloc
    block_t *old_arr = level->blocks.arr;
    block_t *new_arr = calloc(1, new_size.x * new_size.y * sizeof(block_t));

    // copy
    if (old_arr) {
        for (int by = old_offset.y; by < old_offset.y + old_size.y; by++) {
            for (int bx = old_offset.x; bx < old_offset.x + old_size.x; bx++) {
                block_t *old =
                    &old_arr[
                        (by - old_offset.y) * old_size.x + (bx - old_offset.x)];

                // out of bounds? deallocate
                if (bx - new_offset.x < 0
                    || by - new_offset.y < 0
                    || bx - new_offset.x >= new_size.x
                    || by - new_offset.y >= new_size.y) {
                    dynlist_free(old->sectors);
                    dynlist_free(old->walls);

                    dlist_each(block_list, &old->objects, it) {
                        dlist_init_node(&it.el->block_list);
                    }
                } else {
                    new_arr[
                        (by - new_offset.y) * new_size.x
                            + (bx - new_offset.x)] = *old;
                }
            }
        }

        free(old_arr);
    }

    // reassign
    level->blocks.arr = new_arr;
    level->blocks.offset = new_offset;
    level->blocks.size = new_size;
}

void level_traverse_blocks(
    level_t *level,
    vec2s from,
    vec2s to,
    traverse_blocks_f callback,
    void *userdata) {
    if (!level->blocks.arr) { return; }

    // DDA: see lodev.org/cgtutor/raycasting.html
    const vec2s
        dir = glms_vec2_normalize(glms_vec2_sub(to, from)),
        bfrom = glms_vec2_divs(from, BLOCK_SIZE);

    ivec2s
        bpos = VEC_TO_I(glms_vec2_divs(from, BLOCK_SIZE)),
        bstop = VEC_TO_I(glms_vec2_divs(to, BLOCK_SIZE));

    vec2s
        delta_dist = VEC2(
            fabsf(dir.x) <= 0.0000001f ? 1e30 : fabsf(1 / dir.x),
            fabsf(dir.y) <= 0.0000001f ? 1e30 : fabsf(1 / dir.y)
            ),
        side_dist = VEC2(
            delta_dist.x
                * (dir.x < 0 ?
                    (bfrom.x - floorf(bfrom.x))
                    : (ceilf(bfrom.x) - bfrom.x)),
            delta_dist.y
                * (dir.y < 0 ?
                    (bfrom.y - floorf(bfrom.y))
                    : (ceilf(bfrom.y) - bfrom.y))
                );

    const ivec2s bstep = IVEC2(dir.x < 0 ? -1 : 1, dir.y < 0 ? -1 : 1);

    while (true) {
        block_t *block = level_get_block(level, bpos);

        if (!block) {
            WARN("out of blockmap range at %d, %d", bpos.x, bpos.y);
            return;
        }

        if (!callback(level, block, bpos, userdata)) {
            return;
        }

        if (bpos.x == bstop.x && bpos.y == bstop.y) {
            break;
        }

        if (side_dist.x < side_dist.y) {
            side_dist.x += delta_dist.x;
            bpos.x += bstep.x;
        } else {
            side_dist.y += delta_dist.y;
            bpos.y += bstep.y;
        }
    }
}

void level_traverse_block_area(
    level_t *level,
    vec2s mi,
    vec2s ma,
    traverse_blocks_f callback,
    void *userdata) {
    if (!level->blocks.arr) { return; }

    const ivec2s
        bmin = level_pos_to_block(mi),
        bmax = level_pos_to_block(ma);

    for (int by = bmin.y; by <= bmax.y; by++) {
        for (int bx = bmin.x; bx <= bmax.x; bx++) {
            block_t *block = level_get_block(level, IVEC2(bx, by));
            if (!block) { WARN("no block?"); continue; }

            if (!callback(level, block, IVEC2(bx, by), userdata)) {
                return;
            }
        }
    }
}

static bool update_wall_blocks_traverse_remove(
    level_t *level,
    block_t *block,
    ivec2s,
    wall_t *w) {
    dynlist_each(block->walls, it) {
        if (*it.el == w) {
            dynlist_remove_it(block->walls, it);
            return true;
        }
    }

    WARN("wall not found in block");
    return true;
}

static bool update_wall_blocks_traverse_add(
    level_t *level,
    block_t *block,
    ivec2s,
    wall_t *w) {
    *dynlist_push(block->walls) = w;
    return true;
}

void level_update_wall_blocks(
    level_t *level,
    wall_t *wall,
    const vec2s *old_v0,
    const vec2s *old_v1) {
    if (old_v0 && old_v1) {
        level_traverse_blocks(
            level,
            *old_v0,
            *old_v1,
            (traverse_blocks_f) update_wall_blocks_traverse_remove,
            wall);
    }

    level_traverse_blocks(
        level,
        wall->v0->pos,
        wall->v1->pos,
        (traverse_blocks_f) update_wall_blocks_traverse_add,
        wall);
}

void level_update_sector_blocks(
    level_t *level,
    sector_t *sector,
    const vec2s *old_min,
    const vec2s *old_max) {
    // remove from old blocks
    if (old_min && old_max) {
        const ivec2s
            bmin = level_pos_to_block(*old_min),
            bmax = level_pos_to_block(*old_max);

        for (int by = bmin.y; by <= bmax.y; by++) {
            for (int bx = bmin.x; bx <= bmax.x; bx++) {
                block_t *block = level_get_block(level, IVEC2(bx, by));
                if (!block) { continue; }

                dynlist_each(block->sectors, it) {
                    if (*it.el == sector) {
                        dynlist_remove_it(block->sectors, it);
                        return;
                    }
                }
            }
        }
    }

    // add to new blocks
    const ivec2s
        bmin = level_pos_to_block(sector->min),
        bmax = level_pos_to_block(sector->max);

    for (int by = bmin.y; by <= bmax.y; by++) {
        for (int bx = bmin.x; bx <= bmax.x; bx++) {
            block_t *block = level_get_block(level, IVEC2(bx, by));
            if (!block) { WARN("out of bounds?"); continue; }

            *dynlist_push(block->sectors) = sector;
        }
    }
}

static bool set_subsector_blocks_traverse(
    level_t *level,
    block_t *block,
    ivec2s pos,
    subsector_t *sub) {
    *dynlist_push(block->subsectors) = sub->id;
    return true;
}

void level_set_subsector_blocks(level_t *level, subsector_t *sub) {
    level_traverse_block_area(
        level,
        sub->min,
        sub->max,
        (traverse_blocks_f) set_subsector_blocks_traverse,
        sub);
}

static bool remove_subsector_blocks_traverse(
    level_t *level,
    block_t *block,
    ivec2s,
    subsector_t *sub) {
    dynlist_each(block->subsectors, it) {
        if (*it.el == sub->id) {
            dynlist_remove_it(block->subsectors, it);
            break;
        }
    }

    return true;
}

void level_blocks_remove_subsector(
    level_t *level,
    subsector_t *sub) {
    level_traverse_block_area(
        level,
        sub->min,
        sub->max,
        (traverse_blocks_f) remove_subsector_blocks_traverse,
        sub);
}
