#pragma once

#include "util/math.h"
#include "util/types.h"
#include "gfx/sokol.h"
#include "config.h"

#define PALETTE_SIZE 256

typedef struct palette {
    // 3D image map of RGB -> palette
    sg_image image;
    u32 colors[PALETTE_SIZE];
    u32 hashed[PALETTE_SIZE];
} palette_t;

// TODO: load palette from binary data

void palette_init(palette_t *pal);

void palette_destroy(palette_t *pal);

// load a palette from a GIMP .gpl format file
void palette_load_gpl(palette_t *pal, const char *path);

// find palette color for abgr color, returns palette[0] if not found
u8 palette_find(const palette_t *pal, u32 color);

// find nearest palette color for abgr color
u8 palette_nearest(const palette_t *pal, u32 color);
