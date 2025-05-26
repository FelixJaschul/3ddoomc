#include "gfx/atlas.h"
#include "util/file.h"
#include "util/image.h"
#include "util/resource.h"

typedef struct atlas_entry {
    int id;
    aabb_t box;
    vec2s uv;
    int layer;
    bool is_subimage;
} atlas_entry_t;

static atlas_entry_t *get_colliding(
    atlas_t *atlas, int layer, const aabb_t *box) {
    dynlist_each(atlas->boxes[layer], it) {
        if (!(*it.el)->is_subimage
            && aabb_collides((*it.el)->box, *box)) {
            return *it.el;
        }
    }

    return NULL;
}

static bool insert(
    atlas_t *atlas,
    const char *name,
    const u8 *data,
    ivec2s size,
    aabbf_t *box_out,
    int *layer_out) {
    // find layer with lowest coverage
    int layer = 0, min_usage = INT_MAX;
    for (int i = 0; i < ATLAS_DEPTH; i++) {
        if (atlas->usage[i] < min_usage) {
            layer = i;
            min_usage = atlas->usage[i];
        }
    }

    bool next_x = false, next_y = false;
    ivec2s offset = IVEC2(0);

    while (
        offset.x + size.x <= ATLAS_SIZE
           && offset.y + size.y <= ATLAS_SIZE) {
        const aabb_t box = AABB_PS(offset, size);
        atlas_entry_t *colliding = get_colliding(atlas, layer, &box);

        if (!colliding) {
            // found a spot
            break;
        } else {
            // move to the closest side of the colliding box
            if (next_x || (size.x < size.y && !next_y)) {
                offset.x = colliding->box.max.x + 1;
                next_x = false;
            } else {
                offset.y = colliding->box.max.y + 1;
                next_y = false;
            }

            const bool
                of_x = offset.x + size.x > ATLAS_SIZE,
                of_y = offset.y + size.y > ATLAS_SIZE;

            // double overflow means out of space
            ASSERT(!of_x || !of_y);

            // on overflow, reset up to the next row
            // use next_x/y to ensure that we advance on the opposite axis
            // next time
            if (of_x) {
                offset.x = 0;
                next_y = true;
            }

            if (of_y) {
                offset.y = 0;
                next_x = true;
            }
        }
    }

    LOG("allocated %s on atlas at %" PRIv2i, name, FMTv2i(offset));

    // allocate data for layer if it doesn't exist
    if (!atlas->data[layer]) {
        LOG("allocating new atlas layer %d", layer);
        atlas->data[layer] = calloc(1, ATLAS_SIZE * ATLAS_SIZE * 4);
    }

    // copy data
    for (int y = 0; y < size.y; y++) {
        memcpy(
            &atlas->data[layer][((y + offset.y) * ATLAS_SIZE + offset.x) * 4],
            &data[(y * size.x + 0) * 4],
            size.x * 4);
    }

    atlas->dirty = true;

    const vec2s unit = glms_vec2_divs(VEC2(1.0f), ATLAS_SIZE);

    // create entry, insert into lookup table
    atlas_entry_t *entry = calloc(1, sizeof(atlas_entry_t));
    *entry = (atlas_entry_t) {
        .id = atlas->next_entry_id++,
        .box = AABB_PS(offset, size),
        .uv = glms_vec2_mul(IVEC_TO_V(offset), unit),
        .layer = layer,
        .is_subimage = false,
    };

    const aabbf_t box =
        AABBF_PS(
            glms_vec2_mul(IVEC_TO_V(offset), unit),
            glms_vec2_mul(IVEC_TO_V(size), unit));

    atlas->coords_data[entry->id] = (vec4s) {
        .r = box.min.x,
        .g = box.min.y,
        .b = box.max.x,
        .a = box.max.y,
    };

    atlas->layer_data[entry->id] = (u32) entry->layer;

    *dynlist_push(atlas->boxes[layer]) = entry;

    char upper_name[256];
    snprintf(upper_name, sizeof(upper_name), "%s", name);
    strtoupper(upper_name);

    map_insert(&atlas->lookup, upper_name, entry);

    if (box_out) {
        *box_out = box;
    }

    if (layer_out) {
        *layer_out = layer;
    }

#ifdef MAPEDITOR
    *dynlist_push(atlas->names) = strdup(name);
#endif // ifdef MAPEDITOR
    return true;
}

static bool insert_from_path(
    atlas_t *atlas,
    const char *name,
    const char *path,
    aabbf_t *box_out,
    int *layer_out) {
    int res;

    u8 *data;
    ivec2s size;
    if ((res = load_image_rgba(path, &data, &size))) {
        WARN("image load failure: %d", res);
        return false;
    }

    return insert(atlas, name, data, size, box_out, layer_out);
}

void atlas_init(atlas_t *atlas) {
    *atlas = (atlas_t) { 0 };
    atlas->image =
        sg_make_image(
            &(sg_image_desc) {
                .type = SG_IMAGETYPE_ARRAY,
                .width = ATLAS_SIZE,
                .height = ATLAS_SIZE,
                .num_slices = ATLAS_DEPTH,
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST,
                .usage = SG_USAGE_DYNAMIC
            });

    atlas->coords_image =
        sg_make_image(
            &(sg_image_desc) {
                .type = SG_IMAGETYPE_2D,
                .width = ATLAS_MAX_TEXTURES,
                .height = 1,
                .pixel_format = SG_PIXELFORMAT_RGBA32F,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST,
                .usage = SG_USAGE_DYNAMIC
            });

    atlas->layer_image =
        sg_make_image(
            &(sg_image_desc) {
                .type = SG_IMAGETYPE_2D,
                .width = ATLAS_MAX_TEXTURES,
                .height = 1,
                .pixel_format = SG_PIXELFORMAT_R32F,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST,
                .usage = SG_USAGE_DYNAMIC
            });

    atlas->coords_data = calloc(1, ATLAS_MAX_TEXTURES * sizeof(vec4s));
    atlas->layer_data = calloc(1, ATLAS_MAX_TEXTURES * sizeof(u32));

#ifdef MAPEDITOR
    for (int i = 0; i < ATLAS_DEPTH; i++) {
        atlas->layer_images[i] =
            sg_make_image(
                &(sg_image_desc) {
                    .type = SG_IMAGETYPE_2D,
                    .width = ATLAS_SIZE,
                    .height = ATLAS_SIZE,
                    .pixel_format = SG_PIXELFORMAT_RGBA8,
                    .min_filter = SG_FILTER_NEAREST,
                    .mag_filter = SG_FILTER_NEAREST,
                    .usage = SG_USAGE_DYNAMIC
                });
    }
#endif // ifdef MAPEDITOR

    map_init(
        &atlas->lookup,
        map_hash_str,
        NULL,
        NULL,
        map_dup_str,
        map_cmp_str,
        map_default_free,
        NULL,
        NULL);

    // create NOTEX
    atlas_clear(atlas);
}

void atlas_destroy(atlas_t *atlas) {
#ifdef MAPEDITOR
    dynlist_each(atlas->names, it) { free((void*) *it.el); }
    dynlist_free(atlas->names);

    for (int i = 0; i < ATLAS_DEPTH; i++) {
        sg_destroy_image(atlas->layer_images[i]);
    }
#endif // ifdef MAPEDITOR

    sg_destroy_image(atlas->image);
    sg_destroy_image(atlas->coords_image);
    sg_destroy_image(atlas->layer_image);

    free(atlas->coords_data);
    free(atlas->layer_data);

    for (int i = 0; i < ATLAS_DEPTH; i++) {
        if (atlas->data[i]) { free(atlas->data[i]); }

        dynlist_each(atlas->boxes[i], it) {
            free(*it.el);
        }
        dynlist_free(atlas->boxes[i]);
    }

    map_destroy(&atlas->lookup);
}

void atlas_update(atlas_t *atlas) {
    if (!atlas->dirty) { return; }
    atlas->dirty = false;

    sg_image_data image_data;
    for (int i = 0; i < ATLAS_DEPTH; i++) {
        if (atlas->data[i]) {
            image_data.subimage[i][0] =
                (sg_range) {
                    .ptr = atlas->data[i],
                    .size = ATLAS_SIZE * ATLAS_SIZE * 4
                };

#ifdef MAPEDITOR
            sg_update_image(
                atlas->layer_images[i],
                &(sg_image_data) {
                    .subimage[0][0] = {
                        .ptr = atlas->data[i],
                        .size = ATLAS_SIZE * ATLAS_SIZE * 4
                    }
                });
#endif // ifdef MAPEDITOR
        }
    }

    sg_update_image(atlas->image, &image_data);

    sg_update_image(
        atlas->coords_image,
        &(sg_image_data) {
            .subimage[0][0] = {
                .ptr = atlas->coords_data,
                .size = ATLAS_MAX_TEXTURES * sizeof(vec4s)
            }
        });

    sg_update_image(
        atlas->layer_image,
        &(sg_image_data) {
            .subimage[0][0] = {
                .ptr = atlas->layer_data,
                .size = ATLAS_MAX_TEXTURES * sizeof(u32)
            }
        });
}

void atlas_load_all(atlas_t *atlas) {
    DYNLIST(char*) images = NULL;
    ASSERT(
        !file_find_with_ext(
            SPRITES_BASE_DIR,
            "png",
            &images));

    dynlist_each(images, it) {
        char buf[1024];
        strncpy(buf, *it.el, sizeof(buf));
        char *name;
        file_spilt(buf, NULL, &name, NULL);

        insert_from_path(atlas, name, *it.el, NULL, NULL);
        free(*it.el);
    }

    dynlist_free(images);
}

bool atlas_lookup(atlas_t *atlas, resource_t resource, atlas_lookup_t *out) {
    strtoupper(resource.name);

    atlas_entry_t **slot =
        map_find(atlas_entry_t*, &atlas->lookup, resource.name);

    if (!slot) {
        // attempt to load from file
        char path[1024];
        if (resource_find_file(
                resource, SPRITES_BASE_DIR, "png", path, sizeof(path))) {
            if (!insert_from_path(
                    atlas,
                    resource.name,
                    path,
                    NULL, NULL)) {
                WARN("insert failure for %s @ %s", resource.name, path);
                atlas_lookup(atlas, AS_RESOURCE(TEXTURE_NOTEX), out);
                return false;
            }

            // lookup again
            slot = map_find(atlas_entry_t*, &atlas->lookup, resource.name);
            ASSERT(slot);
        } else {
            WARN("could not find atlas entry \"%s\" + no file", resource.name);
            atlas_lookup(atlas, AS_RESOURCE(TEXTURE_NOTEX), out);
            return false;
        }
    }

    const vec2s unit = glms_vec2_divs(VEC2(1.0f), ATLAS_SIZE);
    atlas_entry_t *entry = *slot;
    *out = (atlas_lookup_t) {
        .id = entry->id,
        .layer = entry->layer,
        .box_uv =
            AABBF_PS(
                entry->uv,
                glms_vec2_mul(IVEC_TO_V(aabb_size(entry->box)), unit)),
        .box_px = entry->box,
    };

    return true;
}

void atlas_clear(atlas_t *atlas) {
    for (int i = 0; i < ATLAS_DEPTH; i++) {
        dynlist_free(atlas->boxes[i]);
    }

    map_clear(&atlas->lookup);
    dynlist_free(atlas->names);

    // create NOTEX
    u32 data[16 * 16];
    const u32 c0 = 0xFFFF00FF, c1 = 0xFF550055;

    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            data[(x * 16) + y] = (x / 8) % 2 == (y / 8) % 2 ? c0 : c1;
        }
    }

    insert(
        atlas, TEXTURE_NOTEX, (const u8*) data, IVEC2(16, 16), NULL, NULL);
}
