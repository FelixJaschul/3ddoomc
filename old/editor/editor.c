#include "editor/editor.h"
#include "editor/map.h"
#include "editor/ui.h"
#include "editor/cursor.h"
#include "gfx/renderer.h"
#include "gfx/renderer_types.h"
#include "level/level.h"
#include "level/side.h"
#include "level/io.h"
#include "util/file.h"
#include "util/ini.h"
#include "util/input.h"
#include "util/sort.h"
#include "state.h"

ENUM_DEFN_FUNCTIONS(visopt, VISOPT, ENUM_VISOPT)

// see editor.h
const char *BUTTON_KEYS[BUTTON_COUNT] = {
    [BUTTON_NONE]          = NULL,
    [BUTTON_UP]            = "up",
    [BUTTON_DOWN]          = "down",
    [BUTTON_LEFT]          = "left",
    [BUTTON_RIGHT]         = "right",
    [BUTTON_EDIT]          = "mouse_right",
    [BUTTON_SELECT]        = "mouse_left",
    [BUTTON_DESELECT]      = "mouse_right",
    [BUTTON_ZOOM_IN]       = "]",
    [BUTTON_ZOOM_OUT]      = "[",
    [BUTTON_SNAP]          = "q",
    [BUTTON_SNAP_WALL]     = "z",
    [BUTTON_MULTI_SELECT]  = "left shift",
    [BUTTON_CANCEL]        = "escape|left ctrl+c",
    [BUTTON_MOVE_DRAG]     = "mouse_right",
    [BUTTON_SELECT_AREA]   = "left shift",
    [BUTTON_SWITCH_MODE]   = "/",
    [BUTTON_NEW_WALL]      = "w",
    [BUTTON_NEW_SIDE]      = "s",
    [BUTTON_NEW_VERTEX]    = "v",
    [BUTTON_EZPORTAL]      = "e",
    [BUTTON_SECTOR]        = "r",
    [BUTTON_DELETE]        = "x",
    [BUTTON_FUSE]          = "f",
    [BUTTON_SPLIT]         = "g",
    [BUTTON_CONNECT]       = "c",
    [BUTTON_PLANE_UP]      = "k",
    [BUTTON_PLANE_DOWN]    = "j",
    [BUTTON_SECTOR_UP]     = "i",
    [BUTTON_SECTOR_DOWN]   = "u",
    [BUTTON_BIGADJUST]     = "left shift",
    [BUTTON_SAVE]          = "left meta+s",
    [BUTTON_OPEN]          = "left meta+o",
    [BUTTON_NEW_DECAL]     = "d",
    [BUTTON_NEW_OBJECT]    = "o",
    [BUTTON_MOUSEMODE]     = "1",
    [BUTTON_MAP_TO_EDCAM]  = "2",
    [BUTTON_EDCAM_TO_MAP]  = "3",
    [BUTTON_CLOSE_ALL]     = "f12",
};

// defines list of button conflicts, if any
// if a button defines a conflict it means that it takes priority and the other
// button is not triggered at all when the first is down
static int BUTTON_CONFLICTS[BUTTON_COUNT][4] = {
    [BUTTON_SAVE] = { BUTTON_NEW_SIDE },
    [BUTTON_OPEN] = { BUTTON_NEW_OBJECT },
    [BUTTON_CANCEL] = { BUTTON_CONNECT },
};

f32 editor_highlight_value(editor_t *ed) {
    return (sinf((state->time.now / 1000000000.0f) * 4.0f) + 1.0f) * 0.5f;
}

bool editor_try_select_ptr(editor_t *ed, lptr_t ptr) {
    if (ed->cur.mode == CM_SELECT) {
        ed->cur.mode_select_id.selected = ptr;
        return true;
    }
    return false;
}

void editor_open_for_ptr(editor_t *ed, lptr_t ptr) {
    if (LPTR_IS_NULL(ptr)) {
        return;
    }

    bool opened = false;

    // TODO: holy mother if conditions, clean this up
    dynlist_each(ed->openptrs, it) {
        if (LPTR_EQ(*it.el, ptr)
            || (LPTR_IS(ptr, T_SIDE)
                && (LPTR_SIDE(ed->level, ptr)->wall == LPTR_WALL(ed->level, *it.el)
                    || (LPTR_SIDE(ed->level, *it.el)
                        && LPTR_SIDE(ed->level, ptr)
                            == side_other(LPTR_SIDE(ed->level, *it.el)))))) {

            *dynlist_push(ed->frontptrs) = ptr;

            opened = true;
            break;
        }
    }

    if (!opened) {
        *dynlist_push(ed->toopenptrs) = ptr;
        *dynlist_push(ed->frontptrs) = ptr;
    }
}

void editor_close_for_ptr(editor_t *ed, lptr_t ptr) {
    *dynlist_push(ed->tocloseptrs) = ptr;
}

void editor_highlight_ptr(editor_t *ed, lptr_t ptr) {
    *dynlist_push(ed->highlight.newptrs) = ptr;
}

bool editor_ptr_is_highlight(editor_t *ed, lptr_t ptr) {
    dynlist_each(ed->highlight.ptrs, it) {
        if (LPTR_EQ(*it.el, ptr)) {
            return true;
        }
    }

    return false;
}

void editor_reset(editor_t *ed) {
    memset(ed, 0, sizeof(*ed));
    ed->map.gridsize = MAP_DEFAULT_GRIDSIZE;
    ed->map.scale = 1;
    ed->visopt |= VISOPT_GRID;
}

void editor_init(editor_t *ed) {
    editor_reset(ed);
    editor_load_settings(ed);
}

void editor_destroy(editor_t *ed) {
    editor_save_settings(ed);

    dynlist_free(ed->map.selected);
    dynlist_free(ed->highlight.newptrs);
    dynlist_free(ed->highlight.ptrs);
    dynlist_free(ed->openptrs);
    dynlist_free(ed->frontptrs);
}

void editor_load_settings(editor_t *ed) {
    usize inidatalen;
    char *inidata;

    char levelpath[1024];
    levelpath[0] = '\0';

    if (file_read_str(EDITOR_SETTINGS_PATH, &inidata, &inidatalen)) {
        WARN("unable to read editor settings");
    } else {
        ini_t *ini = ini_load(inidata, NULL);

        const char
            *s_gridsize =
                ini_property_value(
                    ini, INI_GLOBAL_SECTION,
                    ini_find_property(
                        ini, INI_GLOBAL_SECTION,
                        "gridsize", 0)),
            *s_scale =
                ini_property_value(
                    ini, INI_GLOBAL_SECTION,
                    ini_find_property(
                        ini, INI_GLOBAL_SECTION,
                        "scale", 0)),
             *s_pos =
                ini_property_value(
                    ini, INI_GLOBAL_SECTION,
                    ini_find_property(
                        ini, INI_GLOBAL_SECTION,
                        "pos", 0)),
             *s_visopt =
                ini_property_value(
                    ini, INI_GLOBAL_SECTION,
                    ini_find_property(
                        ini, INI_GLOBAL_SECTION,
                        "visopt", 0)),
             *s_level =
                 ini_property_value(
                     ini, INI_GLOBAL_SECTION,
                     ini_find_property(
                         ini, INI_GLOBAL_SECTION,
                         "level", 0));

        if (s_gridsize) {
            sscanf(s_gridsize, "%f", &ed->map.gridsize);
        }

        if (s_scale) {
            f32 scale;
            sscanf(s_scale, "%f", &scale);
            /* editor_map_set_scale(ed, scale); */
        }

        if (s_pos) {
            sscanf(s_pos, "%f,%f", &ed->map.pos.x, &ed->map.pos.y);
        }

        if (s_visopt) {
            sscanf(s_visopt, "%d", &ed->visopt);
        }

        if (s_level) {
            strncpy(levelpath, s_level, sizeof(levelpath));
        }

        free(inidata);
        ini_destroy(ini);
    }

    if (strlen(levelpath) != 0) {
        state_load_level(state, levelpath);
    }
}

void editor_save_settings(editor_t *ed) {
    ini_t *ini = ini_create(NULL);

    char buf[256];

    snprintf(buf, sizeof(buf), "%f", ed->map.gridsize);
    ini_property_add(ini, INI_GLOBAL_SECTION, "gridsize", 0, buf, 0);

    snprintf(buf, sizeof(buf), "%f", ed->map.scale);
    ini_property_add(ini, INI_GLOBAL_SECTION, "scale", 0, buf, 0);

    snprintf(buf, sizeof(buf), "%f,%f", ed->map.pos.x, ed->map.pos.y);
    ini_property_add(ini, INI_GLOBAL_SECTION, "pos", 0, buf, 0);

    snprintf(buf, sizeof(buf), "%d", ed->visopt);
    ini_property_add(ini, INI_GLOBAL_SECTION, "visopt", 0, buf, 0);

    snprintf(buf, sizeof(buf), "%s", ed->levelpath);
    ini_property_add(ini, INI_GLOBAL_SECTION, "level", 0, buf, 0);

    const int size = ini_save(ini, NULL, 0);
    char *data = malloc(size);
    ini_save(ini, data, size);
    file_write_str(EDITOR_SETTINGS_PATH, data);
    free(data);
    ini_destroy(ini);
}

// sort in DESCENDING order
static int cmp_cmd_priority(
    const cursor_mode_t **a,
    const cursor_mode_t **b) {
    return (*b)->priority - (*a)->priority;
}

void editor_frame(editor_t *ed, level_t *level) {
    ed->size = state->window_size;

    igSetCurrentContext(state->imgui_ctx);
    if (!igGetCurrentContext()->WithinFrameScope) {
        LOG("not current!");
        return;
    }

    // TODO: fix, create camobj HERE (edcam) if it does not exist
    // TODO: more efficient?
    // find EDCAM
    /* level_dynlist_each(level->objects, it) { */
    /*     object_t *o = *it.el; */

    /*     if (o->type_index == OT_EDCAM) { */
    /*         ed->camobj = o; */
    /*         break; */
    /*     } */
    /* } */

    /* ASSERT(ed->camobj); */

    // copy highlight pointers from last frame
    dynlist_copy_from(ed->highlight.ptrs, ed->highlight.newptrs);
    dynlist_resize(ed->highlight.newptrs, 0);

    ImGuiIO *io = igGetIO();
    io->ConfigFlags =
        (ed->mousegrab && ed->mode == EDITOR_MODE_CAM) ?
            (io->ConfigFlags | ImGuiConfigFlags_NoMouse)
            : (io->ConfigFlags & ~ImGuiConfigFlags_NoMouse);

    // read all button states and prevent conflicts
    for (int i = BUTTON_NONE + 1; i < BUTTON_COUNT; i++) {
        ed->buttons[i] = input_get(state->input, BUTTON_KEYS[i]);
    }

    // overwrite conflicts
    for (int i = 0; i < BUTTON_COUNT; i++) {
        const u8 state = ed->buttons[i];

        if (state & (INPUT_PRESS | INPUT_DOWN | INPUT_RELEASE)) {
            for (int j = 0; j < (int) ARRLEN(BUTTON_CONFLICTS[0]); j++) {
                const int k = BUTTON_CONFLICTS[i][j];
                if (k == BUTTON_NONE) {
                    break;
                }

                ed->buttons[k] = INPUT_PRESENT;
            }
        }
    }

    if (!io->WantCaptureMouse && !io->WantCaptureKeyboard) {
        if (ed->mode == EDITOR_MODE_CAM
            && (ed->buttons[BUTTON_MOUSEMODE] & INPUT_PRESS)) {
            ed->mousegrab = !ed->mousegrab;
        }

        if (ed->buttons[BUTTON_MAP_TO_EDCAM] & INPUT_PRESS) {
            editor_map_to_edcam(ed);
       }

        if (ed->buttons[BUTTON_EDCAM_TO_MAP] & INPUT_PRESS) {
            editor_edcam_to_map(ed);
        }
    }

    state->mouse_grab = ed->mode != EDITOR_MODE_MAP && ed->mousegrab;

    // complete reset if requested
    if (ed->reload.on) {
        ed->reload.on = false;
        const bool is_new = ed->reload.is_new;

        char path[sizeof(ed->reload.level_path)];
        snprintf(path, sizeof(path), "%s", ed->reload.level_path);

        memset(ed, 0, sizeof(*ed));
        editor_reset(ed);
        LOG("here?");

        int res;
        if (strlen(path) != 0 && !is_new) {
            if ((res = editor_open_level(ed, path))) {
                editor_show_error(ed, "error opening level: %d", res);
            }
        } else {
            if ((res = editor_new_level(ed, path))) {
                editor_show_error(ed, "error creating new level: %d", res);
            }
        }
    }

    ed->cur.pos.screen = IVEC_TO_V(state->input->cursor.pos_raw);
    ed->cur.pos.map =
        editor_screen_to_map(ed, ed->cur.pos.screen);

    ed->level = level;

    // if saved version is 0 (reset or new level), set to current level version
    if (!ed->savedversion) {
        ed->savedversion = ed->level->version;
    }

    ed->ui_has_mouse = io->WantCaptureMouse;
    ed->ui_has_keyboard = io->WantCaptureKeyboard;

    // check global hotkeys
    if (!igIsPopupOpen_Str("", ImGuiPopupFlags_AnyPopup)) {
        if (ed->buttons[BUTTON_SAVE] & INPUT_PRESS) {
            int res;
            if ((res = editor_save_level(ed, ed->levelpath))) {
                editor_show_error(ed, "error saving level: %d", res);
            }

            editor_save_settings(ed);
        }

        if (ed->buttons[BUTTON_OPEN] & INPUT_PRESS) {
            editor_ui_show_open_level(ed);
        }

        if (ed->buttons[BUTTON_CLOSE_ALL] & INPUT_PRESS) {
            dynlist_resize(ed->openptrs, 0);
        }
    }

    if (!ed->ui_has_keyboard) {
        // switch editor mode
        if (ed->buttons[BUTTON_SWITCH_MODE]
                & INPUT_PRESS) {
            ed->mode =
                ed->mode == EDITOR_MODE_MAP ?
                    EDITOR_MODE_CAM
                    : EDITOR_MODE_MAP;
        }

        // cancel operation with CANCEL
        if (ed->buttons[BUTTON_CANCEL] & INPUT_PRESS) {
            editor_map_clear_select(ed);
            ed->cur.mode = CM_DEFAULT;
        }

        if (ed->mode == EDITOR_MODE_MAP) {
            // map controls
            // TODO: dt
            const f32 speed = 0.025f / ed->map.scale;
            if (ed->buttons[BUTTON_LEFT] & INPUT_DOWN) {
                ed->map.pos.x -= speed;
            }

            if (ed->buttons[BUTTON_RIGHT] & INPUT_DOWN) {
                ed->map.pos.x += speed;
            }

            if (ed->buttons[BUTTON_UP] & INPUT_DOWN) {
                ed->map.pos.y += speed;
            }

            if (ed->buttons[BUTTON_DOWN] & INPUT_DOWN) {
                ed->map.pos.y -= speed;
            }

            if (ed->buttons[BUTTON_ZOOM_IN] & INPUT_PRESS) {
                editor_map_set_scale(ed, ed->map.scale * 2.0f);
            }

            if (ed->buttons[BUTTON_ZOOM_OUT] & INPUT_PRESS) {
                editor_map_set_scale(ed, ed->map.scale / 2.0f);
            }
        }
    }

    // update camera cursor data
    memset(&ed->cam, 0, sizeof(ed->cam));

    if (ed->mode == EDITOR_MODE_CAM && !ed->ui_has_mouse) {
        ed->cam.data.raw =
            renderer_info_at(state->renderer, state->input->cursor.pos_3d);
        ed->cam.ptr =
            lptr_from_nogen(
                ed->level,
                lptr_nogen_from_raw((u32) ifnaninf(ed->cam.data.generic.id, 0, 0)));

        switch (LPTR_TYPE(ed->cam.ptr)) {
        case T_SECTOR:
            ed->cam.sect = LPTR_SECTOR(ed->level, ed->cam.ptr);
            ed->cam.plane =
                ed->cam.data.sector.is_ceil > 0 ?
                    PLANE_TYPE_CEIL : PLANE_TYPE_FLOOR;
            ed->cam.sect_pos = ed->cam.data.sector.pos;
            break;
        case T_SIDE:
            ed->cam.side = LPTR_SIDE(ed->level, ed->cam.ptr);
            ed->cam.sect = ed->cam.side->sector;
            ed->cam.side_pos = ed->cam.data.side.pos;
            break;
        case T_DECAL:;
            decal_t *decal = LPTR_DECAL(ed->level, ed->cam.ptr);
            if (decal->is_on_side) {
                ed->cam.side = decal->side.ptr;
                ed->cam.side_pos = ed->cam.data.decal.pos;
                ed->cam.sect = decal->side.ptr->sector;
            } else {
                ed->cam.sect = decal->sector.ptr;
                ed->cam.plane =
                    ed->cam.data.decal.is_ceil > 0 ?
                        PLANE_TYPE_CEIL : PLANE_TYPE_FLOOR;
                ed->cam.sect_pos = ed->cam.data.decal.pos;
            }
            break;
        case T_OBJECT:
            ed->cam.sect = LPTR_OBJECT(ed->level, ed->cam.ptr)->sector;
            break;
        default: break;
        }
    }

    // respond to sector up/down
    if (ed->mode == EDITOR_MODE_CAM && ed->cam.sect) {
        sector_t *sect = ed->cam.sect;
        plane_t *plane = &sect->planes[ed->cam.plane];

        // do "big" adjustments
        const f32 delta =
            (ed->buttons[BUTTON_BIGADJUST] & INPUT_DOWN) ? 0.125f : (1.0f / 32.0f);

        bool change = true;
        if (plane
            && (ed->buttons[BUTTON_PLANE_UP]
                & (INPUT_PRESS | INPUT_REPEAT))) {
            plane->z += delta;
        } else if (
            plane
            && (ed->buttons[BUTTON_PLANE_DOWN]
                & (INPUT_PRESS | INPUT_REPEAT))) {
            plane->z -= delta;
        } else if (ed->buttons[BUTTON_SECTOR_UP]
                & (INPUT_PRESS | INPUT_REPEAT)) {
            sect->floor.z += delta;
            sect->ceil.z += delta;
        } else if (ed->buttons[BUTTON_SECTOR_DOWN]
                & (INPUT_PRESS | INPUT_REPEAT)) {
            sect->floor.z -= delta;
            sect->ceil.z -= delta;
        } else {
            change = false;
        }

        if (change) {
            lptr_recalculate(level, LPTR_FROM(sect));
        }
    }

    // show all ui windows
    editor_show_ui(ed);

    // TODO
    // also highlight in renderer if enabled
    if (ed->visopt & VISOPT_HIGHLIGHT) {
        /* ed->renderer->highlight = */
        /*     dynlist_size(ed->highlight.ptrs) == 0 ? */
        /*         LPTR_NULL */
        /*         : ed->highlight.ptrs[0]; */
    } else {
        /* ed->renderer->highlight = LPTR_NULL; */
    }

    // draw 2D frame if in 2D mode
    if (ed->mode == EDITOR_MODE_MAP) {
        editor_map_frame(ed);
    }

    // update cursor mode
    if (!ed->ui_has_mouse && !ed->ui_has_keyboard) {
        // CMF_EXPLICIT_CANCEL means other cursor modes cannot trigger while
        // this cursor mode is active
        if (!(CURSOR_MODES[ed->cur.mode].flags & CMF_EXPLICIT_CANCEL)) {
            // check triggers, in order of trigger_priority
            const cursor_mode_t *cmds[CM_COUNT];
            for (int i = 0; i < CM_COUNT; i++) {
                cmds[i] = &CURSOR_MODES[i];
            }

            sort(
                cmds,
                CM_COUNT,
                sizeof(cursor_mode_t*),
                (f_sort_cmp) cmp_cmd_priority,
                NULL);

            for (int i = 0; i < CM_COUNT; i++) {
                const cursor_mode_t *cmd = cmds[i];
                if (cmd->trigger
                    && (ed->mode == EDITOR_MODE_MAP
                        || (cmd->flags & CMF_CAM))) {
                    if (cmd->trigger(ed, &ed->cur)) {
                        // run cancel operation if present
                        if (CURSOR_MODES[ed->cur.mode].cancel) {
                            CURSOR_MODES[ed->cur.mode].cancel(ed, &ed->cur);
                        }

                        // switch mode
                        ed->cur.mode = cmd->mode;
                        break;
                    }
                }
            }
        }

        // update only, rendering is handled in editor_map_frame
        const cursor_mode_t *cmd = &CURSOR_MODES[ed->cur.mode];
        if (cmd->update) { cmd->update(ed, &ed->cur); }
    }

    // if clicked in 3D camera area, open editor for selected thing
    if (!LPTR_IS_NULL(ed->cam.ptr)
        && !(CURSOR_MODES[ed->cur.mode].flags & CMF_CAM)) {
        const bool
            select = ed->buttons[BUTTON_SELECT] & INPUT_PRESS,
            deselect = ed->buttons[BUTTON_DESELECT] & INPUT_PRESS;

        // try to select something (as in "SELECT ...") but only with select
        // button
        bool selectedptr = false;
        if (select) {
            selectedptr = editor_try_select_ptr(ed, ed->cam.ptr);
        }

        // otherwise, open editor for (or focus) clicked element if not being
        // selected. also works for deselect (right click)
        if ((select || deselect) && !selectedptr) {
            editor_open_for_ptr(ed, ed->cam.ptr);
        }
    }

    /* p_set_show_editor(p_get(), ed->mode == EDITOR_MODE_MAP); */
    state->allow_input =
        !ed->ui_has_keyboard && ed->mode == EDITOR_MODE_CAM;
}

int editor_open_level(editor_t *ed, const char *path) {
    path = path ? path : ed->levelpath;
    if (!file_exists(path) || file_isdir(path)) { return 1; }

    int res;
    if ((res = state_load_level(state, path))) { return res; }

    snprintf(ed->levelpath, sizeof(ed->levelpath), "%s", path);

    return res;
}

int editor_save_level(editor_t *ed, const char *path) {
    path = path ? path : ed->levelpath;

    FILE *f = fopen(path, "w");
    if (!f) { return 1; }

    int res;
    if ((res = file_makebak(path, ".bak"))) {
        WARN("failed to make backup: %d", res);
        return -1;
    }

    res = io_save_level(f, ed->level);
    fclose(f);

    if (res) { return res; }

    snprintf(ed->levelpath, sizeof(ed->levelpath), "%s", path);
    ed->savedversion = ed->level->version;
    return res;
}

int editor_new_level(editor_t *ed, const char *path) {
    ASSERT(path);
    memset(ed, 0, sizeof(*ed));
    editor_reset(ed);
    int res;
    if ((res = state_load_level(state, NULL))) { return res; }
    snprintf(ed->levelpath, sizeof(ed->levelpath), "%s", path);
    return 0;
}
