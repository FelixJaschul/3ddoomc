#include "gfx/palette.h"
#include "util/assert.h"
#include "util/color.h"
#include "util/file.h"
#include "util/str.h"

#define PALETTE_IMAGE_SIZE 32

// hash the ABGR components of an ABGR color
ALWAYS_INLINE u32 abgr_hash(u32 abgr) {
    u32 seed = 0x12345;
    seed ^= ((abgr >>  0) & 0xFF) + 0x9E3779B9u + (seed << 6) + (seed >> 2);
    seed ^= ((abgr >>  8) & 0xFF) + 0x9E3779B9u + (seed << 6) + (seed >> 2);
    seed ^= ((abgr >> 16) & 0xFF) + 0x9E3779B9u + (seed << 6) + (seed >> 2);
    return seed;
}

ALWAYS_INLINE f32 abgr_dist(u32 x, u32 y) {
    // approximate l/u/v distance
    // see https://www.compuphase.com/cmetric.htm
    const int
        ravg = (((x >> 0) & 0xFF) + ((y >> 0) & 0xFF)) / 2,
        b = ((int) ((x >> 16) & 0xFF)) - ((int) ((y >> 16) & 0xFF)),
        g = ((int) ((x >>  8) & 0xFF)) - ((int) ((y >>  8) & 0xFF)),
        r = ((int) ((x >>  0) & 0xFF)) - ((int) ((y >>  0) & 0xFF));
    return sqrtf(
        (((512 + ravg) * r * r) >> 8)
            + (4 * g * g)
            + (((767 - ravg) * b * b) >> 8));
}

void palette_init(palette_t *pal) {
    pal->image =
        sg_make_image(
            &(sg_image_desc) {
                .type = SG_IMAGETYPE_3D,
                .width = PALETTE_IMAGE_SIZE,
                .height = PALETTE_IMAGE_SIZE,
                .num_slices = PALETTE_IMAGE_SIZE,
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST,
                .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_w = SG_WRAP_CLAMP_TO_EDGE,
                .usage = SG_USAGE_DYNAMIC
            });
}

void palette_destroy(palette_t *pal) {
    sg_destroy_image(pal->image);
}

static void calc_hashes(palette_t *pal) {
    memset(pal->hashed, 0, sizeof(pal->hashed));

    for (int i = 0; i < PALETTE_SIZE; i++) {
        int pos = abgr_hash(pal->colors[i]) % PALETTE_SIZE, count = 0;

        // round-robin
        while (pal->hashed[pos] != 0 && count < PALETTE_SIZE) {
            pos = (pos + 1) % PALETTE_SIZE;
            count++;
        }

        ASSERT(count < PALETTE_SIZE, "failed to hash color (?)");

        // store index in upper 8 bits and BGR in lower 24 bits
        pal->hashed[pos] =
            (((u32) i) << 24u) | (pal->colors[i] & 0xFFFFFF);
    }
}

void palette_load_gpl(palette_t *pal, const char *path) {
    char *data;
    usize len;
    ASSERT(
        !file_read_str(path, &data, &len),
        "failed to read palette file %s", path);

    int i = 0;

    const char *next = data;
    char line[1024];
    while (*next && (next = strline(next, line, sizeof(line)))) {
        const char *p = line;

        // ignore opening line
        if (!strcmp(line, "GIMP Palette")) {
            continue;
        }

        // skip all whitespace, continue on empty or comment lines
        while (isspace(*p)) { p++; }

        if (!*p || *p == '#') {
            continue;
        }

        int ignore[3];
        u32 rgb;
        ASSERT(
            sscanf(
                p,
                "%d %d %d %06x",
                &ignore[0], &ignore[1], &ignore[2], &rgb) == 4,
            "malformed palette file line %s", line);


        pal->colors[i++] =
            0xFF000000
            | ((rgb & 0xFF) << 16)
            | (rgb & 0x00FF00)
            | ((rgb >> 16) & 0xFF);
    }

    if (i != 256) {
        WARN("palette %s only has %d colors, should have 256", path, i);
    }

    free(data);
    calc_hashes(pal);

    // calculate image
    const usize n_bytes =
        PALETTE_IMAGE_SIZE
            * PALETTE_IMAGE_SIZE
            * PALETTE_IMAGE_SIZE
            * sizeof(u32);
    u32 *pixels = malloc(n_bytes);

    const f32 step = 255.0f / (PALETTE_IMAGE_SIZE - 1);
    for (int b = 0; b < PALETTE_IMAGE_SIZE; b++) {
        for (int g = 0; g < PALETTE_IMAGE_SIZE; g++) {
            for (int r = 0; r < PALETTE_IMAGE_SIZE; r++) {
                const u32 abgr =
                    0xFF000000
                    | ((((u32) (b * step)) & 0xFF) << 16)
                    | ((((u32) (g * step)) & 0xFF) <<  8)
                    | ((((u32) (r * step)) & 0xFF) <<  0);
                const u8 col = palette_nearest(pal, abgr);
                const u32 abgr_p = pal->colors[col];
                pixels[
                    (b * PALETTE_IMAGE_SIZE * PALETTE_IMAGE_SIZE)
                    + (g * PALETTE_IMAGE_SIZE)
                    + r] = abgr_p;
            }
        }
    }
    free(pixels);

    sg_update_image(
        pal->image,
        &(sg_image_data) {
            .subimage[0][0] = { .ptr = pixels, .size = n_bytes }
        });
}

u8 palette_find(const palette_t *pal, u32 color) {
    int pos = abgr_hash(color) % PALETTE_SIZE, count = 0;

    while (
        count < PALETTE_SIZE
        && (color & 0xFFFFFF) != (pal->hashed[pos] & 0xFFFFFF)) {
        pos = (pos + 1) % PALETTE_SIZE;
        count++;
    }

    if (count == PALETTE_SIZE) {
        WARN("attempt to find non-palette color %08x", color);
        return 0;
    }

    // upper 8 bits store index in palette
    return (u8) (pal->hashed[pos] >> 24);
}

u8 palette_nearest(const palette_t *pal, u32 color) {
    f32 dist = 1e10;
    u8 nearest = PALETTE_SIZE - 1;

    for (int i = 0; i < PALETTE_SIZE; i++) {
        if ((pal->colors[i] & 0xFFFFFF) == (color & 0xFFFFFF)) {
            return i;
        }

        const f32 d = abgr_dist(color, pal->colors[i]);
        if (d < dist) {
            dist = d;
            nearest = i;
        }
    }

    return nearest;
}
