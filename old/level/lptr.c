#include "level/lptr.h"
#include "level/decal.h"
#include "level/level_defs.h"
#include "level/level.h"
#include "level/object.h"
#include "level/sectmat.h"
#include "level/sector.h"
#include "level/side.h"
#include "level/sidemat.h"
#include "level/vertex.h"
#include "level/wall.h"
#include "util/rand.h"
#include "util/str.h"

int lptr_to_str(char *dst, usize n, const level_t *level, lptr_t ptr) {
    if (LPTR_IS_NULL(ptr)) {
        return snprintf(dst, n, "%s", "(NULL)");
    } else if (!lptr_is_valid(level, ptr)) {
        return snprintf(dst, n, "%s", "(INVALID)");
    } else {
        int i = 0;
        i += level_types_to_str(dst, n, LPTR_TYPE(ptr));
        i +=
            xnprintf(
                dst,
                n,
                " (%d)",
                lptr_is_valid(level, ptr) ? lptr_to_index(level, ptr) : 0);
        return i;
    }
}

int lptr_to_fancy_str(char *dst, usize n, const level_t *level, lptr_t ptr) {
    int i = 0;
    i += lptr_to_str(dst, n, level, ptr);

    if (lptr_is_valid(level, ptr)) {
        switch (LPTR_TYPE(ptr)) {
        case T_SECTOR: {
            sector_t *s = LPTR_SECTOR(level, ptr);
            const vec2s span = glms_vec2_sub(s->max, s->min);
            i += xnprintf(dst, n, " (%.3f x %.3f)", span.x, span.y);
        } break;
        case T_OBJECT: {
            object_t *o = LPTR_OBJECT(level, ptr);
            i += xnprintf(dst, n, " (%s)", object_type_index_to_str(o->type_index));
        } break;
        }
    }

    return i;
}

u16 lptr_to_index(const level_t *level, lptr_t ptr) {
    return LPTR_IS_NULL(ptr) ? 0 : *LPTR_FIELD(level, ptr, index);
}

bool lptr_is_valid(const level_t *level, lptr_t ptr) {
    if (ptr.index == 0) { return false; }
    DYNLIST(void*) *plist = level_get_list(level, ptr.type);
    if (dynlist_size(*plist) <= ptr.index || !(*plist)[ptr.index]) {
        return false;
    }
    return level_get_gen(level, ptr.type, ptr.index) == ptr.gen;
}

// make lptr from tag and arbitrary pointer
lptr_t lptr_make(int tag, void *ptr) {
    switch (tag) {
    case T_VERTEX:  return LPTR_FROM((vertex_t*)  ptr);
    case T_WALL:    return LPTR_FROM((wall_t*)    ptr);
    case T_SIDE:    return LPTR_FROM((side_t*)    ptr);
    case T_SIDEMAT: return LPTR_FROM((sidemat_t*) ptr);
    case T_SECTOR:  return LPTR_FROM((sector_t*)  ptr);
    case T_SECTMAT: return LPTR_FROM((sectmat_t*) ptr);
    case T_DECAL:   return LPTR_FROM((decal_t*)   ptr);
    case T_OBJECT:  return LPTR_FROM((object_t*)  ptr);
    default: ASSERT(false);
    }
}

u8 lptr_rand_palette(lptr_t ptr) {
    rand_t r = rand_create(((int) ptr.type) << 16 | (int) ptr.index );
    return rand_n(&r, 0, 255);
}

void lptr_delete(level_t *level, lptr_t ptr) {
    if (!lptr_is_valid(level, ptr)) {
        char name[64];
        lptr_to_str(name, sizeof(name), level, ptr);
        WARN("attempt to delete invalid lptr %s", name);
        return;
    }

    switch (LPTR_TYPE(ptr)) {
    case T_VERTEX:  vertex_delete(level, LPTR_VERTEX(level, ptr));  break;
    case T_WALL:  wall_delete(level, LPTR_WALL(level, ptr));  break;
    case T_SIDE:  side_delete(level, LPTR_SIDE(level, ptr));  break;
    case T_SIDEMAT: sidemat_delete(level, LPTR_SIDEMAT(level, ptr)); break;
    case T_SECTOR:  sector_delete_with_sides(level, LPTR_SECTOR(level, ptr));  break;
    case T_SECTMAT: sectmat_delete(level, LPTR_SECTMAT(level, ptr)); break;
    case T_DECAL:   decal_delete(level, LPTR_DECAL(level, ptr));   break;
    case T_OBJECT:  object_delete(level, LPTR_OBJECT(level, ptr));  break;
    default: ASSERT(false);
    }
}

void lptr_recalculate(level_t *level, lptr_t ptr) {
    if (!lptr_is_valid(level, ptr)) {
        char name[64];
        lptr_to_str(name, sizeof(name), level, ptr);
        WARN("attempt to recalc invalid lptr %s", name);
        return;
    }

    switch (LPTR_TYPE(ptr)) {
    case T_VERTEX:  vertex_recalculate(level,  LPTR_VERTEX(level,  ptr)); break;
    case T_WALL:    wall_recalculate(level,    LPTR_WALL(level,    ptr)); break;
    case T_SIDE:    side_recalculate(level,    LPTR_SIDE(level,    ptr)); break;
    case T_SIDEMAT: sidemat_recalculate(level, LPTR_SIDEMAT(level, ptr)); break;
    case T_SECTOR:  sector_recalculate(level,  LPTR_SECTOR(level,  ptr)); break;
    case T_SECTMAT: sectmat_recalculate(level, LPTR_SECTMAT(level, ptr)); break;
    case T_DECAL:   decal_recalculate(level,   LPTR_DECAL(level,   ptr)); break;
    case T_OBJECT:  break;
    default: ASSERT(false);
    }
}

void *lptr_raw_ptr(level_t *level, lptr_t ptr) {
    switch (LPTR_TYPE(ptr)) {
    case T_VERTEX:  return level->vertices[ptr.index];
    case T_WALL:    return level->walls[ptr.index];
    case T_SIDE:    return level->sides[ptr.index];
    case T_SIDEMAT: return level->sidemats[ptr.index];
    case T_SECTOR:  return level->sectors[ptr.index];
    case T_SECTMAT: return level->sectmats[ptr.index];
    case T_DECAL:   return level->decals[ptr.index];
    case T_OBJECT:  return level->objects[ptr.index];
    default: ASSERT(false);
    }
 }

u32 lptr_raw(lptr_t ptr) {
    return ptr.raw;
}

lptr_t lptr_from_raw(u32 rawval) {
    return (lptr_t) { .raw = rawval };
}

lptr_nogen_t lptr_to_nogen(lptr_t ptr) {
    return (lptr_nogen_t) {
        .type = ptr.type,
        .index = ptr.index
    };
}

lptr_t lptr_from_nogen(level_t *level, lptr_nogen_t ptr) {
    if (ptr.type == 0) { return LPTR_NULL; }

    return (lptr_t) {
        .type = ptr.type,
        .gen = level_get_gen(level, ptr.type, ptr.index),
        .index = ptr.index
    };
}

lptr_nogen_t lptr_nogen_from_raw(u32 raw) {
    return (lptr_nogen_t) { .raw = raw };
}
