#include "state.h"
#include "gfx/atlas.h"
#include "gfx/renderer.h"
#include "level/level.h"
#include "level/io.h"
#include "editor/editor.h"
#include "level/object.h"
#include "util/file.h"

// global state
state_t *state;

int state_set_mode(state_t *state, game_mode mode) {
    if (mode == state->mode) { return 0; }

    // reload level
    int res;

    // TODO: proper reset on failure
    state->mode = mode;

    // reload level if it does not already exist
    if (state->level->version != 0) {
        if ((res = state_load_level(state, state->level_path))) {
            return 0x10000000 | res;
        }
    }

    return 0;
}

int state_new_level(state_t *state) {
    if (!state->level) {
        state->level = malloc(sizeof(*state->level));
    } else if (state->level->version != 0) {
        if (state->renderer) {
            renderer_destroy_for_level(state->renderer);
        }

        level_destroy(state->level);
    }

    memset(state->level, 0, sizeof(*state->level));
    level_init(state->level);

    if (state->renderer) {
        renderer_set_level(state->renderer, state->level);
    }

    return 0;
}

int state_load_level(state_t *state, const char *path) {
    int res;

    res = state_new_level(state);
    if (res) { return res; }
    if (!path) { return 0; }

    // make backup if opening for editor
    if (state->mode == GAMEMODE_EDITOR) {
        if ((res = file_makebak(path, ".bak"))) {
            WARN("error making backup: %d", res);
        }
    }

    struct { char *ptr; usize len; } data;
    if ((res = file_read(path, &data.ptr, &data.len))) {
        return 1;
    }

    if ((res = io_load_level(state->level, (const u8*) data.ptr, data.len))
            != IO_OK) {
        free(data.ptr);
        return 0x80000000 | res;
    }

    free(data.ptr);

    snprintf(
        state->level_path,
        sizeof(state->level_path),
        "%s",
        path);

#ifdef MAPEDITOR
    snprintf(
        state->editor->levelpath,
        sizeof(state->editor->levelpath),
        "%s",
        path);
#endif // ifdef MAPEDITOR

    LOG(
        "loaded %d sectors with %d walls and %d sides",
        dynlist_size(state->level->sectors),
        dynlist_size(state->level->walls),
        dynlist_size(state->level->sides));

    // spawn "player" if in GAME mode
    if (state->mode == GAMEMODE_GAME) {
        object_t *player = object_new(state->level), *spawn = NULL;
        object_set_type(state->level, player, OT_PLAYER);

        // TODO: level should have some object search function
        // find spawnpoint
        level_dynlist_each(state->level->objects, it) {
            object_t *o = *it.el;
            if (o->type_index == OT_SPAWN) {
                spawn = o;
                break;
            }
        }

        vec2s spawn_pos;

        if (spawn) {
            spawn_pos = spawn->pos;
        } else {
            WARN("no OT_SPAWN found in level, spawning at (2, 2)");
            spawn_pos = VEC2(2);
        }

        object_move(state->level, player, spawn_pos);
    }

    return 0;
}
