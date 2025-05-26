#include "level/tag.h"
#include "level/level_defs.h"
#include "level/lptr.h"

int *lptr_ptag(level_t *level, lptr_t ptr) {
    switch (LPTR_TYPE(ptr)) {
    case T_SIDE: return &LPTR_SIDE(level, ptr)->tag;
    case T_SECTOR: return &LPTR_SECTOR(level, ptr)->tag;
    case T_DECAL: return &LPTR_DECAL(level, ptr)->tag;
    default: ASSERT(false);
    }
}

int tag_suggest(level_t *level) {
    // always start from 1 (first valid tag)
    for (int i = 1; i < TAG_MAX; i++) {
        if (dynlist_size(level->tag_lists[i]) == 0) {
            return i;
        }
    }

    return TAG_NONE;
}

int tag_set_value(level_t *level, int tag, int val) {
    level->tag_values[tag] = val;

    // TODO: maybe don't do this in l_set_tag? kind of a weird spot, maybe
    // these should be done via some sort of flag on the tagged objects
    // trigger tagchange events
    dynlist_each(level->tag_lists[tag], it) {
        if (LPTR_TYPE(*it.el) == T_SECTOR) {
            sector_t *s = LPTR_SECTOR(level, *it.el);
            const sector_func_type_t *sft =
                &SECTOR_FUNC_TYPE[s->func_type];
            if (sft->tag_change) { sft->tag_change(level, s); }
        }
    }

    return val;
}

int tag_get_value(level_t *level, int tag) {
    return level->tag_values[tag];
}

void tag_set(level_t *level, lptr_t ptr, int tag) {
    int *ptag = lptr_ptag(level, ptr);

    if (tag == *ptag) {
        // nothing to update
        return;
    }

    // remove from old tag list
    if (*ptag != TAG_NONE) {
        bool found = false;
        dynlist_each(level->tag_lists[*ptag], it) {
            if (LPTR_EQ(*it.el, ptr)) {
                dynlist_remove_it(level->tag_lists[*ptag], it);
                found = true;
                break;
            }
        }

        ASSERT(found);
    }

    *ptag = tag;

    if (tag != TAG_NONE) {
        // should not be in existing taglist
        dynlist_each(level->tag_lists[tag], it) {
            ASSERT(!LPTR_EQ(*it.el, ptr));
        }

        *dynlist_push(level->tag_lists[tag]) = ptr;
    }
}
