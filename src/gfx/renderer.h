#pragma once

#include "gfx/atlas.h"
#include "gfx/sokol.h"
#include "gfx/dynbuf.h"
#include "gfx/renderer_types.h"
#include "defs.h"

#define RENDERER_PIXELFORMAT_COLOR SG_PIXELFORMAT_RGBA8
#define RENDERER_PIXELFORMAT_INFO SG_PIXELFORMAT_RGBA32F

#define RENDERER_MAX_SECTORS RENDERER_DATA_IMG_LENGTH
#define RENDERER_MAX_OBJECTS RENDERER_DATA_IMG_LENGTH
#define RENDERER_MAX_SIDES RENDERER_DATA_IMG_LENGTH
#define RENDERER_MAX_DECALS RENDERER_DATA_IMG_LENGTH

#define LEVEL_WIREFRAME_IBUF_SIZE (LEVEL_IBUF_SIZE)

typedef struct sprite_instance {
    vec3s offset;
    vec2s size;
    vec4s color;
    f32 id;
    f32 index;
    f32 flags;
} sprite_instance_t;

typedef struct sector_render_portal {
    sector_t *sector;
    side_t *side;
    void *indices;
    aabb_t scissor;
} sector_render_portal_t;

typedef struct sector_render {
    int index;

    sector_t *sector;

    // renderer version on construction
    int r_version;
    int version;

    // pointers into renderer_t buffer
    void *vertices, *indices;

    // counts
    usize n_vertices, n_indices;

    DYNLIST(sector_render_portal_t) portals;
} sector_render_t;

typedef struct side_render {
    int index;

    side_t *side;

    // renderer version on construction
    int r_version;
    int version;
} side_render_t;

typedef struct decal_render {
    int index;

    decal_t *decal;

    // renderer version on construction
    int r_version;
    int version;

    // pointers into renderer_t buffer
    void *indices, *vertices;
} decal_render_t;

typedef struct sprite_render {
    int index;

    atlas_lookup_t lookup;

    bool is_object;
    union {
        struct {
            object_t *ptr;
            int r_version;
            int version;
        } object;

        struct {
            particle_type_index type;
        } particle;
    };
} sprite_render_t;

typedef struct renderer_data_array {
    int render_size;

    void *renders;

    // pointer into image data
    void *data;

    // vertices + indices
    BITMAP_DECL(all_bits, 2048);
    BITMAP_DECL(frame_bits, 2048);
} renderer_data_array_t;

typedef struct renderer {
    level_t *level;

    mat4s view, proj, view_proj;
    aabbf_t frustum_box;

    struct {
        vec3s pos;
        f32 pitch, yaw;
    } cam;

    struct {
        vec4s *data;
        ivec2s size;
        u64 frame;
    } frame_info;

    sg_image data_image;

    // raw data_image data
    struct {
        vec4s buf
            [T_COUNT]
            [RENDERER_DATA_IMG_LENGTH]
            [RENDERER_DATA_IMG_WIDTH_VEC4S];
        bool dirty;
    } data;

    union {
        struct {
            renderer_data_array_t
                sector_data,
                side_data,
                sprite_data,
                decal_data;
        };

        struct {
            renderer_data_array_t data_arrays[4];
        };
    };
    
    sg_pipeline pipeline_level, pipeline_portal, pipeline_sprite;
    sg_shader shader_level, shader_sprite;

    bool level_dirty;
    sg_buffer level_vbuf, level_ibuf;

    sg_buffer sprite_vbuf, sprite_ibuf, sprite_instbuf;

    int frame_sides;

    struct {
        sprite_instance_t *ptr;
        usize size;
    } instance_data;

    struct {
        sprite_render_t *ptrs[PARTICLE_TYPE_COUNT];
        int indices[PARTICLE_TYPE_COUNT];
    } particle_data;

    int n_sprites;

    dynbuf_t db_vertices, db_indices;

    // arbitary, when bumped will invalidate all renderer data (force re-mesh)
    int version;
    bool debug_ui;
} renderer_t;

void renderer_init(renderer_t*);

void renderer_set_level(renderer_t*, level_t*);

void renderer_destroy_for_level(renderer_t*);

void renderer_destroy(renderer_t*);

void renderer_render(renderer_t*);

vec4s renderer_info_at(renderer_t*, ivec2s pos);
