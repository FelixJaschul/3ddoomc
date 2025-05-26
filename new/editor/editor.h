#pragma once

#include "../defs.h"

typedef struct editor {
    char levelpath[256];
    bool is_active;
    bool show_grid;
    bool show_axes;
    bool show_bounds;
} editor_t; 