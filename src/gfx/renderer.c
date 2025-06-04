#include "gfx/renderer.h"
#include "gfx/atlas.h"
#include "gfx/gfx.h"
#include "gfx/palette.h"
#include "gfx/renderer_types.h"
#include "level/level.h"
#include "level/particle.h"
#include "level/portal.h"
#include "level/sector.h"
#include "level/side.h"
#include "imgui.h"
#include "level/wall.h"
#include "state.h"
#include "util/sort.h"

#define LEVEL_VBUF_SIZE (32 * 1024 * 1024)
#define LEVEL_IBUF_SIZE (16 * 1024 * 1024)
#define SPRITE_INSTBUF_SIZE (8 * 1024 * 1024)

#include "shader/level.glsl.h"
#include "shader/sprite.glsl.h"

typedef struct {
    vec3s pos;
    vec2s uv;
    f32 id;
    f32 index;
    f32 flags;
} render_vertex_t;

typedef struct {
    vec3s pos;
    vec2s uv;
} sprite_vertex_t;

typedef enum {
    PREPARE_OK,
    PREPARE_REMESH,
    PREPARE_DATA_UPDATE
} prepare_result;

typedef void (*data_array_alloc_f)(renderer_t*, void*, void*);

enum {
    DATA_REALLOC_KEPT,
    DATA_REALLOC_NEW,
    DATA_REALLOC_OVERWRITE,
    DATA_REALLOC_FAIL
};

ALWAYS_INLINE int data_array_realloc(
    renderer_t *r,
    renderer_data_array_t *data,
    bool need_new,
    void *prender,
    void *pdata,
    int *pindex) {
    // try to keep things in the same spot
    int index =
        *(void**) prender == NULL ?
            INT_MAX
            : ((int) (*((void**) prender) - data->renders)) / data->render_size;

    if (!need_new
        && index != INT_MAX
        && bitmap_get(data->all_bits, index)) {
        bitmap_set(data->frame_bits, index);

        // keep same data
        *(void**)pdata = data->data + (index * RENDERER_DATA_IMG_WIDTH_BYTES);
        *pindex = index;
        return DATA_REALLOC_KEPT;
    }

    int res = DATA_REALLOC_NEW;

    // data has been deallocated, need to find a new spot
    // try to find somewhere that isn't taken in all_bits
    index = -1;
    do {
        index = bitmap_find(data->frame_bits, 2048, index + 1, false);
    } while (index != INT_MAX && bitmap_get(data->all_bits, index));

    // check if all_bits is full
    if (index == INT_MAX) {
        // find a free spot in frame data and just overwrite in all_bits
        index = bitmap_find(data->frame_bits, 2048, 0, false);

        if (index == INT_MAX) {
            WARN("out of space in data array @ %p", data);
            return DATA_REALLOC_FAIL;
        }

        // overwrite existing
        res = DATA_REALLOC_OVERWRITE;
        ASSERT(!bitmap_get(data->frame_bits, index));
    }

    ASSERT(!bitmap_get(data->frame_bits, index));

    bitmap_set(data->all_bits, index);
    bitmap_set(data->frame_bits, index);

    *(void**)prender = data->renders + (index * data->render_size);
    *(void**)pdata = data->data + (index * RENDERER_DATA_IMG_WIDTH_BYTES);
    *pindex = index;
    return res;
}

static void make_pipelines(renderer_t *r) {
    sg_pipeline_desc level_pip_desc = {
        .shader = r->shader_level,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .buffers[0] = {
                .stride = sizeof(render_vertex_t),
            },
            .attrs = {
                [ATTR_level_vs_a_position] = {
                    .offset = offsetof(render_vertex_t, pos),
                    .format = SG_VERTEXFORMAT_FLOAT3,
                },
                [ATTR_level_vs_a_texcoord0] = {
                    .offset = offsetof(render_vertex_t, uv),
                    .format = SG_VERTEXFORMAT_FLOAT2,
                },
                [ATTR_level_vs_a_id] = {
                    .offset = offsetof(render_vertex_t, id),
                    .format = SG_VERTEXFORMAT_FLOAT,
                },
                [ATTR_level_vs_a_index] = {
                    .offset = offsetof(render_vertex_t, index),
                    .format = SG_VERTEXFORMAT_FLOAT,
                },
                [ATTR_level_vs_a_flags] = {
                    .offset = offsetof(render_vertex_t, flags),
                    .format = SG_VERTEXFORMAT_FLOAT,
                },
            }
        },
        .colors = {
            [0] = {
                .pixel_format = RENDERER_PIXELFORMAT_COLOR,
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                    .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .op_rgb = SG_BLENDOP_ADD,
                    .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
                    .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .op_alpha = SG_BLENDOP_ADD,
                }
            },
            [1] = {
                .pixel_format = RENDERER_PIXELFORMAT_INFO,
                .blend = { .enabled = false }
            }
        },
        .color_count = 2,
        .depth = {
            .write_enabled = true,
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        },
        .stencil = {
            .enabled = true,
            .front = {
                .compare = SG_COMPAREFUNC_GREATER_EQUAL,
                .fail_op = SG_STENCILOP_KEEP,
                .depth_fail_op = SG_STENCILOP_KEEP,
                .pass_op = SG_STENCILOP_KEEP,
            },
            .back = {
                .compare = SG_COMPAREFUNC_NEVER,
                .fail_op = SG_STENCILOP_KEEP,
                .depth_fail_op = SG_STENCILOP_KEEP,
                .pass_op = SG_STENCILOP_KEEP,
            },
            .ref = 0,
            .write_mask = 0xFF,
            .read_mask = 0xFF
        },
        .face_winding = SG_FACEWINDING_CCW,
        .cull_mode = SG_CULLMODE_BACK,
    };

    r->pipeline_level = sg_make_pipeline(&level_pip_desc);

    // portal pipeline draws 1s on test fail (which is always)
    sg_pipeline_desc portal_pip_desc = level_pip_desc;
    portal_pip_desc.colors[0].write_mask = 0;
    portal_pip_desc.colors[0].blend.enabled = false;
    portal_pip_desc.colors[1].write_mask = 0;
    portal_pip_desc.depth.write_enabled = false;
    portal_pip_desc.stencil = (sg_stencil_state) {
        // overwritten by gl* functions
        .enabled = true,
        .front = {
            .compare = SG_COMPAREFUNC_EQUAL,
            .fail_op = SG_STENCILOP_KEEP,
            .depth_fail_op = SG_STENCILOP_KEEP,
            .pass_op = SG_STENCILOP_INCR_CLAMP,
        },
        .back = {
            .compare = SG_COMPAREFUNC_NEVER,
            .fail_op = SG_STENCILOP_KEEP,
            .depth_fail_op = SG_STENCILOP_KEEP,
            .pass_op = SG_STENCILOP_KEEP,
        },
        .ref = 0,
        .write_mask = 0xFF,
        .read_mask = 0xFF
    };

    r->pipeline_portal = sg_make_pipeline(&portal_pip_desc);

    r->pipeline_sprite = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = r->shader_sprite,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .buffers = {
                [0] = {
                    .stride = sizeof(sprite_vertex_t),
                    .step_rate = SG_VERTEXSTEP_PER_VERTEX,
                },
                [1] = {
                    .stride = sizeof(sprite_instance_t),
                    .step_rate = 1,
                    .step_func = SG_VERTEXSTEP_PER_INSTANCE,
                }
            },
            .attrs = {
                [ATTR_sprite_vs_a_position] = {
                    .buffer_index = 0,
                    .offset = offsetof(sprite_vertex_t, pos),
                    .format = SG_VERTEXFORMAT_FLOAT3,
                },
                [ATTR_sprite_vs_a_texcoord0] = {
                    .buffer_index = 0,
                    .offset = offsetof(sprite_vertex_t, uv),
                    .format = SG_VERTEXFORMAT_FLOAT2,
                },
                [ATTR_sprite_vs_a_offset] = {
                    .buffer_index = 1,
                    .offset = offsetof(sprite_instance_t, offset),
                    .format = SG_VERTEXFORMAT_FLOAT3,
                },
                [ATTR_sprite_vs_a_size] = {
                    .buffer_index = 1,
                    .offset = offsetof(sprite_instance_t, size),
                    .format = SG_VERTEXFORMAT_FLOAT2,
                },
                [ATTR_sprite_vs_a_color] = {
                    .buffer_index = 1,
                    .offset = offsetof(sprite_instance_t, color),
                    .format = SG_VERTEXFORMAT_FLOAT4,
                },
                [ATTR_sprite_vs_a_id] = {
                    .buffer_index = 1,
                    .offset = offsetof(sprite_instance_t, id),
                    .format = SG_VERTEXFORMAT_FLOAT,
                },
                [ATTR_sprite_vs_a_index] = {
                    .buffer_index = 1,
                    .offset = offsetof(sprite_instance_t, index),
                    .format = SG_VERTEXFORMAT_FLOAT,
                },
                [ATTR_sprite_vs_a_flags] = {
                    .buffer_index = 1,
                    .offset = offsetof(sprite_instance_t, flags),
                    .format = SG_VERTEXFORMAT_FLOAT,
                },
            }
        },
        .colors = {
            [0] = {
                .pixel_format = RENDERER_PIXELFORMAT_COLOR,
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                    .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .op_rgb = SG_BLENDOP_ADD,
                    .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
                    .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .op_alpha = SG_BLENDOP_ADD,
                }
            },
            [1] = {
                .pixel_format = RENDERER_PIXELFORMAT_INFO,
                .blend = { .enabled = false }
            }
        },
        .color_count = 2,
        .depth = {
            .write_enabled = true,
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        },
        .stencil = {
            .enabled = true,
            .front = {
                .compare = SG_COMPAREFUNC_GREATER_EQUAL,
                .fail_op = SG_STENCILOP_KEEP,
                .depth_fail_op = SG_STENCILOP_KEEP,
                .pass_op = SG_STENCILOP_KEEP,
            },
            .back = {
                .compare = SG_COMPAREFUNC_NEVER,
                .fail_op = SG_STENCILOP_KEEP,
                .depth_fail_op = SG_STENCILOP_KEEP,
                .pass_op = SG_STENCILOP_KEEP,
            },
            .ref = 0,
            .write_mask = 0xFF,
            .read_mask = 0xFF
        },
        .face_winding = SG_FACEWINDING_CCW,
        .cull_mode = SG_CULLMODE_BACK,
    });
}

void renderer_on_shader_reload(sg_shader*, void *userdata) {
    renderer_t *r = userdata;
    sg_destroy_pipeline(r->pipeline_level);
    sg_dealloc_pipeline(r->pipeline_level);
    sg_destroy_pipeline(r->pipeline_portal);
    sg_dealloc_pipeline(r->pipeline_portal);
    sg_destroy_pipeline(r->pipeline_sprite);
    sg_dealloc_pipeline(r->pipeline_sprite);
    make_pipelines(r);
}

void renderer_init(renderer_t *r) {
    *r = (renderer_t) { 0 };
    r->version = 1;

    gfx_load_shader(
        &r->shader_level,
        level_level_shader_desc,
        renderer_on_shader_reload,
        r);
    gfx_load_shader(
        &r->shader_sprite,
        sprite_sprite_shader_desc,
        renderer_on_shader_reload,
        r);
    make_pipelines(r);

    dynbuf_init(
        &r->db_indices,
        malloc(LEVEL_IBUF_SIZE),
        LEVEL_IBUF_SIZE,
        DYNBUF_OWNS_MEMORY);

    dynbuf_init(
        &r->db_vertices,
        malloc(LEVEL_VBUF_SIZE),
        LEVEL_VBUF_SIZE,
        DYNBUF_OWNS_MEMORY);

    r->level_ibuf =
        sg_make_buffer(
            &(sg_buffer_desc) {
                .type = SG_BUFFERTYPE_INDEXBUFFER,
                .size = LEVEL_IBUF_SIZE,
                .usage = SG_USAGE_STREAM
            });

    r->level_vbuf =
        sg_make_buffer(
            &(sg_buffer_desc) {
                .type = SG_BUFFERTYPE_VERTEXBUFFER,
                .size = LEVEL_VBUF_SIZE,
                .usage = SG_USAGE_STREAM
            });

    static const u16 obj_indices[6] = { 0, 1, 2, 2, 3, 0 };

    static const sprite_vertex_t obj_vertices[4] = {
        [0] = {
            .pos = VEC3(0, 0, 0),
            .uv = VEC2(0, 0),
        },
        [1] = {
            .pos = VEC3(1, 0, 0),
            .uv = VEC2(1, 0),
        },
        [2] = {
            .pos = VEC3(1, 0, 1),
            .uv = VEC2(1, 1),
        },
        [3] = {
            .pos = VEC3(0, 0, 1),
            .uv = VEC2(0, 1),
        },
    };

    r->sprite_ibuf =
        sg_make_buffer(
            &(sg_buffer_desc) {
                .type = SG_BUFFERTYPE_INDEXBUFFER,
                .data = SG_RANGE(obj_indices)
            });

    r->sprite_vbuf =
        sg_make_buffer(
            &(sg_buffer_desc) {
                .type = SG_BUFFERTYPE_VERTEXBUFFER,
                .data = SG_RANGE(obj_vertices)
            });

    r->sprite_instbuf =
        sg_make_buffer(
            &(sg_buffer_desc) {
                .type = SG_BUFFERTYPE_VERTEXBUFFER,
                .usage = SG_USAGE_STREAM,
                .size = SPRITE_INSTBUF_SIZE,
            });

    r->instance_data.ptr = malloc(SPRITE_INSTBUF_SIZE);
    r->instance_data.size = SPRITE_INSTBUF_SIZE;

    r->data_image =
        sg_make_image(
            &(sg_image_desc) {
                .type = SG_IMAGETYPE_ARRAY,
                .width = RENDERER_DATA_IMG_WIDTH_VEC4S,
                .height = RENDERER_DATA_IMG_LENGTH,
                .num_slices = T_COUNT,
                .pixel_format = SG_PIXELFORMAT_RGBA32F,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST,
                .usage = SG_USAGE_STREAM
            });

    const int render_sizes[4] = {
        sizeof(sector_render_t),
        sizeof(side_render_t),
        sizeof(sprite_render_t),
        sizeof(decal_render_t),
    };

    const int render_indices[4] = {
        T_SECTOR_INDEX,
        T_SIDE_INDEX,
        T_OBJECT_INDEX,
        T_DECAL_INDEX,
    };

    for (int i = 0; i < 4; i++) {
        renderer_data_array_t *data = &r->data_arrays[i];
        *data = (renderer_data_array_t) {
            .render_size = render_sizes[i],
            .renders = malloc(2048 * render_sizes[i]),
            .data = r->data.buf[render_indices[i]],
            // bitmaps are zeroed automatically
        };
        LOG("array %d: %p, %p", i, data->renders, data->data);
    }
}

void renderer_set_level(renderer_t *r, level_t *level) {
    renderer_destroy_for_level(r);

    for (int i = 0; i < 4; i++) {
        renderer_data_array_t *data = &r->data_arrays[i];
        memset(data->renders, 0, 2048 * data->render_size);
        bitmap_fill(data->all_bits, 2048, false);
        bitmap_fill(data->frame_bits, 2048, false);
    }

    dynbuf_reset(&r->db_indices);
    dynbuf_reset(&r->db_vertices);
    r->level = level;
}

void renderer_destroy_for_level(renderer_t *r) {
    if (!r->level) { return; }
    r->version++;
}

void renderer_destroy(renderer_t *r) {
    gfx_unload_shader(&r->shader_level);
    sg_destroy_pipeline(r->pipeline_level);
    sg_destroy_pipeline(r->pipeline_portal);
    sg_destroy_pipeline(r->pipeline_sprite);
    sg_destroy_buffer(r->level_ibuf);
    sg_destroy_buffer(r->level_vbuf);
    sg_destroy_buffer(r->sprite_ibuf);
    sg_destroy_buffer(r->sprite_vbuf);
    sg_destroy_buffer(r->sprite_instbuf);

    free(r->instance_data.ptr);

    dynbuf_destroy(&r->db_indices);
    dynbuf_destroy(&r->db_vertices);

    if (r->frame_info.data) {
        free(r->frame_info.data);
    }
}

static void mesh_decal(
    renderer_t *r,
    decal_t *decal,
    u16 *indices,
    int *ni,
    render_vertex_t *vertices,
    int *nv) {
    decal->render->indices = &indices[*ni];
    decal->render->vertices = &vertices[*nv];

    const u16 base = *nv;
    const int index = decal->render->index;

    atlas_lookup_t lookup;
    atlas_lookup(state->atlas, decal->tex, &lookup);

    const vec2s size =
        glms_vec2_divs(aabbf_size(AABB_TO_F(lookup.box_px)), PX_PER_UNIT);

    vec3s verts[4];
    bool wind_cw = false;

    if (decal->is_on_side) {
        vertex_t *vs[2];
        side_get_vertices(decal->side.ptr, vs);

        const vec2s
            offset = glms_vec2_scale(side_normal(decal->side.ptr), 0.001f),
            v0 =
                glms_vec2_add(
                    glms_vec2_lerp(
                       vs[0]->pos,
                       vs[1]->pos,
                       (decal->side.offsets.x - (size.x / 2.0f))
                           / decal->side.ptr->wall->len),
                   offset),
            v1 =
                glms_vec2_add(
                   glms_vec2_lerp(
                       vs[0]->pos,
                       vs[1]->pos,
                       (decal->side.offsets.x + (size.x / 2.0f))
                           / decal->side.ptr->wall->len),
                   offset);

        const f32
            z_base = decal->side.ptr->sector->floor.z,
            z0 = z_base + decal->side.offsets.y - (size.y / 2.0f),
            z1 = z_base + decal->side.offsets.y + (size.y / 2.0f);

        verts[0] = VEC3(v0, z0);
        verts[1] = VEC3(v1, z0);
        verts[2] = VEC3(v0, z1);
        verts[3] = VEC3(v1, z1);
    } else {
        f32 z;
        if (decal->sector.plane == PLANE_TYPE_CEIL) {
             z = decal->sector.ptr->ceil.z - 0.001f;
        } else {
            wind_cw = true;
            z = decal->sector.ptr->floor.z + 0.001f;
        }

        const vec2s
            v0 = glms_vec2_sub(decal->sector.pos, glms_vec2_divs(size, 2.0f)),
            v1 = glms_vec2_add(decal->sector.pos, glms_vec2_divs(size, 2.0f));

        verts[0] = VEC3(v0.x, v0.y, z);
        verts[1] = VEC3(v1.x, v0.y, z);
        verts[2] = VEC3(v0.x, v1.y, z);
        verts[3] = VEC3(v1.x, v1.y, z);
    }

    // bottom left
    vertices[(*nv)++] = (render_vertex_t) {
        .pos = verts[0],
        .uv = wind_cw ? VEC2(0.0f, 0.0f) : VEC2(1.0f, 0.0f),
        .id = (T_DECAL << 16) | ((int) decal->index),
        .index = index,
        .flags = RENDERER_VFLAG_NONE,
    };

    // bottom right
    vertices[(*nv)++] = (render_vertex_t) {
        .pos = verts[1],
        .uv = wind_cw ? VEC2(1.0f, 0.0f) : VEC2(0.0f, 0.0f),
        .id = (T_DECAL << 16) | ((int) decal->index),
        .index = index,
        .flags = RENDERER_VFLAG_NONE,
    };

    // top left
    vertices[(*nv)++] = (render_vertex_t) {
        .pos = verts[2],
        .uv = wind_cw ? VEC2(0.0f, 1.0f) : VEC2(1.0f, 1.0f),
        .id = (T_DECAL << 16) | ((int) decal->index),
        .index = index,
        .flags = RENDERER_VFLAG_NONE,
    };

    // top right
    vertices[(*nv)++] = (render_vertex_t) {
        .pos = verts[3],
        .uv = wind_cw ? VEC2(1.0f, 1.0f) : VEC2(0.0f, 1.0f),
        .id = (T_DECAL << 16) | ((int) decal->index),
        .index = index,
        .flags = RENDERER_VFLAG_NONE,
    };

    indices[(*ni)++] = base + (wind_cw ? 0 : 2);
    indices[(*ni)++] = base + (wind_cw ? 1 : 1);
    indices[(*ni)++] = base + (wind_cw ? 2 : 0);
    indices[(*ni)++] = base + (wind_cw ? 1 : 2);
    indices[(*ni)++] = base + (wind_cw ? 3 : 3);
    indices[(*ni)++] = base + (wind_cw ? 2 : 1);
}

static int prepare_decal(renderer_t *r, decal_t *decal) {
    int res = PREPARE_OK;

    int index;
    decal_render_data_t *dr_data;
    const int da_res =
        data_array_realloc(
            r, &r->decal_data,
            !decal->render
            || decal->render->decal != decal
            || decal->render->version != decal->version
            || decal->render->r_version != r->version,
            &decal->render,
            &dr_data,
            &index);

    switch (da_res) {
    case DATA_REALLOC_KEPT:
        res = PREPARE_OK;
        goto done;
    case DATA_REALLOC_FAIL:
        ASSERT(false);
    }

    decal_render_t *dr = decal->render;
    *dr = (decal_render_t) {
        .index = index,
        .decal = decal,
        .version = decal->version,
        .r_version = r->version,
    };

    atlas_lookup_t lookup;
    atlas_lookup(state->atlas, decal->tex, &lookup);
    dr_data->tex = lookup.id;

    // TODO: will break if done before these have been updated
    if (decal->is_on_side) {
        dr_data->sector_index = decal->side.ptr->sector->render->index;
        dr_data->side_index = decal->side.ptr->render->index;
        dr_data->plane_type = 0;
    } else {
        dr_data->sector_index = decal->sector.ptr->render->index;
        dr_data->side_index = 0;
        dr_data->plane_type = decal->sector.plane;
    }

done:
    r->data.dirty |= (res != PREPARE_OK);
    return PREPARE_REMESH;
}

static int prepare_side(renderer_t *r, side_t *side) {
    int res = PREPARE_OK;

    if (side->render && side->render->side != side) {
        LOG("I GOT OVERWRITTEN @ %d",
            (int) (side->render - (side_render_t*) r->side_data.renders));
    }

    int index;
    side_render_data_t *sr_data;
    const int da_res =
        data_array_realloc(
            r, &r->side_data,
            !side->render
            || side->render->side != side
            || side->render->version != side->version
            || side->render->r_version != r->version,
            &side->render,
            &sr_data,
            &index);

    switch (da_res) {
    case DATA_REALLOC_OVERWRITE:
        /* LOG("OVERWRITING index %d", index); */
        break;
    case DATA_REALLOC_KEPT:
        res = PREPARE_OK;
        goto done;
    case DATA_REALLOC_FAIL:
        ASSERT(false);
    }

    res = PREPARE_REMESH;

    side_render_t *sr = side->render;
    *sr = (side_render_t) {
        .index = index,
        .side = side,
        .r_version = r->version,
        .version = side->version,
    };

    vertex_t *vs[2];
    side_get_vertices(side, vs);

    const side_texinfo_t *tex_info =
        (side->flags & SIDE_FLAG_MATOVERRIDE) 
            ? &side->tex_info
            : &side->mat->tex_info;

    const bool has_neighbor = side->portal && side->portal->sector;
    *sr_data = (side_render_data_t) {
        .origin = vs[0]->pos,
        .len = side->wall->len,
        .split_bottom = tex_info->split_bottom,
        .split_top = tex_info->split_top,
        .stflags = tex_info->flags,
        .z_floor = side->sector->floor.z,
        .z_ceil = side->sector->ceil.z,
        .nz_floor = has_neighbor 
            ? side->portal->sector->floor.z 
            : 0.0f,
        .nz_ceil = has_neighbor 
            ? side->portal->sector->ceil.z 
            : 0.0f,
        .offsets = IVEC_TO_V(tex_info->offsets),
        .sector_index = side->sector->render->index,
        .normal = side_normal(side)
    };

    atlas_lookup_t lookup;
    atlas_lookup(state->atlas, side->mat->tex_low, &lookup);
    sr_data->tex_low = lookup.id;

    atlas_lookup(state->atlas, side->mat->tex_mid, &lookup);
    sr_data->tex_mid = lookup.id;

    atlas_lookup(state->atlas, side->mat->tex_high, &lookup);
    sr_data->tex_high = lookup.id;

done:
    r->data.dirty |= (res != PREPARE_OK);
    return res;
}

static void mesh_side(
    renderer_t *r,
    sector_render_t *sector_render,
    side_t *side,
    const side_segment_t *seg,
    u16 *indices,
    int *ni,
    render_vertex_t *vertices,
    int *nv) {
    vertex_t *vs[2];
    side_get_vertices(side, vs);

    int flags = seg->portal 
        ? RENDERER_VFLAG_PORTAL 
        : 0;

    switch (seg->index) {
    case SIDE_SEGMENT_WALL: flags |= RENDERER_VFLAG_SEG_MIDDLE; break;
    case SIDE_SEGMENT_MIDDLE: flags |= RENDERER_VFLAG_SEG_MIDDLE; break;
    case SIDE_SEGMENT_TOP: flags |= RENDERER_VFLAG_SEG_TOP; break;
    case SIDE_SEGMENT_BOTTOM: flags |= RENDERER_VFLAG_SEG_BOTTOM; break;
    }

    if (flags & RENDERER_VFLAG_PORTAL) {
        *dynlist_push(sector_render->portals) =
            (sector_render_portal_t) {
                .sector = sector_render->sector,
                .side = side,
                .indices = &indices[*ni]
            };
    }

    const u16 base = *nv;
    const int index = side->render->index;

    // bottom left
    vertices[(*nv)++] = (render_vertex_t) {
        .pos =
            VEC3(
                vs[0]->pos.x,
                vs[0]->pos.y,
                seg->z0),
        .uv = VEC2(1.0f, 0.0f),
        .id = (T_SIDE << 16) | ((int) side->index),
        .index = index,
        .flags = flags,
    };

    // bottom right
    vertices[(*nv)++] = (render_vertex_t) {
        .pos =
            VEC3(
                vs[1]->pos.x,
                vs[1]->pos.y,
                seg->z0),
        .uv = VEC2(0.0f, 0.0f),
        .id = (T_SIDE << 16) | ((int) side->index),
        .index = index,
        .flags = flags,
    };

    // top left
    vertices[(*nv)++] = (render_vertex_t) {
        .pos =
            VEC3(
                vs[0]->pos.x,
                vs[0]->pos.y,
                seg->z1),
        .uv = VEC2(1.0f, 1.0f),
        .id = (T_SIDE << 16) | ((int) side->index),
        .index = index,
        .flags = flags,
    };

    // top right
    vertices[(*nv)++] = (render_vertex_t) {
        .pos =
            VEC3(
                vs[1]->pos.x,
                vs[1]->pos.y,
                seg->z1),
        .uv = VEC2(0.0f, 1.0f),
        .id = (T_SIDE << 16) | ((int) side->index),
        .index = index,
        .flags = flags,
    };

    indices[(*ni)++] = base + 2;
    indices[(*ni)++] = base + 1;
    indices[(*ni)++] = base + 0;
    indices[(*ni)++] = base + 2;
    indices[(*ni)++] = base + 3;
    indices[(*ni)++] = base + 1;
}

// deallocate for an existing sector
static void prepare_sector_dealloc(
    renderer_t *r,
    sector_render_t *sr,
    sector_t *sector) {
    llist_each(sector_sides, &sector->sides, it_s) {
        it_s.el->render = NULL;

        llist_each(node, &it_s.el->decals, it_d) {
            it_d.el->render = NULL;
        }
    }

    llist_each(node, &sector->decals, it_d) {
        it_d.el->render = NULL;
    }

    // deallocate, entire sector must be remeshed
    if (sr->indices) {
        dynbuf_free(&r->db_indices, sector->render->indices);
    }

    if (sr->vertices) {
        dynbuf_free(&r->db_vertices, sector->render->vertices);
    }

    dynlist_free(sr->portals);
}

// returns true on data change
static int prepare_sector(renderer_t *r, sector_t *sector) {
    int res = PREPARE_OK;

    int index;
    sector_render_data_t *sr_data;
    const int da_res =
        data_array_realloc(
            r, &r->sector_data,
            !sector->render
            || sector->render->sector != sector
            || sector->render->version != sector->version
            || sector->render->r_version != r->version,
            &sector->render,
            &sr_data,
            &index);

    sector_render_t *sr = sector->render;

    switch (da_res) {
    case DATA_REALLOC_OVERWRITE:
    case DATA_REALLOC_NEW:
        goto remesh;
    case DATA_REALLOC_KEPT:
        res = PREPARE_OK;
        break;
    case DATA_REALLOC_FAIL:
        ASSERT(false);
    }

    // prepare all children, check if any need to remesh
    llist_each(sector_sides, &sector->sides, it_s) {
        switch (prepare_side(r, it_s.el)) {
        case PREPARE_REMESH: goto remesh;
        case PREPARE_DATA_UPDATE: res = PREPARE_DATA_UPDATE; break;
        }

        llist_each(node, &it_s.el->decals, it_d) {
            switch (prepare_decal(r, it_d.el)) {
            case PREPARE_REMESH: goto remesh;
            case PREPARE_DATA_UPDATE: res = PREPARE_DATA_UPDATE; break;
            }
        }
    }

    llist_each(node, &sector->decals, it_d) {
        switch (prepare_decal(r, it_d.el)) {
        case PREPARE_REMESH: goto remesh;
        case PREPARE_DATA_UPDATE: res = PREPARE_DATA_UPDATE; break;
        }
    }

    // TODO: really weird control flow in this function
    goto done;
remesh:
    res = PREPARE_REMESH;

    // must deallocate existing data
    if (da_res != DATA_REALLOC_NEW) {
        prepare_sector_dealloc(r, sector->render, sector);
    }

    *sr = (sector_render_t) {
        .index = index,
        .sector = sector,
        .version = sector->version,
        .r_version = r->version,
    };

    // need to precalculate which portals get drawn partially
    int n_quads = sector->n_decals;

    llist_each(sector_sides, &sector->sides, it) {
        side_segment_t segs[4];
        side_get_segments(it.el, segs);
        for (int i = 0; i < 4; i++) {
            if (segs[i].mesh) {
                n_quads++;
            }
        }
        n_quads += it.el->n_decals;
    }

    int ni = 0, nv = 0;

    if (n_quads == 0 && dynlist_size(sector->tris) == 0) {
        sr->vertices = NULL;
        sr->indices = NULL;
        sr->n_vertices = 0;
        sr->n_indices = 0;
        goto done;
    }

    atlas_lookup_t lookup;
    atlas_lookup(state->atlas, sector->mat->floor.tex, &lookup);
    sr_data->tex_floor = lookup.id;

    atlas_lookup(state->atlas, sector->mat->ceil.tex, &lookup);
    sr_data->tex_ceil = lookup.id;

    // allocate indices: (3 * 2 (planes) * num tris) + (6 * num sides)
    // same for vertices, but 4/side instead of 6
    const int ni_expected = (3 * 2 * dynlist_size(sector->tris) + (6 * n_quads));
    const int nv_expected = (3 * 2 * dynlist_size(sector->tris) + (4 * n_quads));
    sr->indices =
        dynbuf_alloc(
                &r->db_indices, 
                sizeof(u16) 
                * ni_expected);
    sr->n_indices = ni_expected;

    sr->vertices =
        dynbuf_alloc(
                &r->db_vertices, 
                sizeof(render_vertex_t) 
                * nv_expected);
    sr->n_vertices = nv_expected;

    u16 *indices = sr->indices;
    render_vertex_t *vertices = sr->vertices;

    // floor plane
    dynlist_each(sector->tris, it) {
        for (int j = 0; j < 3; j++) {
            indices[ni++] = nv;
            vertices[nv++] = (render_vertex_t) {
                .pos =
                    VEC3(
                        it.el->vs[j]->pos.x,
                        it.el->vs[j]->pos.y,
                        sector->floor.z),
                .uv = VEC2(0.0f),
                .id = (T_SECTOR << 16) | ((int) sector->index),
                .index = index,
                .flags = RENDERER_VFLAG_NONE,
            };
        }
    }

    // ceiling plane
    dynlist_each(sector->tris, it) {
        for (int j = 0; j < 3; j++) {
            indices[ni++] = nv;
            vertices[nv++] = (render_vertex_t) {
                .pos =
                    VEC3(
                        it.el->vs[2 - j]->pos.x,
                        it.el->vs[2 - j]->pos.y,
                        sector->ceil.z),
                .uv = VEC2(0.0f),
                .id = (T_SECTOR << 16) | ((int) sector->index),
                .index = index,
                .flags = RENDERER_VFLAG_IS_CEIL,
            };
        }
    }

    // sides
    llist_each(sector_sides, &sector->sides, it_s) {
        side_t *s = it_s.el;
        prepare_side(r, s);

        // verify that it has been prepared
        side_render_t *sr_side = s->render;
        ASSERT(sr_side->side == s);
        ASSERT(sr_side->version == s->version);

        side_segment_t segs[4];
        side_get_segments(s, segs);

        for (int i = 0; i < 4; i++) {
            if (segs[i].present && segs[i].mesh) {
                mesh_side(r, sr, s, &segs[i], indices, &ni, vertices, &nv);
            }
        }

        llist_each(node, &s->decals, it_d) {
            prepare_decal(r, it_d.el);
            mesh_decal(r, it_d.el, indices, &ni, vertices, &nv);
        }
    }

    // decals
    llist_each(node, &sector->decals, it_d) {
        prepare_decal(r, it_d.el);
        mesh_decal(r, it_d.el, indices, &ni, vertices, &nv);
    }

    ASSERT(ni == ni_expected);
    ASSERT(nv == nv_expected);

done:;
    const u8 light = sector_light(r->level, sector);
    if (light != (sr_data->light * LIGHT_MAX)) {
        sr_data->light = light / (f32) LIGHT_MAX;
        res = PREPARE_DATA_UPDATE;
    }

    r->data.dirty |= (res != PREPARE_OK);
    return res;
}

static int prepare_particle_type(
    renderer_t *r,
    particle_type_index type_index) {
    int res = PREPARE_OK;

    int index;
    sprite_render_data_t *sr_data;
    sprite_render_t *sr = r->particle_data.ptrs[type_index];
    const int da_res =
        data_array_realloc(
            r, &r->sprite_data,
            !sr
            || sr->is_object
            || sr->particle.type != type_index,
            &sr,
            &sr_data,
            &index);

    switch (da_res) {
    case DATA_REALLOC_KEPT:
        res = PREPARE_OK;
        goto done;
    case DATA_REALLOC_FAIL:
        ASSERT(false);
    }

    res = PREPARE_DATA_UPDATE;

    r->particle_data.ptrs[type_index] = sr;
    r->particle_data.indices[type_index] = index;

    sr->index = index;
    sr->is_object = false;
    sr->particle.type = type_index;

    const particle_type_t *type = &PARTICLE_TYPES[type_index];

    atlas_lookup(state->atlas, type->tex, &sr->lookup);

    sr_data->tex = sr->lookup.id;
    // TODO
    sr_data->sector_index = 0;

done:
    r->data.dirty = (res != PREPARE_OK);
    return res;
}

static bool prepare_particle(
    renderer_t *r,
    particle_t *p,
    sprite_instance_t *inst) {
    const bool res = prepare_particle_type(r, p->type);
    sprite_render_t *sr = r->particle_data.ptrs[p->type];

    *inst = (sprite_instance_t) {
        .offset = p->pos_xyz,
        .size =
            aabbf_size(
                aabbf_scale_min(
                    AABB_TO_F(sr->lookup.box_px),
                    VEC2(1.0f / PX_PER_UNIT))),
        .id = 0,
        .index = r->particle_data.indices[p->type],
        .color = VEC4(1),
        .flags = 0
    };

    particle_instance(r->level, p, inst);
    return res;
}

static int prepare_object(
    renderer_t *r,
    object_t *obj,
    sprite_instance_t *inst) {

    // TODO: store lookup results on object render
    atlas_lookup_t lookup;
    atlas_lookup(state->atlas, obj->type->sprite, &lookup);

    int res = PREPARE_OK;

    int index;
    sprite_render_data_t *sr_data;
    const int da_res =
        data_array_realloc(
            r, &r->sprite_data,
            !obj->render
            || !obj->render->is_object
            || obj->render->object.ptr != obj
            || obj->render->object.version != obj->version
            || obj->render->object.r_version != r->version,
            &obj->render,
            &sr_data,
            &index);

    switch (da_res) {
    case DATA_REALLOC_KEPT:
        res = PREPARE_OK;
        goto prepare_instance;
    case DATA_REALLOC_FAIL:
        ASSERT(false);
    }

    res = PREPARE_DATA_UPDATE;

    sprite_render_t *sr = obj->render;
    sr->index = index;
    sr->is_object = true;
    sr->object.ptr = obj;
    sr->object.r_version = r->version;
    sr->object.version = obj->version;

    sr_data->tex = lookup.id;

    if (obj->sector->render) {
        sr_data->sector_index = obj->sector->render->index;
    } else {
        WARN("rendering object %d without sector_index", obj->index);
    }

prepare_instance:
    *inst = (sprite_instance_t) {
        .offset = VEC3(obj->pos.x, obj->pos.y, obj->z),
        .size =
            aabbf_size(
                aabbf_scale_min(
                    AABB_TO_F(lookup.box_px),
                    VEC2(1.0f / PX_PER_UNIT))),
        .id = (T_OBJECT << 16) | ((int) obj->index),
        .index = index,
        .color = VEC4(1),
        .flags = 0
    };

    r->data.dirty |= (res != PREPARE_OK);
    return res;
}

static void deconstruct_view_matrix(
    mat4s view,
    vec3s *pdir,
    vec3s *ppos,
    f32 *pyaw,
    f32 *ppitch) {
    vec3s dir, pos;
    f32 yaw, pitch;

    const mat4s view_inv = glms_mat4_inv(view);
    pos = VEC3(view_inv.m30, view_inv.m31, view_inv.m32);
    dir = VEC3(-view_inv.m20, -view_inv.m21, -view_inv.m22);
    yaw = atan2f(dir.y, dir.x);
    pitch = asinf(dir.z);

    if (pdir) { *pdir = dir; }
    if (ppos) { *ppos = pos; }
    if (pyaw) { *pyaw = yaw; }
    if (ppitch) { *ppitch = pitch; }
}

static bool side_clip(
    const side_t *side,
    const mat4s *proj,
    const mat4s *view,
    const mat4s *view_proj,
    aabb_t scissor,
    aabb_t *scissor_out) {
    vertex_t *vs[2];
    side_get_vertices(side, vs);

    vec4s planes[6];
    extract_view_proj_planes(view_proj, planes);

    // TODO: use indices, don't need to check same vertices twice
    // check that vertices are on screen 
    const vec3s ps[6] = {
        VEC3(vs[0]->pos, side->sector->floor.z),
        VEC3(vs[1]->pos, side->sector->floor.z),
        VEC3(vs[1]->pos, side->sector->ceil.z),

        VEC3(vs[0]->pos, side->sector->ceil.z),
        VEC3(vs[1]->pos, side->sector->ceil.z),
        VEC3(vs[0]->pos, side->sector->floor.z),
    };

    // points clamped to view frustum
    vec3s ps_clamp[6];

    int outside[6] = { 0 }, hits[6] = { 0 };

    for (int i = 0; i < (int) ARRLEN(ps); i++) {
        vec3s p = ps[i];
        for (int j = 0; j < 6; j++) {
            if (plane_classify(planes[j], ps[i]) < 0) {
                hits[i] |= 1 << j;
            }

            if (plane_classify(planes[j], p) < 0) {
                outside[j]++;

                // clamp to plane
                p = plane_project(planes[j], p);
            }
        }
        ps_clamp[i] = p;
    }

    // throw away if all points are outside one plane
    for (int i = 0; i < 6; i++) {
        if (outside[i] == 6) { return false; }
    }

    // convert clamped -> view
    vec3s ps_view[6];
    for (int i = 0; i < (int) ARRLEN(ps); i++) {
        ps_view[i] = glms_vec3(glms_mat4_mulv(*view, VEC4(ps_clamp[i], 1.0f)));
    }

    int not_ccw = 0;
    for (int i = 0; i < 1; i++) {
        const mat3s m = (mat3s) {
            .col = {
                glms_vec3(glms_mat4_mulv(*view, VEC4(ps[2], 1.0f))),
                glms_vec3(glms_mat4_mulv(*view, VEC4(ps[1], 1.0f))),
                glms_vec3(glms_mat4_mulv(*view, VEC4(ps[0], 1.0f))),
            }
        };

        if (glms_mat3_det(m) > 0) {
            not_ccw++;
        }
    }

    if (not_ccw == 1) {
        if (state->renderer->debug_ui) {
            igText("rejecting %d, not CCW", side->index);
        }
    }

    // screen (pixel) positions
    bool force_full = false;
    ivec2s ps_px[6];

    for (int i = 0; i < (int) ARRLEN(ps); i++) {
        vec4s p_clip =
            glms_mat4_mulv(
                *view_proj,
                VEC4(ps[i], 1.0f));

        p_clip.w = ifnaninf(p_clip.w, 1.0f, 1.0f);

        vec3s p_ndc = glms_vec3_divs(glms_vec3(p_clip), p_clip.w);
        ps_px[i] =
            VEC_TO_I(
                glms_vec2_mul(
                    VEC2(
                        0.5f * (p_ndc.x + 1.0f),
                        0.5f * (p_ndc.y + 1.0f)),
                    VEC2(TARGET_3D_WIDTH - 1, TARGET_3D_HEIGHT - 1)));
        ps_px[i] =
            glms_ivec2_clamp(
                ps_px[i], IVEC2(0), IVEC2(TARGET_3D_WIDTH - 1, TARGET_3D_HEIGHT - 1));

        if (hits[i] & (1 << PLANE_LEFT)) {
            ps_px[i].x = 0;
        }

        if (hits[i] & (1 << PLANE_RIGHT)) {
            ps_px[i].x = TARGET_3D_WIDTH - 1;
        }

        if (hits[i] & (1 << PLANE_BOTTOM)) {
            ps_px[i].y = 0;
        }

        if (hits[i] & (1 << PLANE_TOP)) {
            ps_px[i].y = TARGET_3D_HEIGHT - 1;
        }

        if (hits[i] & (1 << PLANE_NEAR)) {
            force_full = true;
            break;
        }
    }

    if (force_full) {
        *scissor_out = AABB_MM(IVEC2(0), IVEC2(TARGET_3D_WIDTH, TARGET_3D_HEIGHT));
    } else {
        // find min/max on screen
        ivec2s ps_min = ps_px[0], ps_max = ps_px[0];
        for (int i = 0; i < (int) ARRLEN(ps_px); i++) {
            ps_min.x = min(ps_min.x, ps_px[i].x);
            ps_min.y = min(ps_min.y, ps_px[i].y);
            ps_max.x = max(ps_max.x, ps_px[i].x);
            ps_max.y = max(ps_max.y, ps_px[i].y);
        }

        *scissor_out = AABB_MM(ps_min, ps_max);
    }

    if (state->renderer->debug_ui) {
        igText("%d scissored to %" PRIaabb, side->index, FMTaabb(*scissor_out));
    }

    // clip against existing scissor rect
    if (aabb_collides(*scissor_out, scissor)) {
        *scissor_out = aabb_intersect(*scissor_out, scissor);
        return true;
    } else {
        return false;
    }
}

static mat4s clipped_proj_mat(
    const side_t *out,
    mat4s proj,
    mat4s view) {
    vec3s view_pos;
    deconstruct_view_matrix(view, NULL, &view_pos, NULL, NULL);

    const vec3s pos = VEC3(wall_midpoint(out->wall), 0.0f);
    const vec3s normal = VEC3(side_normal(out), 0.0f);
    const f32 sgn = sign(glms_vec3_dot(normal, glms_vec3_sub(pos, view_pos)));

    const vec3s
        pos_v = glms_vec3(glms_mat4_mulv(view, VEC4(pos, 1.0))),
        normal_v =
            glms_vec3_scale(
                glms_vec3(glms_mat4_mulv(view, VEC4(normal, 0.0))),
                sgn);

    const f32 offset = 0.005f;
    const f32 dist_v = -glms_vec3_dot(pos_v, normal_v) + offset;

    // TODO: when close, don't modify 
    // - this saves us some z-fighting issues
    if (fabsf(dist_v) < 0.25f) {
        return proj;
    }

    const vec4s clip_plane = VEC4(normal_v, dist_v);

    const vec4s q =
        glms_mat4_mulv(
            glms_mat4_inv(proj),
            VEC4(
                sign(clip_plane.x),
                sign(clip_plane.y),
                1.0f,
                1.0f));

    const vec4s c =
        glms_vec4_scale(clip_plane, 2.0f / glms_vec4_dot(clip_plane, q));

    f32 a =
        (2.0f * glms_vec4_dot(proj.col[3], q)) / glms_vec4_dot(c, q);

    // row 2 = c - row 3
    f32 *raw = (f32*) proj.raw;
    raw[2]  = (a * c.x) - raw[3];
    raw[6]  = (a * c.y) - raw[7];
    raw[10] = (a * c.z) - raw[11];
    raw[14] = (a * c.w) - raw[15];

	return proj;
}

static void portal_view_proj(
    const side_t *in,
    const side_t *out,
    mat4s proj,
    mat4s view,
    mat4s *proj_out,
    mat4s *view_out) {
    vertex_t *vs_out[2], *vs_in[2];
    side_get_vertices(in, vs_in);
    side_get_vertices(out, vs_out);
    const vec2s
        n_in = side_normal(in),
        n_out = side_normal(out),
        mid_in = glms_vec2_lerp(vs_in[0]->pos, vs_in[1]->pos, 0.5f),
        mid_out = glms_vec2_lerp(vs_out[0]->pos, vs_out[1]->pos, 0.5f);
    const mat4s
        model_in =
            glms_mat4_mul(
                glms_translate_make(VEC3(mid_in, in->sector->floor.z)),
                glms_rotate(
                    glms_mat4_identity(),
                    atan2f(n_in.y, n_in.x),
                    VEC3(0, 0, 1))),
        model_out =
            glms_mat4_mul(
                glms_translate_make(VEC3(mid_out, out->sector->floor.z)),
                glms_rotate(
                    glms_mat4_identity(),
                    atan2f(n_out.y, n_out.x),
                    VEC3(0, 0, 1)));
    // TODO: auto recurse if this goes through the same portal twice (linked
    // portal is visible through this portal)
    *view_out =
        glms_mat4_mul(
            glms_mat4_mul(
                glms_mat4_mul(view, model_in),
                glms_rotate(glms_mat4_identity(), PI, VEC3(0, 0, 1))),
            glms_mat4_inv(model_out));

    // clip on the OUT side. that is where (in view space) we want to oblique
    // near clipping plane to be placed.
    *proj_out =
        clipped_proj_mat(
            out,
            proj,
            *view_out);
}

typedef struct render_pass {
    mat4s view, proj, view_proj;
    f32 yaw;
    aabb_t scissor;
    u8 stencil_ref;
    int depth;
    side_t *entry_side, *exit_side;
    sector_t *sector;
    const struct render_pass *from;
} render_pass_t;

static int sector_render_portal_cmp(
    const sector_render_portal_t **a,
    const sector_render_portal_t **b,
    vec3s *ref_pos) {
    const f32
        d_a =
            glms_vec2_norm2(
                glms_vec2_sub(
                    glms_vec2(*ref_pos), wall_midpoint((*a)->side->wall))),
        d_b =
            glms_vec2_norm2(
                glms_vec2_sub(
                    glms_vec2(*ref_pos), wall_midpoint((*b)->side->wall)));
    return d_a - d_b;
}

static void do_render_pass(
    renderer_t *r,
    const render_pass_t *pass) {
    // TODO: formalize limits
    if (pass->depth >= 8) {
        return;
    }

    char name[256];
    snprintf(
        name,
        sizeof(name),
        "PASS (side: %d -> %d/st: %d/dp: %d)",
        pass->entry_side ? pass->entry_side->index : -1,
        pass->exit_side ? pass->exit_side->index : -1,
        pass->stencil_ref,
        pass->depth);
    const bool ui = r->debug_ui && igTreeNode_Str(name);
    igTreeNodeSetOpen(igGetItemID(), true);
    if (ui) { igText("SCISSOR: %" PRIaabb, FMTaabb(pass->scissor)); }
    if (ui) {
        igText("VIEW:\n %" PRIm4, FMTm4(pass->view));
    }

    // first - draw and stencil portals
    level_vs_params_t vs_params;
    vs_params.view = pass->view;
    vs_params.proj = pass->proj;

    level_fs_params_t fs_params;
    fs_params.cam_pos = r->cam.pos;
    fs_params.is_portal_pass = true;

    sg_bindings bind =
        (sg_bindings) {
            .vs_images = {
                [SLOT_level_data_image] = r->data_image
            },
            .fs_images = {
                [SLOT_level_atlas] = state->atlas->image,
                [SLOT_level_atlas_coords] = state->atlas->coords_image,
                [SLOT_level_atlas_layers] = state->atlas->layer_image,
                [SLOT_level_palette] = state->palette->image,
                [SLOT_level_data_image] = r->data_image,
            },
            .index_buffer = r->level_ibuf,
            .vertex_buffers[0] = r->level_vbuf,
        };

    // accumulate visible sectors
    DYNLIST(sector_t*) sectors = NULL; 
    level_dynlist_each(r->level->sectors, it) {
        *dynlist_push(sectors) = *it.el;
    }

    // accumulate list of distance-sorted portals
    DYNLIST(sector_render_portal_t*) portals = NULL;

    dynlist_each(sectors, it) {
        if (r->debug_ui) {
            igText("rendering sector %d", (*it.el)->index);
        }

        sector_render_t *sr = (*it.el)->render;
        if (!sr->indices && !sr->vertices) { continue; }

        dynlist_each(sr->portals, it) {
            sector_render_portal_t *srp = it.el;
            side_t *side = srp->side;
            // TODO: breaking recursive portals
            if (side == pass->exit_side) {
                continue;
            }

            if (pass->exit_side) {
                const vec2s
                    exit_normal = side_normal(pass->exit_side),
                    normal = side_normal(side);

                if (glms_vec2_dot(exit_normal, normal) > 0.0f) {
                    continue;
                }
            }

            side_render_t *side_render = side->render;
            ASSERT(
                side_render->side == side,
                "%p/%p/%p",
                side_render,
                side_render->side,
                side);
            ASSERT(side_render->version == side->version);

            if (!side->portal) {
                WARN("invalid portals entry!");
                dynlist_remove_it(sr->portals, it);
                continue;
            }

            aabb_t scissor;
            if (!side_clip(
                    side,
                    &pass->proj,
                    &pass->view,
                    &pass->view_proj,
                    pass->scissor,
                    &scissor)) {
                if (ui) {
                    igText("side %d is not visible from %d, skipping",
                        side ? side->index : -1,
                        pass->exit_side ? pass->exit_side->index : -1);
                }
                continue;
            }

            srp->scissor = scissor;
            *dynlist_push(portals) = srp;
        }
    }

    vec3s ref_pos =
        pass->exit_side ?
            VEC3(wall_midpoint(pass->exit_side->wall), 1.0f)
            : r->cam.pos;

    sort(
        portals,
        dynlist_size(portals),
        sizeof(portals[0]), 
        (f_sort_cmp) sector_render_portal_cmp,
        &ref_pos);

    dynlist_each(portals, it) {
        sector_render_portal_t *srp = *it.el;
        side_t *side = srp->side;
        sector_render_t *sr = srp->sector->render;

        mat4s view_portal, proj_portal;
        portal_view_proj(
            side,
            side->portal,
            pass->proj,
            pass->view,
            &proj_portal,
            &view_portal);

        // draw portal outline with EQUAL for stenciling, but INCR where pass
        sg_apply_pipeline(r->pipeline_portal);

        sg_apply_uniforms(
            SG_SHADERSTAGE_VS, SLOT_level_vs_params,
            &(sg_range) { &vs_params, sizeof(vs_params) });

        sg_apply_uniforms(
            SG_SHADERSTAGE_FS, SLOT_level_fs_params,
            &(sg_range) { &fs_params, sizeof(fs_params) });

        bind.index_buffer_offset = sr->indices - r->db_indices.ptr;
        bind.vertex_buffer_offsets[0] = sr->vertices - r->db_vertices.ptr;
        sg_apply_bindings(&bind);

        glEnable(GL_STENCIL_TEST);
        glStencilFuncSeparate(GL_FRONT, GL_EQUAL, pass->stencil_ref, 0xFF);
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR);
        glColorMaski(0, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        sg_draw(
            (int) (((u16*) srp->indices) - ((u16*) (sr->indices))), 6, 1);

        // reset to "expected state" before messing with sokol pipeline
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glStencilFuncSeparate(GL_FRONT, GL_EQUAL, 0, 0xFF);
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR);
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);

        // recursively draw inside of portal
        if (ui) {
            igText(
                "DOING PORTAL %d (from %d)",
                side->index,
                pass->exit_side ? pass->exit_side->index : -1);
            igText("  SCISSOR: %" PRIaabb, FMTaabb(srp->scissor));
        }
        do_render_pass(
            r,
            &(render_pass_t) {
                .proj = proj_portal,
                .view = view_portal,
                .view_proj = glms_mat4_mul(proj_portal, view_portal),
                .yaw =
                    pass->yaw + portal_angle(r->level, side, side->portal),
                .scissor = srp->scissor,
                .stencil_ref = pass->stencil_ref + 1,
                .depth = pass->depth + 1,
                .entry_side = side,
                .exit_side = side->portal,
                .sector = side->portal->sector,
                .from = pass,
            });

        // draw again to decrement
        sg_apply_pipeline(r->pipeline_portal);

        sg_apply_uniforms(
            SG_SHADERSTAGE_VS, SLOT_level_vs_params,
            &(sg_range) { &vs_params, sizeof(vs_params) });

        sg_apply_uniforms(
            SG_SHADERSTAGE_FS, SLOT_level_fs_params,
            &(sg_range) { &fs_params, sizeof(fs_params) });

        bind.index_buffer_offset = sr->indices - r->db_indices.ptr;
        bind.vertex_buffer_offsets[0] = sr->vertices - r->db_vertices.ptr;

        // SECOND DRAW:
        // draw to paint proper depth over this part of the scene

        // do not write to stencil, color, ONLY depth
        glEnable(GL_STENCIL_TEST);
        glStencilFuncSeparate(GL_FRONT, GL_EQUAL, pass->stencil_ref + 1, 0xFF);
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
        glColorMaski(0, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_ALWAYS);

        sg_apply_bindings(&bind);
        sg_draw(
            (int) (((u16*) srp->indices) - ((u16*) (sr->indices))), 6, 1);

        if (r->debug_ui) {
            igText("drawing into %d depth buffer", side->index);
        }

        // reset to "expected state" before messing with sokol pipeline
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_TRUE);
        glEnable(GL_STENCIL_TEST);
        glEnable(GL_DEPTH_TEST);
        glStencilFuncSeparate(GL_FRONT, GL_EQUAL, 0, 0xFF);
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR);
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xFF);
        glStencilFuncSeparate(GL_FRONT, GL_EQUAL, 0, 0xFF);
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR);
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);

        // THIRD DRAW: decrement on failure to reset to original stencil
        // value
        sg_apply_bindings(&bind);

        // fail when inside, and decrement on failure
        glEnable(GL_STENCIL_TEST);
        glStencilFuncSeparate(GL_FRONT, GL_NOTEQUAL, pass->stencil_ref + 1, 0xFF);
        glStencilOpSeparate(GL_FRONT, GL_DECR, GL_KEEP, GL_KEEP);
        glColorMaski(0, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);

        sg_draw(
            (int) (((u16*) srp->indices) - ((u16*) (sr->indices))), 6, 1);

        // reset to "expected state" before messing with sokol pipeline
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        glStencilFuncSeparate(GL_FRONT, GL_EQUAL, 0, 0xFF);
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR);
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
    }

    // draw regular level geometry
    sg_apply_pipeline(r->pipeline_level);

    sg_apply_uniforms(
        SG_SHADERSTAGE_VS, SLOT_level_vs_params,
        &(sg_range) { &vs_params, sizeof(vs_params) });

    fs_params.is_portal_pass = false;
    sg_apply_uniforms(
        SG_SHADERSTAGE_FS, SLOT_level_fs_params,
        &(sg_range) { &fs_params, sizeof(fs_params) });

    const int base_sprites = r->n_sprites;
    dynlist_each(sectors, it) {
        // objects
        dlist_each(sector_list, &(*it.el)->objects, it_o) {
            prepare_object(r, it_o.el, &r->instance_data.ptr[r->n_sprites++]);
        }

        // particles
        dynlist_each((*it.el)->particles, it_p) {
            particle_t *p = &r->level->particles[*it_p.el];
            prepare_particle(r, p, &r->instance_data.ptr[r->n_sprites++]);
        }

        sector_render_t *sr = (*it.el)->render;
        if (!sr->indices && !sr->vertices) { continue; }
        if (sr->n_indices == 0 || sr->n_vertices == 0) { continue; }

        bind.index_buffer_offset = sr->indices - r->db_indices.ptr;
        bind.vertex_buffer_offsets[0] = sr->vertices - r->db_vertices.ptr;
        sg_apply_bindings(&bind);

        // only draw where EQUAL, do not INCR
        glEnable(GL_STENCIL_TEST);
        glStencilFuncSeparate(GL_FRONT, GL_LEQUAL, pass->stencil_ref, 0xFF);
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
        glDepthFunc(GL_LEQUAL);

        sg_draw(0, sr->n_indices, 1);

        // reset to "expected" state
        glDepthFunc(GL_LEQUAL);
        glStencilFuncSeparate(GL_FRONT, GL_EQUAL, 0, 0xFF);
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
    }

    // draw accumulated sprite instances
    {
        const int inst_bytes =
            (r->n_sprites - base_sprites) * sizeof(sprite_instance_t);
        sg_append_buffer(
            r->sprite_instbuf,
            &(sg_range) {
                .size = inst_bytes,
                .ptr = &r->instance_data.ptr[base_sprites]
            });

        sprite_vs_params_t vs_params;
        vs_params.yaw = pass->yaw;
        vs_params.view = pass->view;
        vs_params.proj = pass->proj;

        sg_apply_pipeline(r->pipeline_sprite);
        sg_apply_uniforms(
            SG_SHADERSTAGE_VS, SLOT_sprite_vs_params,
            &(sg_range) { &vs_params, sizeof(vs_params) });

        sprite_fs_params_t fs_params;
        fs_params.cam_pos = r->cam.pos;
        sg_apply_uniforms(
            SG_SHADERSTAGE_FS, SLOT_sprite_fs_params,
            &(sg_range) { &fs_params, sizeof(fs_params) });

        sg_bindings bind =
            (sg_bindings) {
                .vs_images = {
                    [SLOT_sprite_data_image] = r->data_image
                },
                .fs_images = {
                    [SLOT_sprite_atlas] = state->atlas->image,
                    [SLOT_sprite_atlas_coords] = state->atlas->coords_image,
                    [SLOT_sprite_atlas_layers] = state->atlas->layer_image,
                    [SLOT_sprite_palette] = state->palette->image,
                    [SLOT_sprite_data_image] = r->data_image,
                },
                .index_buffer = r->sprite_ibuf,
                .vertex_buffers[0] = r->sprite_vbuf,
                .vertex_buffers[1] = r->sprite_instbuf,
                .vertex_buffer_offsets[1] =
                    base_sprites * sizeof(sprite_instance_t)
            };

        glEnable(GL_STENCIL_TEST);
        glStencilFuncSeparate(GL_FRONT, GL_LEQUAL, pass->stencil_ref, 0xFF);
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
        glDepthFunc(GL_LEQUAL);

        sg_apply_bindings(&bind);
        sg_draw(0, 6, r->n_sprites - base_sprites);

        glEnable(GL_STENCIL_TEST);
        glDepthFunc(GL_LEQUAL);
        glStencilFuncSeparate(GL_FRONT, GL_EQUAL, 0, 0xFF);
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
    }

    if (ui) { 
        igTreePop(); 
    }

    dynlist_free(portals);
    dynlist_free(sectors);
}

void renderer_render(renderer_t *r) {
    if (r->debug_ui) { 
        igBegin("DEBUG", NULL, 0); 
    }

    r->n_sprites = 0;

    for (int i = 0; i < 4; i++) {
        bitmap_fill(r->data_arrays[i].frame_bits, 2048, false);
    }

    level_dynlist_each(r->level->sectors, it) {
        switch (prepare_sector(r, *it.el)) {
        case PREPARE_DATA_UPDATE:
        case PREPARE_REMESH:
            r->level_dirty = true;
            break;
        }
    }

    if (r->level_dirty) {
        r->level_dirty = false;

        sg_append_buffer(
            r->level_ibuf,
            &(sg_range) {
                .size = r->db_indices.used,
                .ptr = r->db_indices.ptr
            });

        sg_append_buffer(
            r->level_vbuf,
            &(sg_range) {
                .size = r->db_vertices.used,
                .ptr = r->db_vertices.ptr
            });
    }

    // update dirty images
    if (r->data.dirty) {
        r->data.dirty = false;

        sg_update_image(
            r->data_image,
            &(sg_image_data) {
                .subimage[0][0] = {
                    .ptr = r->data.buf,
                    .size = sizeof(r->data.buf),
                }
            });
    }

    const vec3s dir =
            glms_vec3_normalize(
                VEC3(
                    cosf(r->cam.pitch) * cosf(r->cam.yaw),
                    cosf(r->cam.pitch) * sinf(-r->cam.yaw),
                    sinf(r->cam.pitch)));

    const vec3s up = VEC3(0, 0, 1);

    const mat4s view =
            glms_lookat(
                r->cam.pos,
                glms_vec3_add(dir, r->cam.pos),
                up);
    const mat4s proj =
            glms_perspective(
                deg2rad(80.0f - lerp(0.0f, 16.0f, state->aim_factor / 0.2f)),
                TARGET_3D_WIDTH / (f32) TARGET_3D_HEIGHT,
                0.0025f,
                128.0f);

    r->view = view;
    r->proj = proj;
    r->view_proj = glms_mat4_mul(r->proj, r->view);
    r->frustum_box = extract_view_proj_aabb_2d(&r->view_proj);

    do_render_pass(
        r,
        &(render_pass_t) {
            .depth = 0,
            .view = r->view,
            .proj = r->proj,
            .view_proj = r->view_proj,
            .yaw = state->cam.yaw,
            .scissor = AABB_PS(IVEC2(0), IVEC2(TARGET_3D_WIDTH, TARGET_3D_HEIGHT)),
            .sector = state->cam.sector,
            .from = NULL
        });

    if (r->n_sprites > (int) (
        SPRITE_INSTBUF_SIZE 
            / sizeof(sprite_instance_t))) {
        WARN("too many sprites! (%d)", r->n_sprites);
        r->n_sprites =
            min(
                r->n_sprites, 
                SPRITE_INSTBUF_SIZE 
                / sizeof(sprite_instance_t));
    }

    if (r->debug_ui) { 
        igEnd(); 
    }
}

vec4s renderer_info_at(renderer_t *r, ivec2s pos) {
    const ivec2s frame_size = IVEC2(TARGET_3D_WIDTH, TARGET_3D_HEIGHT);

    if (!glms_ivec2_eq(r->frame_info.size, frame_size)) {
        if (r->frame_info.data) { free(r->frame_info.data); }

        r->frame_info.size = frame_size;
        r->frame_info.data =
            malloc(
                r->frame_info.size.x 
                * r->frame_info.size.y 
                * sizeof(vec4s));
    }

    if (r->frame_info.frame != state->time.frame) {
        sg_query_image_pixels(
            state->fbs.primary.info,
            r->frame_info.data,
            r->frame_info.size.x 
                * r->frame_info.size.y 
                * sizeof(vec4s));
    }

    return r->frame_info.data[pos.y * frame_size.x + pos.x];
}
