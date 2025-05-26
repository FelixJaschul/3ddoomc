#include "level/decal.h"
#include "editor/editor.h"
#include "level/level_defs.h"
#include "level/level.h"
#include "level/lptr.h"
#include "level/sector.h"
#include "level/side.h"
#include "level/tag.h"
#include "state.h"

decal_t *decal_new(level_t *level) {
    decal_t *d = level_alloc(level, level->decals);
    d->tex_base = AS_RESOURCE(TEXTURE_NOTEX);
    d->tex = AS_RESOURCE(TEXTURE_NOTEX);
    d->ticks = -1;
    return d;
}

void decal_delete(level_t *level, decal_t *decal) {
#ifdef MAPEDITOR
    if (state->mode == GAMEMODE_EDITOR) {
        editor_close_for_ptr(state->editor, LPTR_FROM(decal));
    }
#endif //ifdef MAPEDITOR

    if (decal->is_on_side) {
        decal->side.ptr->n_decals--;
        llist_remove(node, &decal->side.ptr->decals, decal);
        side_recalculate(level, decal->side.ptr);
    } else {
        decal->sector.ptr->n_decals--;
        llist_remove(node, &decal->sector.ptr->decals, decal);
        sector_recalculate(level, decal->sector.ptr);
    }

    tag_set(level, LPTR_FROM(decal), TAG_NONE);
    level_free(level, level->decals, decal);
}

void decal_tick(level_t *level, decal_t *decal) {
    if (decal->ticks != -1 && --decal->ticks <= 0) {
        decal_delete(level, decal);
    }
}

void decal_recalculate(level_t *level, decal_t *decal) {
    if (decal->level_flags & LF_DO_NOT_RECALC) { return; }

    level->version++;
    decal->version++;

    // TODO: animations, etc.
    memcpy(&decal->tex, &decal->tex_base, sizeof(decal->tex));

    // clamp
    if (decal->is_on_side) {
        decal->side.offsets =
            VEC2(
                clamp(decal->side.offsets.x, 0.0f, decal->side.ptr->wall->len),
                clamp(
                    decal->side.offsets.y,
                    0.0f,
                    decal->side.ptr->sector ?
                        decal->side.ptr->sector->ceil.z
                            - decal->side.ptr->sector->floor.z
                    : 0.0f));
    } else {
        // TODO
        /* decal->sector.pos = sector_clamp */
    }
}

vec2s decal_worldpos(const decal_t *decal) {
    if (decal->is_on_side) {
        vertex_t *vertices[2];
        side_get_vertices(decal->side.ptr, vertices);
        return
            glms_vec2_lerp(
                vertices[0]->pos,
                vertices[1]->pos,
                decal->side.offsets.x / decal->side.ptr->wall->len);
    } else {
        return decal->sector.pos;
    }
}

static void decal_detach(level_t *level, decal_t *decal) {
    if (decal->is_on_side) {
        if (decal->side.ptr) {
            decal->side.ptr->n_decals--;
            llist_remove(node, &decal->side.ptr->decals, decal);
            side_recalculate(level, decal->side.ptr);
        }
    } else {
        if (decal->sector.ptr) {
            decal->sector.ptr->n_decals--;
            llist_remove(node, &decal->sector.ptr->decals, decal);
            sector_recalculate(level, decal->sector.ptr);
        }
    }
}

void decal_set_side(
    level_t *level, decal_t *decal, side_t *side) {
    if (decal->is_on_side && decal->side.ptr == side) { return; }

    decal_detach(level, decal);

    decal->side.ptr = side;
    decal->side.ptr->n_decals++;
    llist_prepend(node, &side->decals, decal);
    decal->is_on_side = true;
    side_recalculate(level, decal->side.ptr);
}

void decal_set_sector(
    level_t *level, decal_t *decal, sector_t *sector, plane_type plane) {
    if (!decal->is_on_side
        && decal->sector.ptr == sector
        && decal->sector.plane == plane) {
        return;
    }

    decal_detach(level, decal);

    decal->sector.ptr = sector;
    decal->sector.ptr->n_decals++;
    llist_prepend(node, &sector->decals, decal);
    decal->sector.plane = plane;
    decal->is_on_side = false;
    sector_recalculate(level, decal->sector.ptr);
}

/* static void switch_button_interact( */
/*     level_t *level, */
/*     decal_t *decal, */
/*     object_t *object) { */
/*     if (!decal->tag) { */
/*         WARN( */
/*             "switch decal %d has no tag", */
/*             lptr_to_index(level, LPTR_FROM(decal))); */
/*         return; */
/*     } */

/*     const i16 val = */
/*         l_set_tag_val(level, decal->tag, 1 - l_get_tag_val(level, decal->tag)); */

/*     nameindex_change_suffix(&decal->tex, val ? 'N' : 'F'); */
/*     atlas_update_nameindex(&state->textures, &decal->tex); */
/* } */

/* static void dt_switch_interact( */
/*     level_t *level, */
/*     decal_t *decal, */
/*     object_t *object) { */
/*     switch_button_interact(level, decal, object); */
/* } */

/* static void dt_button_interact( */
/*     level_t *level, */
/*     decal_t *decal, */
/*     object_t *object) { */
/*     switch_button_interact(level, decal, object); */
/*     // TODO: TOGGLE TIME (via funcdata ?) */
/* } */

decal_type_t DECAL_TYPE_INFO[DT_COUNT] = {
    [DT_PLACEHOLDER] = {
        .flags = 0,
        .interact = NULL
    },
    [DT_SWITCH] = {
        .flags = 0,
        /* .interact = dt_switch_interact */
    },
    [DT_BUTTON] = {
        .flags = 0,
        /* .interact = dt_button_interact */
    },
    [DT_RIFT] = {
        .flags = 0
            | DTF_PORTAL,
        .interact = NULL
    }
};
