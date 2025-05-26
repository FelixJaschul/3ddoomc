#pragma once

#include "defs.h"

sidemat_t *sidemat_new(level_t *level);

void sidemat_delete(level_t *level, sidemat_t *sidemat);

void sidemat_recalculate(level_t *level, sidemat_t *sidemat);

// property copy, everything is updated except "name"
void sidemat_copy_props(sidemat_t *dst, const sidemat_t *src);
