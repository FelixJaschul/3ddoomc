#include "level/side.h"
#include "editor/editor.h"
#include "level/level_defs.h"
#include "level/level.h"
#include "level/sector.h"
#include "level/side_texinfo.h"
#include "level/wall.h"
#include "level/decal.h"
#include "level/tag.h"
#include "level/lptr.h"
#include "state.h"

side_t *side_new(
    level_t *level,
    const side_t *like) {
    side_t *s = level_alloc(level, level->sides);
    side_texinfo_init(&s->tex_info);

    if (like) {
        s->mat = like->mat;
        s->cos_type = like->cos_type;
        s->func_type = like->func_type;
    } else {
        s->mat = level->sidemats[SIDEMAT_NOMAT];
        s->cos_type = 0;
        s->func_type = 0;
    }

    // don't recalculate - nothing to update
    return s;
}

void side_delete(
    level_t *level,
    side_t *side) {
#ifdef MAPEDITOR
    if (state->mode == GAMEMODE_EDITOR) {
        editor_close_for_ptr(state->editor, LPTR_FROM(side));
    }
#endif //ifdef MAPEDITOR

    if (side->sector) {
        sector_remove_side(level, side->sector, side);
    }

    // TODO: can this be done without traversing all sides?
    // remove portals to this side
    level_dynlist_each(level->sides, it) {
        side_t *other = *it.el;
        if (other->portal == side) {
            other->portal = NULL;
            /* TODO: level_dirty(level, LPTR_FROM(other)); */
        }
    }

    // remove from wall
    if (side->wall) {
        wall_set_side(level, side->wall, side_index(side), NULL);
    }

    // remove all decals
    llist_each(node, &side->decals, it) {
        decal_delete(level, it.el);
    }

    tag_set(level, LPTR_FROM(side), TAG_NONE);
    LOG("DELETING SIDE %d", side->index);
    level_free(level, level->sides, side);
}

void side_recalculate(level_t *level, side_t *side) {
    if (side->level_flags & LF_DO_NOT_RECALC) { return; }
    level->version++;
    side->version++;

    // enqueue for sector update
    *dynlist_push(level->dirty_sides) = LPTR_FROM(side);

    bool on_wall = false;
    for (int i = 0; i < 2; i++) {
        on_wall |= side->wall->sides[i] == side;
    }

    ASSERT(on_wall, "side references wall but wall does not have side");

    if (side->portal && side->portal != side_other(side)) {
        side->flags |= SIDE_FLAG_DISCONNECT;
    } else {
        side->flags &= ~SIDE_FLAG_DISCONNECT;
    }

    // recalculate for decals
    llist_each(node, &side->decals, it) {
        decal_recalculate(level, it.el);
    }
}

int side_index(const side_t *side) {
    return side->wall ? (side->wall->side0 == side ? 0 : 1) : -1;
}

side_t *side_other(const side_t *side) {
    if (!side->wall) {
        return NULL;
    }

    return side->wall->side0 == side ? side->wall->side1 : side->wall->side0;
}

vec2s side_normal_point(
    const side_t *side) {
    if (!side->wall) { return VEC2(0); }
    const f32 s = side_is_left(side) ? 1 : -1;
    const vec2s midpoint = wall_midpoint(side->wall);
    return
        VEC2(
            midpoint.x + 0.0001f * s * side->wall->normal.x,
            midpoint.y + 0.0001f * s * side->wall->normal.y);
}

vec2s side_normal(const side_t *side) {
    if (!side->wall) { return VEC2(0); }
    const f32 s = side_is_left(side) ? 1 : -1;
    return
        VEC2(
            s * side->wall->normal.x,
            s * side->wall->normal.y);
}

void sides_to_vertices(
    side_t **sides,
    int nsides,
    vec2s vs[][2],
    int n) {
    for (int i = 0; i < nsides; i++) {
        vertex_t *verts[2];
        side_get_vertices(sides[i], verts);
        vs[i][0] = verts[0]->pos;
        vs[i][1] = verts[1]->pos;
    }
}

bool side_is_left(const side_t *side) {
    return side && side->wall && side == side->wall->sides[0];
}

void side_get_vertices(
    const side_t *side,
    vertex_t *vs[2]) {
    if (!side->wall) {
        vs[0] = NULL;
        vs[1] = NULL;
    } else if (side_is_left(side)) {
        vs[0] = side->wall->v0;
        vs[1] = side->wall->v1;
    } else {
        vs[0] = side->wall->v1;
        vs[1] = side->wall->v0;
    }
}

int side_get_segments(const side_t *side, side_segment_t *segs) {
    memset(segs, 0, 4 * sizeof(segs[0]));

    if (!side->portal) {
        // only wall
        segs[SIDE_SEGMENT_WALL] =
            (side_segment_t) {
                .present = true,
                .mesh = true,
                .index = SIDE_SEGMENT_WALL,
                .z0 = side->sector->floor.z,
                .z1 = side->sector->ceil.z,
            };
        return 1;
    }

    if (!side->portal->sector) {
        // no segments, bad portal
        return 0;
    }

    int n = 0;

    const bool disconnect = side->flags & SIDE_FLAG_DISCONNECT;

    const side_t *port = side->portal;
    const sector_t *sect = side->sector, *port_sect = port->sector;

    // bottom segment only rendered if not disconnected - disconnected portals
    // always have aligning floors
    if (!disconnect && port_sect->floor.z > sect->floor.z) {
        segs[SIDE_SEGMENT_BOTTOM] =
            (side_segment_t) {
                .present = true,
                .mesh = true,
                .index = SIDE_SEGMENT_BOTTOM,
                .z0 = sect->floor.z,
                .z1 = port_sect->floor.z,
            };
        n++;
    }

    const f32
        mid_z0 =
            disconnect ?
                sect->floor.z
                : port_sect->floor.z,
        mid_z1 =
            disconnect ?
                mid_z0 + (port_sect->ceil.z - port_sect->floor.z)
                : port_sect->ceil.z;

    if ((!disconnect && port_sect->ceil.z < sect->ceil.z)
        || (disconnect && (mid_z1 - mid_z0) < (sect->ceil.z - sect->floor.z))) {
        segs[SIDE_SEGMENT_TOP] =
            (side_segment_t) {
                .present = true,
                .mesh = true,
                .index = SIDE_SEGMENT_TOP,
                .z0 = mid_z1,
                .z1 = sect->ceil.z,
            };
        n++;
    }

    // middle segment exists for all portals
    segs[SIDE_SEGMENT_MIDDLE] =
        (side_segment_t) {
            .present = true,
            .portal = true,
            .mesh = disconnect,
            .index = SIDE_SEGMENT_MIDDLE,
            .z0 = mid_z0,
            .z1 = mid_z1,
        };
    n++;

    // TODO: middle segment for portals
    return n;
}

vec2s side_a_to_b(const side_t *side) {
    return glms_vec2_scale(side->wall->d, side_is_left(side) ? 1 : -1);
}

vec2s side_dir(const side_t *side) {
    return glms_vec2_normalize(
        glms_vec2_scale(side->wall->d, side_is_left(side) ? 1 : -1));
}
