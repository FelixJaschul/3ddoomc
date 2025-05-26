#pragma once

#include "util/types.h"
#include "util/math.h"
#include "ext/stb_image.h"

int load_image_rgba(const char *path, u8 **pdata, ivec2s *psize);

#ifdef UTIL_IMPL
int load_image_rgba(const char *path, u8 **pdata, ivec2s *psize) {
    int channels;
    stbi_set_flip_vertically_on_load(true);
    *pdata = stbi_load(path, &psize->x, &psize->y, &channels, 4);
    if (!*pdata) { WARN("stbi failed (%s): %s", path, stbi_failure_reason()); }
    return *pdata ? 0 : 1;
}
#endif // ifdef UTIL_IMPL
