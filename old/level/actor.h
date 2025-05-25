#pragma once

#include "defs.h"

#define T_HAS_ACTOR (T_SECTOR | T_OBJECT | T_SIDE)

// add actor to level (can optionally be associated with some thing "owner")
void actor_add(level_t *level, actor_t actor, lptr_t owner);

