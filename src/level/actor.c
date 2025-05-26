#include "level/actor.h"
#include "level/level_defs.h"
#include "level/level.h"
#include "level/lptr.h"
#include "reload.h"
#include "state.h"

static actor_t **lptr_pactor(level_t *level, lptr_t ptr) {
    switch (LPTR_TYPE(ptr)) {
    case T_SECTOR: return &LPTR_SECTOR(level, ptr)->actor;
    case T_OBJECT: return &LPTR_OBJECT(level, ptr)->actor;
    case T_SIDE:   return &LPTR_SIDE(level, ptr)->actor;
    default: ASSERT(false);
    }
}

void actor_add(level_t *level, actor_t actor, lptr_t ptr) {
    // TODO: allow camera actors to bypass, or something?
    /* ASSERT(state->mode == GAMEMODE_GAME, "cannot add actors in edit mode"); */

    actor_t *p = calloc(1, sizeof(actor_t));
    *p = actor;

    if (!LPTR_IS_NULL(ptr)) {
        if (LPTR_TYPE(ptr) & T_HAS_ACTOR) {
            actor_t **storage = lptr_pactor(level, ptr);
            *storage = p;
            p->storage = storage;
        } else {
            WARN("adding actor for lptr type with no actor??");
            ASSERT(false);
        }
    }

    // APPEND (not prepend!) to list so that if this actor was added by another,
    // it will run after and not before
    dlist_append(listnode, &level->actors, p);

#ifdef RELOADABLE
    if (g_reload_host) {
        g_reload_host->reg_fn((void**) &p->act);
    }
#endif // ifdef RELOADABLE
}
