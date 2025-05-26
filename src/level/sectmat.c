#include "level/sectmat.h"
#include "editor/editor.h"
#include "level/level_defs.h"
#include "level/level.h"
#include "level/lptr.h"
#include "level/sector.h"
#include "state.h"

sectmat_t *sectmat_new(level_t *level) {
    sectmat_t *s = level_alloc(level, level->sectmats);
    LOG("new sectmat with index %d", s->index);

    s->name = resource_next_valid("SECTMAT");

    for (int i = 0; i < 2; i++) {
        s->mats[i].tex = AS_RESOURCE(TEXTURE_NOTEX);
        s->mats[i].offset = IVEC2(0);
    }

    sectmat_recalculate(level, s);
    return s;
}

void sectmat_delete(level_t *level, sectmat_t *s) {
#ifdef MAPEDITOR
    if (state->mode == GAMEMODE_EDITOR) {
        editor_close_for_ptr(state->editor, LPTR_FROM(s));
    }
#endif //ifdef MAPEDITOR

    // cannot remove NOMAT
    ASSERT(lptr_to_index(level, LPTR_FROM(s)) != SIDEMAT_NOMAT);

    // update all sects with this material
    level_dynlist_each(level->sectors, it) {
        sector_t *sect = *it.el;

        if (sect->mat == s) {
            sect->mat = level->sectmats[SIDEMAT_NOMAT];
            sector_recalculate(level, sect);
        }
    }

    level_free(level, level->sectmats, s);
}

void sectmat_recalculate(level_t *level, sectmat_t *sectmat) {
    level_dynlist_each(level->sectors, it) {
        if ((*it.el)->mat == sectmat) {
            sector_recalculate(level, *it.el);
        }
    }
}

void sectmat_copy_props(sectmat_t *dst, const sectmat_t *src) {
    const resource_t name = dst->name;
    const u16 index = dst->index;
    *dst = *src;
    dst->name = name;
    dst->index = index;
}
