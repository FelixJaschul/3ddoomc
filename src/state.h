#pragma once

#include "util/aabb.h"
#include "util/math.h"
#include "gfx/gfx.h"
#include "gfx/sokol.h"
#include "defs.h"
#include "util/rand.h"
#include "util/sound.h"

typedef struct ImGuiContext ImGuiContext;
typedef struct gfx_state gfx_state_t;

typedef struct state {
    struct SDL_Window *window;
    struct SDL_GLContext *gl_ctx;
    ImGuiContext *imgui_ctx;

    ivec2s window_size;

    struct {
        vec3s pos, dir, up, right;
        f32 pitch, yaw;
        mat4s view, proj;
        sector_t *sector;
    } cam;

    struct {
        u64 frame_start, last_second;
        u64 tick_remainder;
        u64 frame_ticks;
        u64 delta;
        u64 second_ticks, tps;
        u64 second_frames, fps;
        u64 frame_avg;
        u64 tick;
        u64 frame;
        u64 now;
        f32 dt;
    } time;

    struct {
        struct {
            sg_image color, info, depth_stencil;
            sg_pass_action pass_action;
            sg_pass pass;
        } primary;

        struct {
            sg_image color, depth;
            sg_pass_action pass_action;
            sg_pass pass;
        } overlay;

        struct {
            sg_image color, depth;
            sg_pass_action pass_action;
            sg_pass pass;
        } editor;
    } fbs;

    bool quit;
    int gun_mode; 
    input_t *input;

    editor_t *editor;
    level_t *level;
    renderer_t *renderer;
    atlas_t *atlas;
    gfx_state_t *gfx_state;
    palette_t *palette;
    sound_state_t *sound;
    sound_id_t sound_id;

    bool is_walking;

    game_mode mode;

    bool mouse_grab;
    bool mouse_look;
    bool allow_input;
    
    sg_imgui_t sg_imgui_state;
    bool sg_imgui_show;

    void *sg_state, *sgp_state, *simgui_state, *sgl_state;

    sgl_context sgl_ctx;
    sgl_pipeline sgl_pipeline;

    char level_path[1024];

    gfx_batcher_t batcher;

    rand_t rand;

    f32 aim_factor, aim_time;
    f32 swing;
    int last_cbump;
    int last_shot;
    f32 cbump;
} state_t;

// global state storage
extern state_t *state;

int state_set_mode(state_t *state, game_mode mode);

// create new empty level on state
int state_new_level(state_t *state);

// TODO
int state_load_level(state_t *state, const char *path);
