#pragma once

#include "defs.h"

sectmat_t *sectmat_new(level_t *level);

void sectmat_delete(level_t *level, sectmat_t *sectmat);

void sectmat_recalculate(level_t *level, sectmat_t *sectmat);

// property copy, everything is updated except "name"
void sectmat_copy_props(sectmat_t *dst, const sectmat_t *src);
