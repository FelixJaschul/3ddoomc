#include "gfx/gfx.h"
#include "gfx/atlas.h"
#include "gfx/sokol.h"
#include "util/dynlist.h"
#include "util/file.h"
#include "sdl2.h"
#include "state.h"
#include "reload.h"

// NOTE: include all shaders! so they don't have to be impl'd elsewhere
#define SOKOL_SHDC_IMPL
#include "shader/batch.glsl.h"
#include "shader/basic.glsl.h"
#include "shader/level.glsl.h"
#include "shader/sprite.glsl.h"
#include "shader/screenquad.glsl.h"

typedef struct {
    const sg_shader_desc*(*desc_fn)(sg_backend);
    void (*callback)(sg_shader*, void*);
    void *userdata;
    sg_shader *shader;
    int hook_id;
} shader_reload_t;

RELOAD_VISIBLE void on_screenquad_reload(sg_shader *shader, gfx_state_t *gs) {
    if (gs->screenquad.pipeline.id) {
        sg_destroy_pipeline(gs->screenquad.pipeline);
        sg_dealloc_pipeline(gs->screenquad.pipeline);
    }

    gs->screenquad.shader = *shader;
    gs->screenquad.pipeline = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = *shader,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT2,
                [1].format = SG_VERTEXFORMAT_FLOAT2,
            }
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true
        },
        .colors[0].blend = {
            .enabled = true,
            .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
            .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .op_rgb = SG_BLENDOP_ADD,
            .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
            .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .op_alpha = SG_BLENDOP_ADD,
        },
        .face_winding = SG_FACEWINDING_CCW,
        .cull_mode = SG_CULLMODE_BACK,
    });
}

RELOAD_VISIBLE void on_batch_reload(sg_shader *shader, gfx_state_t *gs) {
    if (gs->batch.pipeline.id) {
        sg_destroy_pipeline(gs->batch.pipeline);
        sg_dealloc_pipeline(gs->batch.pipeline);
    }

    gs->batch.shader = *shader;
    gs->batch.pipeline = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = *shader,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .buffers[1] = {
                .stride = sizeof(gfx_batcher_entry_t),
                .step_func = SG_VERTEXSTEP_PER_INSTANCE,
            },
            .attrs = {
                [ATTR_batch_vs_a_position] = {
                    .format = SG_VERTEXFORMAT_FLOAT2,
                    .buffer_index = 0,
                },
                [ATTR_batch_vs_a_texcoord0] = {
                    .format = SG_VERTEXFORMAT_FLOAT2,
                    .buffer_index = 0,
                },
                [ATTR_batch_vs_a_offset] = {
                    .format = SG_VERTEXFORMAT_FLOAT2,
                    .offset = offsetof(gfx_batcher_entry_t, offset),
                    .buffer_index = 1,
                },
                [ATTR_batch_vs_a_scale] = {
                    .format = SG_VERTEXFORMAT_FLOAT2,
                    .offset = offsetof(gfx_batcher_entry_t, scale),
                    .buffer_index = 1,
                },
                [ATTR_batch_vs_a_uvmin] = {
                    .format = SG_VERTEXFORMAT_FLOAT2,
                    .offset = offsetof(gfx_batcher_entry_t, uv_min),
                    .buffer_index = 1,
                },
                [ATTR_batch_vs_a_uvmax] = {
                    .format = SG_VERTEXFORMAT_FLOAT2,
                    .offset = offsetof(gfx_batcher_entry_t, uv_max),
                    .buffer_index = 1,
                },
                [ATTR_batch_vs_a_layer] = {
                    .format = SG_VERTEXFORMAT_FLOAT,
                    .offset = offsetof(gfx_batcher_entry_t, layer),
                    .buffer_index = 1,
                },
                [ATTR_batch_vs_a_color] = {
                    .format = SG_VERTEXFORMAT_FLOAT4,
                    .offset = offsetof(gfx_batcher_entry_t, color),
                    .buffer_index = 1,
                },
                [ATTR_batch_vs_a_z] = {
                    .format = SG_VERTEXFORMAT_FLOAT,
                    .offset = offsetof(gfx_batcher_entry_t, z),
                    .buffer_index = 1,
                },
                [ATTR_batch_vs_a_flags] = {
                    .format = SG_VERTEXFORMAT_FLOAT,
                    .offset = offsetof(gfx_batcher_entry_t, flags),
                    .buffer_index = 1,
                },
            }
        },
        .colors[0].blend = {
            .enabled = false,
            .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
            .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .op_rgb = SG_BLENDOP_ADD,
            .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
            .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .op_alpha = SG_BLENDOP_ADD,
        },
        .depth = {
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .compare = SG_COMPAREFUNC_ALWAYS,
            .write_enabled = true,
        },
        .face_winding = SG_FACEWINDING_CCW,
        .cull_mode = SG_CULLMODE_NONE,
    });
}

void gfx_state_init(gfx_state_t *gs) {
    *gs = (gfx_state_t) { 0 };

#ifdef RELOADABLE
    map_init(
        &gs->shader_reload_map,
        map_hash_id,
        NULL,
        NULL,
        NULL,
        map_cmp_id,
        NULL,
        NULL,
        NULL);
#endif // ifdef RELOADABLE

    const f32 vertices[] = {
        // pos       // uv
        0.0f, 1.0f,  0.0f, 1.0f,
        1.0f, 1.0f,  1.0f, 1.0f,
        1.0f, 0.0f,  1.0f, 0.0f,
        0.0f, 0.0f,  0.0f, 0.0f,
    };

    const u16 indices[] = {
        2, 1, 0,
        3, 2, 0
    };

    gs->screenquad.vbuf =
        sg_make_buffer(&(sg_buffer_desc) { .data = SG_RANGE(vertices) });
    gs->screenquad.ibuf =
        sg_make_buffer(
            &(sg_buffer_desc) {
                .type = SG_BUFFERTYPE_INDEXBUFFER,
                .data = SG_RANGE(indices)
            });

    gfx_load_shader(
        &gs->screenquad.shader,
        screenquad_screenquad_shader_desc,
        (gfx_shader_reload_f) on_screenquad_reload,
        gs);
    on_screenquad_reload(&gs->screenquad.shader, gs);

    gs->batch.vbuf = gs->screenquad.vbuf;
    gs->batch.ibuf = gs->screenquad.ibuf;

    gfx_load_shader(
        &gs->batch.shader,
        batch_batch_shader_desc,
        (gfx_shader_reload_f) on_batch_reload,
        gs);
    on_batch_reload(&gs->batch.shader, gs);
}

void gfx_state_destroy(gfx_state_t *gs) {
#ifdef RELOADABLE
    if (g_reload_host) {
        map_each(sg_shader*, shader_reload_t*, &gs->shader_reload_map, it) {
            g_reload_host->unhook_fn(it.value->hook_id);
            g_reload_host->del_fn((void**) &it.value->desc_fn);
            if (it.value->callback) {
                g_reload_host->del_fn((void**) &it.value->callback);
            }
            free(it.value);
        }
    }
#endif // ifdef RELOADABLE

    // TODO: dealloc buffers and pipelines
}

// cannot be static, must be visible to reload host
void gfx_reload_shader(shader_reload_t *sr) {
    sg_destroy_shader(*sr->shader);
    sg_dealloc_shader(*sr->shader);

    *sr->shader = sg_make_shader(sr->desc_fn(sg_query_backend()));

    if (sr->callback) {
        sr->callback(sr->shader, sr->userdata);
    }
}

void gfx_load_shader(
    sg_shader *shader,
    const sg_shader_desc *(desc_fn)(sg_backend),
    void (*callback)(sg_shader*, void*),
    void *userdata) {
    *shader = sg_make_shader(desc_fn(sg_query_backend()));

#ifdef RELOADABLE
    if (g_reload_host) {
        shader_reload_t *sr = malloc(sizeof(shader_reload_t));
        sr->desc_fn = desc_fn;
        sr->shader = shader;
        sr->hook_id =
            g_reload_host->hook_fn((rh_hook_callback_f) gfx_reload_shader, sr);
        g_reload_host->reg_fn((void**) &sr->desc_fn);

        sr->callback = callback;
        sr->userdata = userdata;

        if (sr->callback) {
            g_reload_host->reg_fn((void**) &sr->callback);
        }

        map_insert(
            &state->gfx_state->shader_reload_map,
            shader,
            sr);
    }
#endif // ifdef RELOADABLE
}

void gfx_unload_shader(sg_shader *shader) {
    sg_destroy_shader(*shader);

#ifdef RELOADABLE
    if (g_reload_host) {
        shader_reload_t **psr =
            map_find(
                shader_reload_t*,
                &state->gfx_state->shader_reload_map,
                shader);
        map_remove(&state->gfx_state->shader_reload_map, shader);

        if (!*psr) {
            WARN("unloading invalid shader @ %p", shader);
        }

        shader_reload_t *sr = *psr;
        g_reload_host->unhook_fn(sr->hook_id);
        g_reload_host->del_fn((void**) &sr->desc_fn);
        if (sr->callback) {
            g_reload_host->del_fn((void**) &sr->callback);
        }
        free(sr);
    }
#endif
}

static int load_image(const char *filepath, u8 **pdata, ivec2s *psize) {
    char *filedata;
    usize filesz;
    ASSERT(
        !file_read(filepath, &filedata, &filesz),
        "failed to read %s",
        filepath);

    SDL_RWops *rw = SDL_RWFromMem(filedata, filesz);
    ASSERT(rw);

    SDL_Surface *surf = IMG_LoadPNG_RW(rw);
    ASSERT(surf);
    ASSERT(surf->w >= 0 && surf->h >= 0);

    if (surf->format->format != SDL_PIXELFORMAT_RGBA32) {
        SDL_Surface *newsurf =
            SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surf);
        surf = newsurf;
    }

    *pdata = malloc(surf->w * surf->h * 4);
    *psize = (ivec2s) {{ surf->w, surf->h }};

    for (int y = 0; y < surf->h; y++) {
        memcpy(
            &(*pdata)[y * (psize->x * 4)],
            &((u8*) surf->pixels)[(surf->h - y - 1) * (psize->x * 4)],
            psize->x * 4);
    }

    SDL_FreeSurface(surf);
    SDL_FreeRW(rw);
    free(filedata);

    return 0;
}

int gfx_load_image(const char *resource, sg_image *out) {
    int res;
    u8 *data;
    ivec2s size;

    if ((res = load_image(resource, &data, &size))) {
        return res;
    }

    *out =
        sg_make_image(
            &(sg_image_desc) {
                .width = size.x,
                .height = size.y,
                .data.subimage[0][0] = {
                    .ptr = data,
                    .size = (size_t) (size.x * size.y * 4),
                }
            });
    free(data);

    return 0;
}

void gfx_screenquad(sg_image image, bool is_stencil) {
    const sg_bindings bind = {
        .fs_images[0] = image,
        .vertex_buffers[0] = state->gfx_state->screenquad.vbuf,
        .index_buffer = state->gfx_state->screenquad.ibuf
    };

    const mat4s
        model = glms_mat4_identity(),
        view = glms_mat4_identity(),
        proj = glms_ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);

    screenquad_vs_params_t vs_params;
    memcpy(&vs_params.model, &model, sizeof(model));
    memcpy(&vs_params.view, &view, sizeof(view));
    memcpy(&vs_params.proj, &proj, sizeof(proj));

    screenquad_fs_params_t fs_params;
    fs_params.is_stencil = is_stencil ? 1 : 0;

    sg_apply_pipeline(state->gfx_state->screenquad.pipeline);

    sg_apply_bindings(&bind);
    sg_apply_uniforms(
        SG_SHADERSTAGE_VS, SLOT_screenquad_vs_params, SG_RANGE_REF(vs_params));
    sg_apply_uniforms(
        SG_SHADERSTAGE_FS, SLOT_screenquad_fs_params, SG_RANGE_REF(fs_params));
    sg_draw(0, 6, 1);
}

void gfx_batcher_init(gfx_batcher_t *batcher) {
    *batcher = (gfx_batcher_t) { 0 };
    batcher->instance_data =
        sg_make_buffer(
            &(sg_buffer_desc) {
                .size = 64 * 1024 * 1024,
                .usage = SG_USAGE_STREAM
            });
}

void gfx_batcher_destroy(gfx_batcher_t *batcher) {
    dynlist_free(batcher->entries);
    sg_destroy_buffer(batcher->instance_data);
}

// push sprite to render from atlas
void gfx_batcher_push_sprite(
    gfx_batcher_t *batcher,
    const gfx_sprite_t *sprite) {
    atlas_lookup_t lookup;
    atlas_lookup(state->atlas, sprite->res, &lookup);
    // TODO ??
    /* const vec2s half_px = {{ */
    /*     (1.0f / atlas->size_px.x) / 8.0f, */
    /*     (1.0f / atlas->size_px.y) / 8.0f, */
    /* }}; */
    const gfx_batcher_entry_t entry = {
        .offset = sprite->pos,
        .scale =
            glms_vec2_mul(IVEC_TO_V(aabb_size(lookup.box_px)), sprite->scale),
        .uv_min = lookup.box_uv.min,
        .uv_max = lookup.box_uv.max,
        .layer = lookup.layer,
        .color = sprite->color,
        .z = sprite->z,
        .flags = sprite->flags
    };
    *dynlist_push(batcher->entries) = entry;
}

/* void gfx_batcher_push_image( */
/*     gfx_batcher_t *batcher, */
/*     sg_image image, */
/*     vec2s pos, */
/*     vec4s color, */
/*     f32 z, */
/*     int flags) { */
/*     sg_image_desc desc = sg_query_image_desc(image); */
/*     gfx_batcher_push_subimage( */
/*         batcher, image, pos, color, z, flags, */
/*         (ivec2s) {{ 0, 0 }}, */
/*         (ivec2s) {{ desc.width, desc.height }}); */
/* } */

/* void gfx_batcher_push_subimage( */
/*     gfx_batcher_t *batcher, */
/*     sg_image image, */
/*     vec2s pos, */
/*     vec4s color, */
/*     f32 z, */
/*     int flags, */
/*     ivec2s offset, */
/*     ivec2s size) { */
/*     sg_image_desc desc = sg_query_image_desc(image); */
/*     batcher_list_t *list = list_for_image(batcher, image); */
/*     const vec2s uv_unit = {{ */
/*         1.0f / desc.width, */
/*         1.0f / desc.height */
/*     }}; */
/*     *dynlist_push(list->entries) = (gfx_batcher_entry_t) { */
/*         .offset = pos, */
/*         .scale = {{ size.x, size.y }}, */
/*         .uv_min = {{ */
/*             offset.x * uv_unit.x, */
/*             offset.y * uv_unit.y, */
/*         }}, */
/*         .uv_max = {{ */
/*             (offset.x + (size.x - 0)) * uv_unit.x, */
/*             (offset.y + (size.y - 0)) * uv_unit.y, */
/*         }}, */
/*         .color = color, */
/*         .z = z, */
/*         .flags = flags */
/*     }; */
/* } */

void gfx_batcher_draw(
    gfx_batcher_t *batcher,
    const mat4s *proj,
    const mat4s *view) {
    if (dynlist_size(batcher->entries) == 0) { return; }

    sg_update_buffer(
        batcher->instance_data,
        &(sg_range) {
            .ptr = batcher->entries,
            .size = dynlist_size_bytes(batcher->entries)
        });
    ASSERT(!sg_query_buffer_overflow(batcher->instance_data));

    sg_apply_pipeline(state->gfx_state->batch.pipeline);

    const sg_bindings bind = {
        .fs_images[SLOT_batch_atlas] = state->atlas->image,
        .index_buffer = state->gfx_state->batch.ibuf,
        .vertex_buffers[0] = state->gfx_state->batch.vbuf,
        .vertex_buffers[1] = batcher->instance_data,
        .vertex_buffer_offsets[1] = 0,
    };

    const mat4s model = glms_mat4_identity();

    batch_vs_params_t vsparams;
    memcpy(vsparams.model, &model, sizeof(model));
    memcpy(vsparams.view, view, sizeof(*view));
    memcpy(vsparams.proj, proj, sizeof(*proj));

    sg_apply_bindings(&bind);
    sg_apply_uniforms(
        SG_SHADERSTAGE_VS, SLOT_batch_vs_params, SG_RANGE_REF(vsparams));
    sg_draw(0, 6, dynlist_size(batcher->entries));

    dynlist_free(batcher->entries);
}

void gfx_batcher_clear(gfx_batcher_t *batcher) {
    dynlist_free(batcher->entries);
}

void gfx_draw_debug(
    gfx_state_t *gs,
    const mat4s *proj,
    const mat4s *view) {
    sgl_defaults();
    sgl_load_pipeline(state->sgl_pipeline);
    sgl_point_size(4.0f);

    const mat4s view_proj = glms_mat4_mul(*proj, *view);
    sgl_load_matrix((float*) view_proj.raw);

    sgl_set_context(state->sgl_ctx);
    sgl_begin_lines();
    dynlist_each(gs->debug_lines, it) {
        sgl_v3f_c4f(SVEC3(it.el->a), SVEC4(it.el->color));
        sgl_v3f_c4f(SVEC3(it.el->b), SVEC4(it.el->color));
    }
    sgl_end();

    sgl_begin_points();
    dynlist_each(gs->debug_points, it) {
        sgl_v3f_c4f(SVEC3(it.el->p), SVEC4(it.el->color));
    }
    sgl_end();
    sgl_context_draw(state->sgl_ctx);
}

void gfx_finish(gfx_state_t *gs) {
    dynlist_each(gs->debug_lines, it) {
        if (it.el->frames == -1 || --it.el->frames <= 0) {
            dynlist_remove_it(gs->debug_lines, it);
        }
    }

    dynlist_each(gs->debug_points, it) {
        if (it.el->frames == -1 || --it.el->frames <= 0) {
            dynlist_remove_it(gs->debug_points, it);
        }
    }
}

void gfx_debug_line(const gfx_debug_line_t *line) {
    // no shame in a little global state :)
    *dynlist_push(state->gfx_state->debug_lines) = *line;
}

void gfx_debug_point(const gfx_debug_point_t *point) {
    *dynlist_push(state->gfx_state->debug_points) = *point;
}
