#include "level/side_texinfo.h"
#include "level/level_defs.h"

void side_texinfo_init(side_texinfo_t *s) {
    memset(s, 0, sizeof(*s));
    s->flags = STF_NONE;
    s->split_bottom = 0.0f;
    s->split_top = 1024.0f;
}
