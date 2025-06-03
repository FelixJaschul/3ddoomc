#pragma once

#include "defs.h"
#include "gfx/sokol.h"
#include "util/math.h"
#include "util/types.h"
#include "util/aabb.h"
#include "util/map.h"
#include "util/dynlist.h"
#include "config.h"

#define ATLAS_SIZE 4096
#define ATLAS_DEPTH 1

#define ATLAS_MAX_TEXTURES 2048

typedef struct atlas_entry atlas_entry_t;

typedef struct atlas {
    sg_image image;

    sg_image coords_image, layer_image;

    vec4s *coords_data;
    u32 *layer_data;
    int next_entry_id;

    // boxes for each layer
    DYNLIST(atlas_entry_t*) boxes[ATLAS_DEPTH];

    // data for each layer
    u8 *data[ATLAS_DEPTH];

    // area coverage of each layer
    int usage[ATLAS_DEPTH];

    // const char* -> atlas_entry_t*
    map_t lookup;

    // if true, atlas is uploaded next atlas_update
    bool dirty;

#ifdef MAPEDITOR
    sg_image layer_images[ATLAS_DEPTH];

    // names of ALL loaded textures
    DYNLIST(const char*) names;
#endif // ifdef MAPEDITOR
} atlas_t;

void atlas_init(atlas_t*);
void atlas_destroy(atlas_t*);
void atlas_update(atlas_t*);

typedef struct atlas_lookup {
    int id;
    int layer;
    aabbf_t box_uv;
    aabb_t box_px;
} atlas_lookup_t;

bool atlas_lookup(atlas_t*, resource_t, atlas_lookup_t*);

void atlas_load_all(atlas_t*);

void atlas_clear(atlas_t*);
