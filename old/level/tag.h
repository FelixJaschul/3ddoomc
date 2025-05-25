#pragma once

#include "defs.h"

// types which can have tags
#define T_HAS_TAG (T_SIDE | T_DECAL | T_SECTOR)

// pointer to lptr_t tag field, crashes if not T_HAS_TAG
int *lptr_ptag(level_t *level, lptr_t ptr);

// suggest a new, unused tag
int tag_suggest(level_t *level);

// find level objects with the specified tag
int tag_find(level_t *level, int tag, lptr_t *ptrs, int n);

// set tag value
int tag_set_value(level_t *level, int tag, int val);

// get tag value
int tag_get_value(level_t *level, int tag);

// sets tag on a taggable pointer (T_SIDE, T_SECTOR, T_DECAL)
void tag_set(level_t *level, lptr_t ptr, int tag);
