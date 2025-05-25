#include "level/sidemat.h"
#include "editor/editor.h"
#include "level/level_defs.h"
#include "level/level.h"
#include "level/lptr.h"
#include "level/side.h"
#include "level/side_texinfo.h"
#include "state.h"

sidemat_t *sidemat_new(level_t *level) {
    sidemat_t *s = level_alloc(level, level->sidemats);
    LOG("new sidemat with index %d", s->index);

    s->name = resource_next_valid("SIDEMAT");
    for (int i = 0; i < (int) ARRLEN(s->texs); i++) {
        s->texs[i] = AS_RESOURCE(TEXTURE_NOTEX);
    }

    side_texinfo_init(&s->tex_info);

    sidemat_recalculate(level, s);
    return s;
}

void sidemat_delete(level_t *level, sidemat_t *s) {
#ifdef MAPEDITOR
    if (state->mode == GAMEMODE_EDITOR) {
        editor_close_for_ptr(state->editor, LPTR_FROM(s));
    }
#endif //ifdef MAPEDITOR

    // cannot remove NOMAT
    ASSERT(lptr_to_index(level, LPTR_FROM(s)) != SIDEMAT_NOMAT);

    // update all sides with this material
    level_dynlist_each(level->sides, it) {
        side_t *side = *it.el;

        if (side->mat == s) {
            side->mat = level->sidemats[SIDEMAT_NOMAT];
            side_recalculate(level, side);
        }
    }

    level_free(level, level->sidemats, s);
}

void sidemat_recalculate(level_t *level, sidemat_t *sidemat) {
    level_dynlist_each(level->sides, it) {
        if ((*it.el)->mat == sidemat) {
            side_recalculate(level, *it.el);
        }
    }
}

void sidemat_copy_props(sidemat_t *dst, const sidemat_t *src) {
    const resource_t name = dst->name;
    const u16 index = dst->index;
    *dst = *src;
    dst->name = name;
    dst->index = index;
}
