#ifndef UTIL_IMPL
#define UTIL_IMPL
#endif

#include "soloud_c.h"

#include "util/input.h"
#include "util/dynlist.h"
#include "util/map.h"
#include "util/file.h"
#include "util/dlist.h"
#include "util/llist.h"
#include "util/input.h"
#include "util/log.h"
#include "util/assert.h"
#include "util/rand.h"
#include "util/bitmap.h"
#include "util/image.h"
#include "util/sound.h"

#include "level/vertex.h"
#include "level/wall.h"
#include "level/side.h"
#include "level/sector.h"
#include "level/sectmat.h"
#include "level/sidemat.h"
#include "level/tag.h"
#include "level/lptr.h"
#include "level/block.h"
#include "level/particle.h"
#include "level/path.h"
#include "level/decal.h"

#include "editor/editor.h"

#include "gfx/opengl.h"
#include "gfx/sokol.h"
#include "gfx/atlas.h"
#include "gfx/palette.h"
#include "sdl2.h"
#include "imgui.h"

#include "gfx/gfx.h"
#include "gfx/renderer.h"
#include "state.h"
#include "config.h"
#include "reload.h"

#include "level/level.h"
#include "level/side.h"

static void sg_logger_func(
    UNUSED const char* tag,
    uint32_t log_level,
    UNUSED uint32_t log_item_id,
    const char* message_or_null,
    uint32_t line_nr,
    const char* filename_or_null,
    UNUSED void* user_data) {
    log_write(
        filename_or_null 
            ? filename_or_null 
            : "(sokol_gfx.h)",
        line_nr,
        "",
        log_level <= 2 
            ? stderr 
            : stdout,
        ((const char *[]) {
            "PAN",
            "ERR",
            "WRN",
            "LOG"
         })[log_level],
        "%s",
        message_or_null 
            ? message_or_null 
            : "(NULL)");
}

static void make_fbs() {
    sg_image *images[5] = {
        &state->fbs.primary.color,
        &state->fbs.primary.depth_stencil,
        &state->fbs.overlay.color,
        &state->fbs.overlay.depth,
        &state->fbs.editor.color,
    };

    for (int i = 0; i < (int) ARRLEN(images); i++) {
        if (images[i]->id) {
            sg_destroy_image(*images[i]);
            images[i]->id = SG_INVALID_ID;
        }
    }

    state->fbs.primary.color = sg_make_image(
            &(sg_image_desc) {
                .render_target = true,
                .width = TARGET_3D_WIDTH,
                .height = TARGET_3D_HEIGHT,
                .pixel_format = RENDERER_PIXELFORMAT_COLOR,
                .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST, });

    state->fbs.primary.info = sg_make_image(
            &(sg_image_desc) {
                .render_target = true,
                .width = TARGET_3D_WIDTH,
                .height = TARGET_3D_HEIGHT,
                .pixel_format = RENDERER_PIXELFORMAT_INFO,
                .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST, });

    state->fbs.primary.depth_stencil = sg_make_image(
            &(sg_image_desc) {
                .render_target = true,
                .width = TARGET_3D_WIDTH,
                .height = TARGET_3D_HEIGHT,
                .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
                .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST, });

    state->fbs.primary.pass = sg_make_pass(
            &(sg_pass_desc) {
                .color_attachments[0].image = state->fbs.primary.color,
                .color_attachments[1].image = state->fbs.primary.info,
                .depth_stencil_attachment.image = state->fbs.primary.depth_stencil, });

    state->fbs.primary.pass_action = (sg_pass_action) {
            .colors[0] = { .action = SG_ACTION_CLEAR, .value = { 0.2f, 0.3f, 0.3f, 1.0f } },
            .colors[1] = { .action = SG_ACTION_CLEAR, .value = { 0.0f, 0.0f, 0.0f, 1.0f }, },
            .depth = { .action = SG_ACTION_CLEAR, .value = 100.0f },
            .stencil = { .action = SG_ACTION_CLEAR, .value = 0 } };

    state->fbs.overlay.color = sg_make_image(
            &(sg_image_desc) {
                .render_target = true,
                .width = TARGET_WIDTH,
                .height = TARGET_HEIGHT,
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST, });

    state->fbs.overlay.depth = sg_make_image(
            &(sg_image_desc) {
                .render_target = true,
                .width = TARGET_WIDTH,
                .height = TARGET_HEIGHT,
                .pixel_format = SG_PIXELFORMAT_DEPTH,
                .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST, });

    state->fbs.overlay.pass = sg_make_pass(
            &(sg_pass_desc) {
                .color_attachments[0].image = state->fbs.overlay.color,
                .depth_stencil_attachment.image = state->fbs.overlay.depth });

    state->fbs.overlay.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .value = { 0.0f, 0.0f, 0.0f, 0.0f } },
        .depth = { .action = SG_ACTION_CLEAR, .value = 100.0f, } };

    state->fbs.editor.color = sg_make_image(
            &(sg_image_desc) {
                .render_target = true,
                .width = state->window_size.x,
                .height = state->window_size.y,
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST, });

    state->fbs.editor.depth = sg_make_image(
            &(sg_image_desc) {
                .render_target = true,
                .width = state->window_size.x,
                .height = state->window_size.y,
                .pixel_format = SG_PIXELFORMAT_DEPTH,
                .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST, });

    state->fbs.editor.pass = sg_make_pass(
            &(sg_pass_desc) {
                .color_attachments[0].image = state->fbs.editor.color,
                .depth_stencil_attachment.image = state->fbs.editor.depth, });

    state->fbs.editor.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .value = { 0.0f, 0.0f, 0.0f, 0.0f } },
        .depth = { .action = SG_ACTION_CLEAR, .value = 1000.0f }
    };
}

static void init() {
    state->allow_input = true;
    state->rand = rand_create(0xDEADBEEF);
    state->gun_mode = 1;

    ASSERT(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO), "failed to init SDL: %s", SDL_GetError());

    ASSERT(IMG_Init(IMG_INIT_PNG) == IMG_INIT_PNG, "failed to init SDL_Image, %s", IMG_GetError());

    SDL_version compiled;
    SDL_version linked;
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
 
    state->window = SDL_CreateWindow(
            "WINDOW",
            SDL_WINDOWPOS_CENTERED_DISPLAY(0),
            SDL_WINDOWPOS_CENTERED_DISPLAY(0),
            1280, 720,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    ASSERT(state->window);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    state->gl_ctx = SDL_GL_CreateContext(state->window);
    ASSERT(state->gl_ctx);

    SDL_GL_MakeCurrent(state->window, state->gl_ctx);
    SDL_GL_SetSwapInterval(0);

    state->imgui_ctx = igCreateContext(NULL);
    igSetCurrentContext(state->imgui_ctx);

    state->input = malloc(sizeof(*state->input));
    input_init(state->input);

    state->sg_state = sg_create_state();
    sg_set_state(state->sg_state);

    state->sgp_state = sgp_create_state();
    sgp_set_state(state->sgp_state);

    state->simgui_state = simgui_create_state();
    simgui_set_state(state->simgui_state);

    state->sgl_state = sgl_create_state();
    sgl_set_state(state->sgl_state);

    sg_setup(&(sg_desc) { .logger = (sg_logger) { .func = sg_logger_func } });

    sgp_setup(&(sgp_desc) { .max_vertices = 65536 * 8 });

    simgui_setup(&(simgui_desc_t){ 0 });

    sg_imgui_init(&state->sg_imgui_state, &(sg_imgui_desc_t) { 0 });

    sgl_setup(&(sgl_desc_t) { .logger.func = sg_logger_func });
    state->sgl_ctx =
        sgl_make_context(
            &(sgl_context_desc_t) {
                .max_vertices = 65536,
                .max_commands = 16384,
                .color_format = SG_PIXELFORMAT_RGBA8,
                .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
                .sample_count = 1, });

    state->sgl_pipeline = sgl_context_make_pipeline(
        state->sgl_ctx,
        &(sg_pipeline_desc) {
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
                    } },
                [1] = {
                    .pixel_format = RENDERER_PIXELFORMAT_INFO,
                    .blend = { .enabled = false } }
            },
            .color_count = 2,
            .depth = {
                .write_enabled = true,
                .compare = SG_COMPAREFUNC_LESS_EQUAL,
                .pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
            } });

    SDL_GL_GetDrawableSize(
        state->window, &state->window_size.x, &state->window_size.y);
    make_fbs();

    state->gfx_state = malloc(sizeof(*state->gfx_state));
    gfx_state_init(state->gfx_state);

    state->renderer = malloc(sizeof(*state->renderer));
    renderer_init(state->renderer);

    state->atlas = malloc(sizeof(*state->atlas));
    atlas_init(state->atlas);
    atlas_load_all(state->atlas);

    state->palette = malloc(sizeof(*state->palette));
    palette_init(state->palette);
    palette_load_gpl(state->palette, "res/doompal.gpl");

    gfx_batcher_init(&state->batcher);

    sound_init();

    ASSERT(!state_new_level(state));
    state->level = malloc(sizeof(*state->level));
    level_init(state->level);

    state->editor = malloc(sizeof(*state->editor));
    editor_init(state->editor);
    state->mode = GAMEMODE_EDITOR;
}

static void deinit() {
    sound_destroy();

    gfx_batcher_destroy(&state->batcher);

    palette_destroy(state->palette);
    free(state->palette);

    atlas_destroy(state->atlas);
    free(state->atlas);

    renderer_destroy(state->renderer);
    free(state->renderer);

    gfx_state_destroy(state->gfx_state);
    free(state->gfx_state);

    SDL_GL_DeleteContext(state->gl_ctx);
    SDL_DestroyWindow(state->window);
    SDL_Quit();
    sg_shutdown();
    simgui_destroy_state(state->simgui_state);
    sgl_destroy_state(state->sgl_state);
    sgp_destroy_state(state->sgp_state);
    sg_destroy_state(state->sg_state);
    free(state);
}

typedef struct {
    path_hit_t hit;
} resolve_bullet_data_t;

static int resolve_bullet(
    level_t *level,
    const path_hit_t *hit,
    vec3s *from,
    vec3s *to,
    void *) {
    
    if (hit->type & T_OBJECT) {
        if (hit->object.ptr->type_index == OT_PLAYER) {
            return PATH_TRACE_CONTINUE;
        }
        
        for (int i = 0, n = rand_n(&state->rand, 20, 50); i < n; i++) {
            particle_t *particle = particle_new(level, hit->object.ptr->pos);
            if (!particle) {
                continue;
            }

            particle->type = PARTICLE_TYPE_FLARE;
            particle->z = hit->object.ptr->z + 0.5f;
            particle->vel_xyz =
                glms_vec3_scale(
                        rand_v3_hemisphere(&state->rand, VEC3(0, 0, 1)),
                        rand_f32(&state->rand, 8.0f, 16.0f));
            particle->ticks = TICKS_PER_SECOND;
            particle->flare.color = VEC4(VEC3(0.0), 1.0f);
        }
        return PATH_TRACE_STOP;
    }

    // particle spawn info
    vec3s part_pos;
    vec3s part_normal;

    if (hit->type & T_WALL) {
        bool collide = true;

        side_t * portal = hit->wall.side->portal;
        LOG("hit a wall %d", hit->wall.side->index);

        if (portal && portal->sector) {
            side_segment_t segs[4];
            side_get_segments(hit->wall.side, segs);
            ASSERT(segs[SIDE_SEGMENT_MIDDLE].present);

            const f32 z_t = lerp(from->z, to->z, hit->t);
            if (z_t > segs[SIDE_SEGMENT_MIDDLE].z0
                && z_t < segs[SIDE_SEGMENT_MIDDLE].z1) {
                collide = false;

                int res = PATH_TRACE_CONTINUE;
                f32 portal_angle = 0.0f;
                if (path_trace_3d_resolve_portal(
                        level, hit, from, to, &res, &portal_angle,
                        PATH_TRACE_RESOLVE_PORTAL_NONE)) {
                    return res;
                }
            }
        }

        // no further collision
        if (!collide) {
            return PATH_TRACE_CONTINUE;
        }

        LOG("hit side %d @ %f", hit->wall.side->index, hit->wall.u);

        vertex_t *vs[2];
        side_get_vertices(hit->wall.side, vs);

        decal_t *decal = decal_new(state->level);

        decal->type = DT_PLACEHOLDER;
        decal->tex_base = AS_RESOURCE("XHOLE");

        decal_set_side(level, decal, hit->wall.side);

        decal->side.offsets =
            VEC2(
                hit->wall.x,
                lerp(from->z, to->z, hit->t)
                    - hit->wall.side->sector->floor.z);

        decal->ticks = 240;

        part_normal = 
            VEC3(
                side_normal(hit->wall.side), 
                0.0f);
        part_pos =
            glms_vec3_add(
                VEC3(hit->swept_pos, lerp(from->z, to->z, hit->t)),
                glms_vec3_scale(part_normal, 0.001f));

        gfx_debug_point(
            &(gfx_debug_point_t) {
                .p = VEC3(hit->swept_pos, lerp(from->z, to->z, hit->t)),
                .color = VEC4(rand_v3(&state->rand, VEC3(0), VEC3(1)), 1.0),
                .frames = 5000 });
    } else {
        LOG("hit sector %d at %" PRIv2, hit->sector.ptr->index, FMTv2(hit->swept_pos));
        decal_t *decal = decal_new(state->level);
        decal->type = DT_PLACEHOLDER;
        decal->tex_base = AS_RESOURCE("XHOLE");

        decal_set_sector(
            state->level, decal, hit->sector.ptr, hit->sector.plane);

        decal->sector.pos = hit->swept_pos;
        decal->ticks = 240;

        decal_recalculate(level, decal);

        const f32 plane_sgn = hit->sector.plane == PLANE_TYPE_FLOOR ? 1 : -1;
        part_pos =
            VEC3(
                hit->swept_pos,
                hit->sector.ptr->planes[hit->sector.plane].z
                    + 0.001f * plane_sgn);

        part_normal = 
            VEC3(0, 0, plane_sgn);
    } 

    for (int i = 0, n = rand_n(&state->rand, 5, 10); i < n; i++) {
        particle_t *particle = particle_new(level, glms_vec2(part_pos));
        if (!particle) {
            continue;
        }

        particle->type = PARTICLE_TYPE_FLARE;
        particle->z = part_pos.z;
        particle->vel_xyz =
            glms_vec3_scale(
                    rand_v3_hemisphere(&state->rand, part_normal),
                    rand_f32(&state->rand, 8.0f, 16.0f));
        particle->ticks = TICKS_PER_SECOND + rand_n(&state->rand, -15, 15);
        particle->flare.color = VEC4(VEC3(0.5), 1.0f);
    }

    return PATH_TRACE_STOP;    
}

static void do_bullet(object_t *player) {
    DYNLIST(sector_t*) near_sectors = NULL;
    level_get_visible_sectors(
        state->level, player->sector, &near_sectors,
        LEVEL_GET_VISIBLE_SECTORS_PORTALS);

    dynlist_each(near_sectors, it) {
        (*it.el)->flags |= SECTOR_FLAG_FLASH;
    }

    dynlist_free(near_sectors);

    sound_play(AS_RESOURCE("stest0"));

    const vec3s dir = state->cam.dir;

    vec3s
        from = state->cam.pos,
        to = glms_vec3_add(state->cam.pos, glms_vec3_scale(dir, 100.0f));

    gfx_debug_line(
        &(gfx_debug_line_t) {
            .a = from,
            .b = to,
            .color = VEC4(rand_v3(&state->rand, VEC3(0), VEC3(1)), 1.0),
            .frames = 5000
        });

    path_trace_3d(
        state->level,
        &from, &to,
        0.0f,
        (path_trace_3d_resolve_f) resolve_bullet,
        NULL,
        PATH_TRACE_ADD_OBJECTS);
}

static void do_ui() {
    object_t *player = NULL;
    level_dynlist_each(state->level->objects, it) {
        if ((*it.el)->type_index == OT_PLAYER) {
            player = *it.el;
            break;
        }
    }

    if (!player) { 
        return; 
    }


    const int since_shot = state->time.tick - state->last_shot;
    int frame;

    const int frame_ticks = 5, n_frames = 3;
    if (since_shot >= frame_ticks * (n_frames + 1)) {
        frame = 0;
    } else if (since_shot >= frame_ticks * n_frames) {
        frame = 2;
    } else {
        frame = 1 + (since_shot / frame_ticks) % 3;
    }

    resource_t resource;
    atlas_lookup_t lookup;
    ivec2s gun_pos;

    // LOG("STATE->GUN_MODE: %d", state->gun_mode);
    switch (state->gun_mode) {
    case 1:
        resource = resource_from("HGUN2");
        atlas_lookup(state->atlas, resource, &lookup);
        gun_pos = IVEC2(150, 0);
        break;
    case 2:
        resource = resource_from("HGUN4%c", (char) ('A' + frame));
        atlas_lookup(state->atlas, resource, &lookup);
        gun_pos = IVEC2(200, -20);
        break;
    }

    const f32 lv = glms_vec2_norm(player->vel);
    state->swing += (32.0f * lv) * state->time.dt;

    // move towards swing % period = 0.0f
    const f32 period = 250.0f;
    const f32 mult = 
        roundf(
            state->swing 
            / (period / 2.0f)) 
            * (period / 2.0f);
    
    if (fabsf(lv) < 1.0f) {
        state->swing = 
            lerp(
                state->swing,
                lerp(state->swing, mult, 0.1f),
                (1.0f - lv) / 1.0f);
    }

    const bool is_aim =
        input_get(state->input, "mouse_right|left shift") & INPUT_DOWN;
    state->aim_time =
        max(state->aim_time 
            + (is_aim 
                ? state->time.dt 
                : -state->time.dt), 
            0.0f);

    if (!is_aim && state->aim_factor < 0.01f) {
        state->aim_time = 0.0f;
        state->cbump = 0.0f;
    }

    state->aim_factor =
        clamp(
            state->aim_factor 
            + (state->time.dt 
                * (is_aim ? 1 : -1)), 
            0.0f, 0.2f);

    const f32 powx = lerp(1.0f, 0.15f, state->aim_factor / 0.2f);
    const f32 powy = lerp(1.0f, 0.8f, state->aim_factor / 0.2f);

    const f32 a = TAU * (state->swing / period);
    gun_pos.x += (powx * 20) * cosf(PI_2 + a);
    gun_pos.y -= (powy * 8) * fabsf(sinf(a - PI_2));

    gun_pos.x -= 8 * (state->aim_factor / 0.2f);
    gun_pos.y += 16 * (state->aim_factor / 0.2f);

    if (input_get(state->input, "mouse_left") & INPUT_PRESS) {
        state->last_shot = state->time.tick;
        state->cbump += lerp(0.2f, 0.05f, max(state->cbump - 1.0f, 0.0f));
        state->last_cbump = state->time.tick;

        rand_t r = rand_create(state->time.tick);
        const vec3s pos =
            glms_vec3_add(
                VEC3(player->pos, player->z + 1.05f),
                glms_vec3_scale(state->cam.dir, 1.1f));

        particle_t *particle = particle_new(state->level, glms_vec2(pos));
        if (particle) {
            particle->type = PARTICLE_TYPE_FLARE;
            particle->z = pos.z;

            const f32 angle =
                player->angle + (rand_sign(&r) * (rand_f32(&r, 1 * PI_6, 4 * PI_6)));
            particle->vel_xyz =
                VEC3(
                    cosf(angle) * rand_f32(&r, 10.0f, 16.0f),
                    -sinf(angle) * rand_f32(&r, 10.0f, 16.0f),
                    rand_f32(&r, 0.0f, 0.5f));
            particle->ticks = TICKS_PER_SECOND;
            particle->flare.color = VEC4(VEC3(0.2), 1.0);
        }

        do_bullet(player);
    }

    const f32 q =
        lerp(
            0.0f,
            state->time.dt,
            clamp((state->time.tick - state->last_cbump) / 20.0f, 0.0f, 1.0f));

    state->cbump = max(state->cbump - q, 0.0f);

    if (frame == 1) {
        gfx_batcher_push_sprite(
            &state->batcher,
            &(gfx_sprite_t) {
                .res = AS_RESOURCE("HFLASH0"),
                // TODO: this is only possible for 2 guns lol
                .pos = state->gun_mode == 2 
                    ? IVEC_TO_V(glms_ivec2_add(gun_pos, IVEC2(4, 92))) 
                    : IVEC_TO_V(glms_ivec2_add(gun_pos, IVEC2(53, 50))) ,
                .scale = VEC2(1),
                .color = VEC4(1),
                .z = 0.4f,
                .flags = GFX_NO_FLAGS });
    }

    u8 light = sector_light(state->level, player->sector);

    if (frame == 1) {
        light = LIGHT_EXTRA;
    }

    gfx_batcher_push_sprite(
        &state->batcher,
        &(gfx_sprite_t) {
            .res = resource,
            .pos = IVEC_TO_V(gun_pos),
            .scale = VEC2(1),
            .color = VEC4(VEC3(light / (f32) LIGHT_MAX), 1),
            .z = 0.4f,
            .flags = GFX_NO_FLAGS });
}

static void frame() {
    SDL_GL_GetDrawableSize(state->window, &state->window_size.x, &state->window_size.y);

    simgui_new_frame(
        &(simgui_frame_desc_t) {
            .width = state->window_size.x,
            .height = state->window_size.y,
            .delta_time = 0.0016f,
            .dpi_scale = 1.0f });

    state->input->cursor.grab = state->mouse_grab;

    {   // time managment
        state->time.frame++;

        const u64 now = time_ns();
        state->time.now = now;
        state->time.delta = state->time.frame_start == 0 ? 16000 : (now - state->time.frame_start);
        // running average of last 60 frames
        state->time.frame_avg = (state->time.frame_avg * (59.0 / 60.0)) + (state->time.delta * (1.0 / 60.0));
        state->time.frame_start = now;

        if (now - state->time.last_second > 1000000000) {
            state->time.tps = state->time.second_ticks;
            state->time.second_ticks = 0;
            state->time.fps = state->time.second_frames;
            state->time.second_frames = 0;
            state->time.last_second = now;
        }

        const u64 tick_time = state->time.delta + state->time.tick_remainder;
        state->time.frame_ticks = min(tick_time / NS_PER_TICK, TICKS_PER_SECOND);
        state->time.tick_remainder = tick_time % NS_PER_TICK;

        state->time.second_ticks += state->time.frame_ticks;
        state->time.second_frames++;

        state->time.dt = (f32) (state->time.delta / 1000000000.0);
    }

    input_update(state->input);

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_MOUSEMOTION:
            igGetIO()->MousePos = (ImVec2) { ev.motion.x, ev.motion.y };
            break;
        case SDL_MOUSEBUTTONDOWN:
            igGetIO()->MousePos = (ImVec2) { ev.motion.x, ev.motion.y };
            break;
        case SDL_QUIT:
            state->quit = true;
            break;
        }

        imgui_impl_process_sdl_event(&ev);
        input_process(state->input, &ev);
    }

    // do debug UI
    if (input_get(state->input, "F9") & INPUT_PRESS) {
        state->sg_imgui_show = !state->sg_imgui_show;
    }

    if (input_get(state->input, "F10") & INPUT_PRESS) {
        state->renderer->debug_ui = !state->renderer->debug_ui;
    }

    if (state->sg_imgui_show
        && igBegin("SOKOL", &state->sg_imgui_show, 0)) {
        if (igTreeNode_Str("BUFFERS")) {
            sg_imgui_draw_buffers_content(&state->sg_imgui_state);
            igTreePop();
        }

        if (igTreeNode_Str("IMAGES")) {
            sg_imgui_draw_images_content(&state->sg_imgui_state);
            igTreePop();
        }

        if (igTreeNode_Str("SHADERS")) {
            sg_imgui_draw_shaders_content(&state->sg_imgui_state);
            igTreePop();
        }

        if (igTreeNode_Str("PIPELINES")) {
            sg_imgui_draw_pipelines_content(&state->sg_imgui_state);
            igTreePop();
        }

        if (igTreeNode_Str("PASSES")) {
            sg_imgui_draw_passes_content(&state->sg_imgui_state);
            igTreePop();
        }

        if (igTreeNode_Str("CAPTURE")) {
            sg_imgui_draw_capture_content(&state->sg_imgui_state);
            igTreePop();
        }

        if (igTreeNode_Str("CAPABILITIES")) {
            sg_imgui_draw_capabilities_content(&state->sg_imgui_state);
            igTreePop();
        }

        igEnd();
    }

    state->cam.dir = VEC3(
            cosf(state->cam.pitch) * cosf(state->cam.yaw),
            cosf(state->cam.pitch) * sinf(-state->cam.yaw),
            sinf(state->cam.pitch));
    state->cam.dir = glms_vec3_normalize(state->cam.dir);
    state->cam.up = VEC3(0, 0, 1);
    state->cam.right = glms_vec3_cross(state->cam.dir, state->cam.up);
    state->cam.sector = NULL;

    if (state->mode == GAMEMODE_EDITOR
        && state->editor->mode == EDITOR_MODE_CAM
        && state->editor->mousegrab) {
        const f32 speed = 18.0f;

        if (input_get(state->input, "w|up") & INPUT_DOWN) {
            state->cam.pos = glms_vec3_add(
                    state->cam.pos,
                    glms_vec3_scale(state->cam.dir, state->time.dt * speed));
        }

        if (input_get(state->input, "s|down") & INPUT_DOWN) {
            state->cam.pos = glms_vec3_add(
                    state->cam.pos,
                    glms_vec3_scale(state->cam.dir, -state->time.dt * speed));
        }

        if (input_get(state->input, "d|right") & INPUT_DOWN) {
            state->cam.pos = glms_vec3_add(
                    state->cam.pos,
                    glms_vec3_scale(state->cam.right, state->time.dt * speed));
        }

        if (input_get(state->input, "a|left") & INPUT_DOWN) {
            state->cam.pos = glms_vec3_add(
                    state->cam.pos,
                    glms_vec3_scale(state->cam.right, -state->time.dt * speed));
        }

        if (input_get(state->input, "space") & INPUT_DOWN) {
            state->cam.pos =
                glms_vec3_add(
                    state->cam.pos,
                    glms_vec3_scale(state->cam.up, state->time.dt * speed));
        }

        if (input_get(state->input, "left shift") & INPUT_DOWN) {
            state->cam.pos = glms_vec3_add(
                    state->cam.pos,
                    glms_vec3_scale(state->cam.up, -state->time.dt * speed));
        }

        state->cam.yaw += state->input->cursor.motion_raw.x / 200.0f;
        state->cam.pitch += state->input->cursor.motion_raw.y / 200.0f;
    }

    state->cam.pitch = clamp(state->cam.pitch, -PI_2 + 0.05f, PI_2 - 0.05f);
    state->cam.yaw = wrap_angle(state->cam.yaw);

    state->cam.view = glms_lookat(
            state->cam.pos,
            glms_vec3_add(state->cam.dir, state->cam.pos),
            state->cam.up);
    state->cam.proj = glms_perspective(
            deg2rad(80.0f),
            state->window_size.x / (f32) state->window_size.y,
            0.01f,
            100.0f);

    // TODO: elsewhere
    if (state->allow_input) {
        if (state->mode == GAMEMODE_GAME
            && (input_get(state->input, "1") & INPUT_PRESS)) {
            state->mouse_grab = !state->mouse_grab;
            // state->gun_mode = 0;
        }

        if (state->mode == GAMEMODE_GAME
            && (input_get(state->input, "2") & INPUT_PRESS)) {
            if (state->gun_mode < 2) { // 2 is the amaount of weapons in the game
                state->gun_mode++;
            } else {
                state->gun_mode = 1;
            }
        }

        if (input_get(state->input, "F3") & INPUT_PRESS) {
            if (state->mode == GAMEMODE_EDITOR
                && state->level->version != state->editor->savedversion) {
                const int res = editor_save_level(state->editor, state->editor->levelpath);

                if (res) WARN("error saving level to %s: %d", state->editor->levelpath, res);
            }
            
            state_set_mode(state, state->mode == GAMEMODE_EDITOR ? GAMEMODE_GAME : GAMEMODE_EDITOR);
        }
    }

    atlas_update(state->atlas);
    level_update(state->level, state->time.dt);

    for (int i = 0; i < (int) state->time.frame_ticks; i++) {
        level_tick(state->level);
        state->time.tick++;
    }

    // render render bla bla
    
    sg_begin_pass(state->fbs.primary.pass, &state->fbs.primary.pass_action);
    sg_push_debug_group("3D");
    {
        state->renderer->cam.pos = state->cam.pos;
        state->renderer->cam.pitch = state->cam.pitch;
        state->renderer->cam.yaw = state->cam.yaw;
        renderer_render(state->renderer);

        gfx_draw_debug(state->gfx_state, &state->cam.proj, &state->cam.view);
    }
    sg_pop_debug_group();
    sg_end_pass();
    sg_commit();

    sg_begin_pass(state->fbs.editor.pass, &state->fbs.editor.pass_action);
    sg_push_debug_group("EDITOR");
    {
        sgp_begin(SIVEC2(state->window_size));
        if (state->mode == GAMEMODE_EDITOR) {
            editor_frame(state->editor, state->level);
        }

        state->sg_imgui_state.capture.open = true;
        sgp_flush();
        sgp_end();
    }
    sg_pop_debug_group();
    sg_end_pass();
    sg_commit();

    // more render

    sg_begin_pass(state->fbs.overlay.pass, &state->fbs.overlay.pass_action);
    sg_push_debug_group("2D");
    {
        sg_apply_scissor_rect(0, 0, TARGET_WIDTH, TARGET_HEIGHT, false);
        do_ui(); 

        const mat4s view = glms_mat4_identity();
        const mat4s proj = glms_ortho(0.0f, TARGET_WIDTH, 0.0f, TARGET_HEIGHT, -1.0f, 1.0f);
        gfx_batcher_draw(&state->batcher, &proj, &view);

        sgp_begin(TARGET_WIDTH, TARGET_HEIGHT);
        sgp_viewport(0, 0, TARGET_WIDTH, TARGET_HEIGHT);
        sgp_project(0.0f, TARGET_WIDTH, TARGET_HEIGHT, 0.0f);
        sgp_set_blend_mode(SGP_BLENDMODE_BLEND);

        sgp_set_color(1, 0, 1, 1);

        sgp_flush();
        sgp_end();
    }
    sg_pop_debug_group();
    sg_end_pass();
    sg_commit();

    // moreee render

    sg_begin_default_pass(
        &(sg_pass_action) { .colors[0] = { .action = SG_ACTION_CLEAR, .value = { 0.0f, 0.0f, 0.0f, 1.0f } } }, 
        SIVEC2(state->window_size));
    sg_push_debug_group("COMPOSITE");
    {
        gfx_screenquad(state->fbs.primary.color, false);
        gfx_screenquad(state->fbs.overlay.color, false);
        gfx_screenquad(state->fbs.editor.color, false);
        sg_pop_debug_group();

        igSetCurrentContext(state->imgui_ctx);
        simgui_render();
    }
    sg_end_pass();
    sg_commit();


    // finally
    gfx_finish(state->gfx_state);
    SDL_GL_SwapWindow(state->window);
}

int main() {
    state = calloc(1, sizeof(*state));
    init();
    while (!state->quit) frame();
    deinit();

    return 0;
}
