#include "editor/cursor.h"
#include "editor/map.h"
#include "editor/editor.h"
#include "editor/cursor.h"
#include "level/level.h"
#include "level/lptr.h"
#include "level/vertex.h"
#include "level/wall.h"
#include "level/side.h"
#include "level/sidemat.h"
#include "level/sector.h"
#include "level/sectmat.h"
#include "level/decal.h"
#include "level/object.h"
#include "level/tag.h"
#include "gfx/sokol.h"
#include "util/input.h"
#include "util/aabb.h"
#include "state.h"

static void cm_default_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    snprintf(p, n, "%s", "NORMAL");
}

static void cm_select_id_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    char tagnames[128];
    level_types_to_str(tagnames, sizeof(tagnames), ed->cur.mode_select_id.tag);
    snprintf(p, n, "SELECT %s", tagnames);
}

static void cm_select_id_update(editor_t *ed, cursor_t *cur) {
    // try to select hovered item when select is pressed
    if (ed->buttons[BUTTON_SELECT] & INPUT_RELEASE) {
        editor_map_clear_select(ed);
        cur->mode_select_id.selected =
            ed->mode == EDITOR_MODE_MAP ? cur->hover : ed->cam.ptr;
    }
}

static void decal_placement_pos(
    editor_t *ed,
    vec2s pos,
    side_t **pside,
    f32 *px,
    sector_t **psector,
    vec2s *ppos,
    vec2s *pworld) {
    // check if we are "on" a side
    DYNLIST(wall_t*) near_walls = NULL;
    level_walls_in_radius(
        ed->level,
        pos,
        MAP_DECAL_SIZE * 0.8f,
        &near_walls);

    side_t *side = NULL;
    f32 side_dist = 1e10;

    dynlist_each(near_walls, it) {
        wall_t *wall = *it.el;
        const vec2s wall_to_pos = glms_vec2_sub(pos, wall_midpoint(wall));

        for (int i = 0; i < 2; i++) {
            if (!wall->sides[i]) { 
                continue; 
            }

            // check that pos is on this side of wall
            if (glms_vec2_dot(
                    side_normal(wall->sides[i]), wall_to_pos) < 0.0f) {
                continue;
            }

            const f32 dist =
                point_to_segment(pos, wall->v0->pos, wall->v1->pos);

            if (!side || dist < side_dist) {
                side = wall->sides[i];
                side_dist = dist;
            }
        }
    }

    if (side) {
        vertex_t *vs[2];
        side_get_vertices(side, vs);

        const f32 t =
            clamp(
                point_project_segment_t(pos, vs[0]->pos, vs[1]->pos),
                0, 1);
        *pside = side;
        *px = t * side->wall->len;
        *pworld = glms_vec2_lerp(vs[0]->pos, vs[1]->pos, t);
    } else {
        sector_t *sector = NULL;
        if ((sector = level_find_point_sector(ed->level, pos, NULL))) {
            *psector = sector;
            *ppos = pos;
            *pworld = pos;
        }
    }

    dynlist_free(near_walls);
}

static bool cm_drag_trigger(editor_t *ed, cursor_t *cur) {
    if (CURSOR_MODES[cur->mode].flags & CMF_NODRAG) {
        return false;
    }

    if (LPTR_IS(cur->hover, T_WALL | T_VERTEX | T_DECAL | T_OBJECT)
        && (ed->buttons[BUTTON_SELECT] & INPUT_PRESS)) {
        cur->mode_drag.start = cur->pos.map;
        cur->mode_drag.startmap = NULL;
        return true;
    }

    return false;
}

static uintptr_t lptr_to_key(lptr_t ptr) {
    return (uintptr_t) ptr.raw;
}

typedef struct {
    vec2s start;
} drag_data_t;

static void cm_drag_update(editor_t *ed, cursor_t *cur) {
    const u8
        bs_drag = ed->buttons[BUTTON_SELECT],
        bs_snap = ed->buttons[BUTTON_SNAP];

    const vec2s
        p = cur->pos.map,
        delta = glms_vec2_sub(p, cur->mode_drag.start);

    // create map if not already created
    if (!cur->mode_drag.startmap) {
        cur->mode_drag.startmap = malloc(sizeof(map_t));
        map_init(
            cur->mode_drag.startmap,
            map_hash_id,
            NULL,
            NULL,
            NULL,
            map_cmp_id,
            NULL,
            map_default_free,
            NULL);
    }

    if (bs_drag & INPUT_RELEASE) {
        // stop dragging
        cur->mode = CM_DEFAULT;

        map_destroy(cur->mode_drag.startmap);
        free(cur->mode_drag.startmap);
        cur->mode_drag.startmap = NULL;

        // deselect UNLESS area drag is down
        if (!(ed->buttons[BUTTON_SELECT_AREA] & INPUT_DOWN)) {
            editor_map_clear_select(ed);
        }
    } else if (bs_drag & INPUT_DOWN) {
        // accumulate list of things (vertices, objects, decals) to move
        DYNLIST(lptr_t) tomove = NULL;

        dynlist_each(ed->map.selected, it) {
            switch (LPTR_TYPE(*it.el)) {
            case T_VERTEX:
            case T_DECAL:
            case T_OBJECT: {
                *dynlist_push(tomove) = *it.el;
            } break;
            case T_WALL: {
                wall_t *w = LPTR_WALL(ed->level, *it.el);
                *dynlist_push(tomove) = LPTR_FROM(w->v0);
                *dynlist_push(tomove) = LPTR_FROM(w->v1);
            } break;
            default: break;
            }
        }

        // only move each thing in list onced, don't move marked
        dynlist_each(tomove, it) {
            u8 *pflags = LPTR_FIELD(ed->level, *it.el, level_flags);
            if (*pflags & LF_MARK) { continue; }

            // lookup by pointer
            drag_data_t
                **pdata =
                    map_find(
                        drag_data_t*,
                        cur->mode_drag.startmap,
                        lptr_to_key(*it.el)),
                *data = pdata ? *pdata : NULL;

            if (!data) {
                LOG("no data for ptr %08x", (*it.el).raw);

                // create drag data
                data = malloc(sizeof(*data));

                switch (LPTR_TYPE(*it.el)) {
                case T_VERTEX:
                    data->start = LPTR_VERTEX(ed->level, *it.el)->pos;
                    break;
                case T_DECAL:
                    data->start = decal_worldpos(LPTR_DECAL(ed->level, *it.el));
                    break;
                case T_OBJECT:
                    data->start = LPTR_OBJECT(ed->level, *it.el)->pos;
                    break;
                default: ASSERT(false, "bad drag type %d", LPTR_TYPE(*it.el));
                }

                // insert into map
                map_insert(cur->mode_drag.startmap, lptr_to_key(*it.el), data);
            }

            const vec2s
                start = data->start,
                rawpos = glms_vec2_add(start, delta),
                pos =
                    (bs_snap & INPUT_DOWN) ?
                        editor_map_snap_to_grid(ed, rawpos)
                        : rawpos;

            // mark as moved
            *pflags |= LF_MARK;
            switch (LPTR_TYPE(*it.el)) {
            case T_VERTEX:
                LPTR_VERTEX(ed->level, *it.el)->pos =
                    glms_vec2_maxv(pos, VEC2(0));
                break;
            case T_DECAL: {
                decal_t *decal = LPTR_DECAL(ed->level, *it.el);

                side_t *side = NULL;
                f32 x;
                sector_t *sector = NULL;
                vec2s sect_pos, world_pos;
                decal_placement_pos(
                    ed, pos, &side, &x, &sector, &sect_pos, &world_pos);

                if (side) {
                    decal_set_side(ed->level, decal, side);
                    decal->side.offsets = VEC2(x, decal->side.offsets.y);
                } else if (sector) {
                    decal_set_sector(
                        ed->level, decal, sector, decal->sector.plane);
                    decal->sector.pos = world_pos;
                }
            } break;
            case T_OBJECT: {
                object_move(ed->level, LPTR_OBJECT(ed->level, *it.el), pos);
            } break;
            default: ASSERT(false);
            }

            lptr_recalculate(ed->level, *it.el);
        }

        // clear marks
        dynlist_each(tomove, it) {
            *LPTR_FIELD(ed->level, *it.el, level_flags) &= ~LF_MARK;
        }

        dynlist_free(tomove);
    }
}

static void cm_drag_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    char buf[32];
    snprintf(p, n, "%s", "DRAG");

    dynlist_each(ed->map.selected, it) {
        lptr_to_str(buf, sizeof(buf), ed->level, *it.el);
        xnprintf(p, n, "%s%s", it.i == 0 ? " " : ", ", buf);
    }
}

static void cm_drag_cancel(editor_t *ed, cursor_t *cur) {
    if (cur->mode_drag.startmap) {
        map_destroy(cur->mode_drag.startmap);
        free(cur->mode_drag.startmap);
        cur->mode_drag.startmap = NULL;
    }
}

static bool cm_move_drag_trigger(editor_t *ed, cursor_t *cur) {
    if (cur->mode == CM_DEFAULT
        && (LPTR_IS_NULL(cur->hover) || LPTR_IS(cur->hover, T_SECTOR))
        && (ed->buttons[BUTTON_MOVE_DRAG] & INPUT_PRESS)) {
        cur->mode_move_drag.last = cur->pos.screen;
        cur->mode_move_drag.moved = false;
        return true;
    }

    return false;
}

static void cm_move_drag_update(editor_t *ed, cursor_t *cur) {
    const u8 bs = ed->buttons[BUTTON_MOVE_DRAG];

    if (bs & INPUT_RELEASE) {
        cur->mode = CM_DEFAULT;
    } else if (bs & INPUT_DOWN) {
        const vec2s delta =
            glms_vec2_sub(
                editor_screen_to_map(ed, cur->pos.screen),
                editor_screen_to_map(ed, cur->mode_move_drag.last));

        if (delta.x != 0 || delta.y != 0) {
            cur->mode_move_drag.moved = true;
        }

        ed->map.pos = glms_vec2_sub(ed->map.pos, delta);
        cur->mode_move_drag.last = cur->pos.screen;
    }
}

static void cm_move_drag_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    snprintf(p, n, "%s", "MOVE DRAG");
}

static bool cm_select_area_trigger(editor_t *ed, cursor_t *cur) {
    if (cur->mode == CM_DEFAULT
        && (LPTR_IS_NULL(cur->hover) || LPTR_IS(cur->hover, T_SECTOR))
        && (ed->buttons[BUTTON_SELECT_AREA] & INPUT_DOWN)
        && (ed->buttons[BUTTON_SELECT] & INPUT_DOWN)) {
        cur->mode_select_area.start = cur->pos.map;
        cur->mode_select_area.end = cur->mode_select_area.start;
        cur->mode_select_area.ptrs = NULL;
        return true;
    }

    return false;
}

static void cm_select_area_update(editor_t *ed, cursor_t *cur) {
    const u8
        bs0 = ed->buttons[BUTTON_SELECT_AREA],
        bs1 = ed->buttons[BUTTON_SELECT];

    cur->mode_select_area.end = cur->pos.map;

    // update selection list
    dynlist_resize(cur->mode_select_area.ptrs, 0);

    editor_map_ptrs_in_area(
        ed,
        &cur->mode_select_area.ptrs,
        cur->mode_select_area.start,
        cur->mode_select_area.end,
        T_VERTEX | T_WALL | T_OBJECT | T_DECAL,
        E_MPIA_WHOLEWALL);

    if (!(bs0 & INPUT_DOWN) || !(bs1 & INPUT_DOWN)) {
        // select everything in area
        cur->mode = CM_DEFAULT;

        editor_map_clear_select(ed);
        dynlist_each(cur->mode_select_area.ptrs, it) {
            editor_map_select(ed, *it.el);
        }

        dynlist_free(cur->mode_select_area.ptrs);
    }
}

static void cm_select_area_render(editor_t *ed, cursor_t *cur) {
    // render box
    sgp_set_color(1.0f, 1.0f, 1.0f, 0.3f);

    sgp_draw_filled_aabbf(
        aabbf_sort(
            AABBF_MM(
                cur->mode_select_area.start,
                cur->mode_select_area.end)));
}

static void cm_select_area_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    snprintf(
        p, n,
        "SELECT (%f, %f) -> (%f, %f)",
        cur->mode_select_area.start.x,
        cur->mode_select_area.start.y,
        cur->mode_select_area.end.x,
        cur->mode_select_area.end.y);
}

static void cm_select_area_cancel(editor_t *ed, cursor_t *cur) {
    dynlist_free(cur->mode_select_area.ptrs);
}

static bool cm_wall_trigger(editor_t *ed, cursor_t *cur) {
    if (cur->mode == CM_DEFAULT
        && (ed->buttons[BUTTON_NEW_WALL]
                & INPUT_PRESS)) {
        cur->mode_wall.started = false;
        return true;
    }

    return false;
}

static void cm_wall_update(editor_t *ed, cursor_t *cur) {
    const u8
        bs_select = ed->buttons[BUTTON_SELECT],
        bs_deselect = ed->buttons[BUTTON_DESELECT];

    // stop after deselect
    if (bs_deselect & INPUT_RELEASE) {
        cur->mode = CM_DEFAULT;
        return;
    }

    if (!(bs_select & INPUT_RELEASE)) {
        return;
    }

    if (!cur->mode_wall.started) {
        cur->mode_wall.started = true;
        cur->mode_wall.start =
            editor_map_snapped_cursor_pos(
                ed, cur, &cur->mode_wall.startptr, NULL);
        return;
    }

    // TODO: prevent walls which span multiple sectors

    lptr_t endptr;
    vec2s endpos = editor_map_snapped_cursor_pos(ed, cur, &endptr, NULL);

    // get or create two vertices from start to here
    const vec2s ps[2] = { cur->mode_wall.start, endpos };
    lptr_t ptrs[2] = { cur->mode_wall.startptr, endptr };

    // pointers should either be vertex, wall, or nothing
    for (int i = 0; i < 2; i++) {
        switch (LPTR_TYPE(ptrs[i])) {
        case T_VERTEX:
        case T_WALL:
            break;
        case T_SIDE:
            ptrs[i] = LPTR_FROM(LPTR_SIDE(ed->level, ptrs[i])->wall);
            break;
        default:
            ptrs[i] = LPTR_NULL;
            break;
        }
    }

    vertex_t *vs[2] = { NULL, NULL };
    bool onborder[2] = { false, false };

    // get or create two end vertices
    for (int i = 0; i < 2; i++) {
        if (LPTR_IS_NULL(ptrs[i])) {
            vs[i] = vertex_new(ed->level, ps[i]);
            onborder[i] = false;
        } else {
            switch (LPTR_TYPE(ptrs[i])) {
            case T_VERTEX: {
                vs[i] = LPTR_VERTEX(ed->level, ptrs[i]);
                onborder[i] = true;
            } break;
            case T_WALL: {
                vs[i] =
                    wall_split(
                        ed->level,
                        LPTR_WALL(ed->level, ptrs[i]),
                        ps[i])->v0;
                onborder[i] = true;
            } break;
            default: ASSERT(false);
            }
        }
    }

    sector_t
        *sharesect = vertices_shared_sector(ed->level, vs[0], vs[1]),
        *insidesect = NULL;

    // try to find sector which wall is being created inside
    // don't use vertices which were on existing borders (walls or vertices
    // themselves)
    for (int i = 0; i < 2 && !insidesect; i++) {
        if (!onborder[i]) {
            insidesect =
                level_find_point_sector(ed->level, vs[i]->pos, NULL);
        }
    }

    // make a wall between the two vertices
    wall_t *newwall = wall_new(ed->level, vs[0], vs[1]);

    // find near side on vs[0] (where wall creation started)
    side_t *nearside = level_find_near_side(ed->level, vs[0], sharesect);

    // if there is a shared or inside sector, create a sides on both sides
    // otherwise only create a side on the right.
    // shared sectors also portal to each other
    for (int i = 0; i < 2; i++) {
        if (!sharesect && !insidesect && i == 1) {
            // check that there is actually a sector here if we're creating the
            // opposite wall
            const vec2s midpoint = wall_midpoint(newwall);
            if (!level_find_point_sector(
                    ed->level,
                    glms_vec2_sub(midpoint, glms_vec2_sign(newwall->normal)),
                    NULL)) {
                continue;
            }
        }

        side_t *newside = side_new(ed->level, nearside);
        wall_set_side(ed->level, newwall, i, newside);
    }

    for (int i = 0; i < 2; i++) {
        if (!newwall->sides[i]) { continue; }

        if (sharesect) {
            sector_add_side(ed->level, sharesect, newwall->sides[i]);
        }
    }

    // shared sector, portal sides to each other
    if (sharesect && newwall->sides[0] && newwall->sides[1]) {
        newwall->sides[0]->portal = newwall->sides[1];
        newwall->sides[1]->portal = newwall->sides[0];
        side_recalculate(ed->level, newwall->sides[1]);
        side_recalculate(ed->level, newwall->sides[0]);
    }

    // restart with wall on current vertex if shift pressed
    if (ed->buttons[BUTTON_MULTI_SELECT]
            & INPUT_DOWN) {
        cur->mode_wall.started = true;
        cur->mode_wall.start = endpos;
        cur->mode_wall.startptr = LPTR_FROM(vs[1]);
    } else {
        cur->mode_wall.started = false;
    }
}

static void cm_wall_render(editor_t *ed, cursor_t *cur) {
    sgp_set_color(SVEC4(MAP_NEW_WALL_COLOR));
    const vec2s cursor_pos = editor_map_snapped_cursor_pos(ed, cur, NULL, NULL);

    if (cur->mode_wall.started) {
        // vertex
        sgp_draw_filled_rect(
            SVEC2(
                glms_vec2_sub(
                    cur->mode_wall.start,
                    VEC2(MAP_VERTEX_SIZE / 2.0f))),
            SVEC2(VEC2(MAP_VERTEX_SIZE)));

        // wall
        sgp_draw_thick_line(
            SVEC2(cur->mode_wall.start),
            SVEC2(cursor_pos),
            MAP_LINE_THICKNESS);
    }

    // draw potential vertex location
    sgp_draw_filled_rect(
        SVEC2(
            glms_vec2_sub(
                cursor_pos,
                VEC2(MAP_VERTEX_SIZE / 2.0f))),
        SVEC2(VEC2(MAP_VERTEX_SIZE)));
}

static void cm_wall_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    if (cur->mode_wall.started) {
        snprintf(
            p, n,
            "WALL FROM (%f, %f)",
            cur->mode_wall.start.x,
            cur->mode_wall.start.y);
    } else {
        snprintf(p, n, "%s", "WALL");
    }
}

static bool cm_vertex_trigger(editor_t *ed, cursor_t *cur) {
    return cur->mode == CM_DEFAULT
        && (ed->buttons[BUTTON_NEW_VERTEX] & INPUT_PRESS);
}

static void cm_vertex_update(editor_t *ed, cursor_t *cur) {
    const u8
        bs_select = ed->buttons[BUTTON_SELECT],
        bs_deselect = ed->buttons[BUTTON_DESELECT];

    // stop after deselect
    if (bs_deselect & INPUT_RELEASE) {
        cur->mode = CM_DEFAULT;
        return;
    }

    if (!(bs_select & INPUT_RELEASE)) {
        return;
    }

    // add new vertex
    wall_t *wall = NULL;
    lptr_t hover = LPTR_NULL;
    const vec2s pos = editor_map_snapped_cursor_pos(ed, cur, &hover, NULL);

    if (LPTR_IS(hover, T_WALL)) {
        wall = LPTR_WALL(ed->level, hover);
    } else if (LPTR_IS(hover, T_SIDE)) {
        wall = LPTR_SIDE(ed->level, hover)->wall;
    }

    if (wall) {
        wall_split(ed->level, wall, pos);
    } else {
        vertex_new(ed->level, pos);
    }
}

static void cm_vertex_render(editor_t *ed, cursor_t *cur) {
    const vec2s pos = editor_map_snapped_cursor_pos(ed, cur, NULL, NULL);
    sgp_set_color(SVEC4(MAP_NEW_VERTEX_COLOR));
    sgp_draw_filled_rect(
        SVEC2(
            glms_vec2_sub(
                pos,
                VEC2(MAP_VERTEX_SIZE / 2.0f))),
        SVEC2(VEC2(MAP_VERTEX_SIZE)));
}

static void cm_vertex_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    wall_t *wall;

    lptr_t hover;
    const vec2s pos = editor_map_snapped_cursor_pos(ed, cur, &hover, NULL);
    wall = LPTR_WALL(ed->level, hover);

    snprintf(p, n, "VERTEX (%f, %f)", pos.x, pos.y);

    if (wall) {
        char buf[32];
        lptr_to_str(buf, sizeof(buf), ed->level, LPTR_FROM(wall));
        xnprintf(p, n, "(%s)", buf);
    }
}

static bool cm_side_trigger(editor_t *ed, cursor_t *cur) {
    if (cur->mode == CM_DEFAULT
        && (ed->buttons[BUTTON_NEW_SIDE] & INPUT_PRESS)) {
        cur->mode_side.started = false;
        return true;
    }

    return false;
}

static void cm_side_update(editor_t *ed, cursor_t *cur) {
    if (ed->buttons[BUTTON_SELECT] & INPUT_PRESS) {
        cur->mode_side.started = true;
        cur->mode_side.start = cur->pos.map;
    }

    if (!(cur->mode_side.started
            && (ed->buttons[BUTTON_SELECT] & INPUT_RELEASE))) {
        return;
    }

    // find sides we intersect
    wall_t *walls[64];
    side_t **sides[64];
    const int n =
        level_intersect_walls_on_line(
            ed->level,
            cur->mode_side.start,
            cur->pos.map,
            &walls[0],
            &sides[0],
            16);

    for (int i = 0; i < n; i++) {
        if (*sides[i]) { continue; }

        // create side like other side
        const int index = sides[i] == &walls[i]->sides[0] ? 0 : 1;

        side_t *new_side = side_new(ed->level, walls[i]->sides[1 - index]);
        wall_set_side(ed->level, walls[i], index, new_side);
        side_recalculate(ed->level, new_side);
    }

    // return to default state unless shift is held
    if (!(ed->buttons[BUTTON_MULTI_SELECT]
            & INPUT_DOWN)) {
        cur->mode = CM_DEFAULT;
    }

    cur->mode_side.started = false;
}

static void cm_side_render(editor_t *ed, cursor_t *cur) {
    if (!cur->mode_side.started) { return; }

    sgp_set_color(SVEC4(MAP_SIDE_ARROW_COLOR));
    sgp_draw_thick_line(
        SVEC2(cur->mode_side.start),
        SVEC2(cur->pos.map),
        MAP_LINE_THICKNESS);
}

static void cm_side_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    snprintf(p, n, "%s", "SIDE");
}

static bool cm_ezportal_trigger(editor_t *ed, cursor_t *cur) {
    if (cur->mode == CM_DEFAULT
        && (ed->buttons[BUTTON_EZPORTAL] & INPUT_RELEASE)) {
        cur->mode_ezportal.started = false;
        return true;
    }

    return false;
}

static void cm_ezportal_update(editor_t *ed, cursor_t *cur) {
    if (ed->buttons[BUTTON_SELECT]
            & INPUT_PRESS) {
        cur->mode_ezportal.started = true;
        cur->mode_ezportal.start = cur->pos.map;
    }

    if (!(cur->mode_ezportal.started
            && ed->buttons[BUTTON_SELECT]
                & INPUT_RELEASE)) {
        return;
    }

    // get walls in area and make portals between all sides, making sides if
    // they do not already exist
    DYNLIST(lptr_t) ptrs = NULL;
    editor_map_ptrs_in_area(
        ed,
        &ptrs,
        cur->mode_ezportal.start,
        cur->pos.map,
        T_WALL,
        E_MPIA_NONE);

    dynlist_each(ptrs, it) {
        wall_t *wall = LPTR_WALL(ed->level, *it.el);

        // must be a sector on either side of wall
        sector_t *sects[2];
        for (int i = 0; i < 2; i++) {
            if (wall->sides[i]) {
                sects[i] = wall->sides[i]->sector;
            } else {
                const int sign = i == 0 ? 1 : -1;
                const vec2s midpoint = wall_midpoint(wall);
                sects[i] =
                    level_find_point_sector(
                        ed->level,
                        glms_vec2_add(
                            midpoint,
                            glms_vec2_scale(
                                wall->normal,
                                0.001f * sign)),
                        NULL);

                // if no sector is found, try to add a side and create one
                side_t *newside =
                    side_new(
                        ed->level,
                        wall->sides[1 - i]);

                wall_set_side(ed->level, wall, i, newside);

                sects[i] = level_update_side_sector(ed->level, newside);

                // if we couldn't create sector, remove side
                if (!sects[i]) {
                    side_delete(ed->level, newside);
                }
            }
        }

        // must have side on both sides now
        if (!wall->side0 || !wall->side1) {
            continue;
        }

        // sides must have sectors
        if (!wall->side0->sector || !wall->side1->sector) {
            continue;
        }

        wall->side0->portal = wall->side1;
        wall->side1->portal = wall->side0;

        wall->side0->mat = ed->level->sidemats[SIDEMAT_NOMAT];
        wall->side1->mat = ed->level->sidemats[SIDEMAT_NOMAT];

        side_recalculate(ed->level, wall->side0);
        side_recalculate(ed->level, wall->side1);

        if (wall->side0->sector) {
            sector_recalculate(ed->level, wall->side0->sector);
        }

        if (wall->side1->sector) {
            sector_recalculate(ed->level, wall->side1->sector);
        }
    }

    dynlist_free(ptrs);

    // only continue on shift
    if (!(ed->buttons[BUTTON_MULTI_SELECT]
            & INPUT_DOWN)) {
        cur->mode = CM_DEFAULT;
    }

    cur->mode_ezportal.started = false;
}

static void cm_ezportal_render(editor_t *ed, cursor_t *cur) {
    if (!cur->mode_ezportal.started) { return; }

    sgp_set_color(SVEC4(MAP_SELECT_BORDER_COLOR));
    sgp_draw_thick_line(
        SVEC2(cur->mode_ezportal.start),
        SVEC2(cur->pos.map),
        MAP_LINE_THICKNESS);
}

static void cm_ezportal_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    snprintf(p, n, "%s", "EZPORTAL");
}

static bool cm_sector_trigger(editor_t *ed, cursor_t *cur) {
    return cur->mode == CM_DEFAULT
        && (ed->buttons[BUTTON_SECTOR] & INPUT_RELEASE);
}

static void cm_sector_update(editor_t *ed, cursor_t *cur) {
    if (!(ed->buttons[BUTTON_SELECT] & INPUT_PRESS)) {
        return;
    }

    // find nearest side to cursor and repair its sector
    side_t *nearest = level_nearest_side(ed->level, cur->pos.map);

    if (nearest) {
        // TODO: some indicator, status_line maybe?
        level_update_side_sector(ed->level, nearest);

        if (nearest->sector) {
            sector_recalculate(ed->level, nearest->sector);
        }
    }

    if (!(ed->buttons[BUTTON_MULTI_SELECT] & INPUT_DOWN)) {
        cur->mode = CM_DEFAULT;
    }
}

static void cm_sector_render(editor_t *ed, cursor_t *cur) {
    // show nearest side which would be affected
    side_t *nearest = level_nearest_side(ed->level, cur->pos.map);

    if (nearest) {
        sgp_set_color(SVEC4(MAP_SELECT_BORDER_COLOR));
        sgp_draw_thick_line(
            SVEC2(side_normal_point(nearest)),
            SVEC2(cur->pos.map),
            MAP_LINE_THICKNESS);
    }
}

static void cm_sector_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    snprintf(p, n, "%s", "SECTOR FIXER");
}

static bool cm_delete_trigger(editor_t *ed, cursor_t *cur) {
    if (cur->mode == CM_DEFAULT
        && (ed->buttons[BUTTON_DELETE] & INPUT_RELEASE)) {
        cur->mode_delete.drag = false;
        return true;
    }

    return false;
}

static void cm_delete_update(editor_t *ed, cursor_t *cur) {
    if ((ed->buttons[BUTTON_SELECT] & INPUT_PRESS)) {
        if (ed->buttons[BUTTON_SELECT_AREA] & INPUT_DOWN) {
            cur->mode_delete.drag = true;
            cur->mode_delete.dragstart = cur->pos.map;
        } else if (!LPTR_IS_NULL(ed->cur.hover)) {
            // try to delete hover
            lptr_delete(ed->level, ed->cur.hover);
        }
    }

    if (cur->mode_delete.drag
        && (ed->buttons[BUTTON_SELECT] & INPUT_RELEASE)) {
        // delete in area
        DYNLIST(lptr_t) ptrs = NULL;
        editor_map_ptrs_in_area(
            ed,
            &ptrs,
            cur->mode_delete.dragstart,
            cur->pos.map,
            T_SIDE | T_WALL | T_SECTOR | T_VERTEX | T_DECAL | T_OBJECT,
            E_MPIA_WHOLEWALL);

        dynlist_each(ptrs, it) {
            lptr_delete(ed->level, *it.el);
        }

        dynlist_free(ptrs);

        cur->mode_delete.drag = false;
    }
}

static void cm_delete_render(editor_t *ed, cursor_t *cur) {
    if (!cur->mode_delete.drag) {
        return;
    }

    sgp_set_color(1.0f, 0.2f, 0.2f, 0.2f);
    sgp_draw_filled_aabbf(
        aabbf_sort(
            AABBF_MM(
                cur->mode_delete.dragstart,
                cur->pos.map)));
}

static void cm_delete_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    snprintf(p, n, "%s", "DELETE");

    if (!LPTR_IS_NULL(cur->hover)) {
        char name[64];
        lptr_to_str(name, sizeof(name), ed->level, cur->hover);
        xnprintf(p, n, " %s", name);
    }
}

static bool cm_fuse_trigger(editor_t *ed, cursor_t *cur) {
    if (cur->mode == CM_DEFAULT
        && (ed->buttons[BUTTON_FUSE] & INPUT_RELEASE)) {
        cur->mode_fuse.first = LPTR_NULL;
        return true;
    }

    return false;
}

static void cm_fuse_update(editor_t *ed, cursor_t *cur) {
    if (!(ed->buttons[BUTTON_SELECT] & INPUT_RELEASE)) {
        return;
    } else if (!LPTR_IS(cur->hover, T_VERTEX)) {
        return;
    }

    if (LPTR_IS_NULL(cur->mode_fuse.first)) {
        cur->mode_fuse.first = cur->hover;
        return;
    }

    vertex_t *vs[2] = {
        LPTR_VERTEX(ed->level, cur->mode_fuse.first),
        LPTR_VERTEX(ed->level, cur->hover)
    };

    // if there is a wall connecting the vertices directly, remove it
    wall_t *sharewall = vertices_shared_wall(ed->level, vs[0], vs[1]);
    if (sharewall) {
        LOG("removing shared wall %d", sharewall->index);
        wall_delete(ed->level, sharewall);
    }

    // change all vertex connections from vx -> v0 to vx -> v1
    DYNLIST(wall_t*) walls = dynlist_copy(vs[0]->walls);

    dynlist_each(walls, it) {
        wall_t *wall = *it.el;

        // remove wall if vertices are already vx -> v1
        const int vindex = vs[0] == wall->v0 ? 0 : 1;

        // check if there already exists a wall from the other vertex to the
        // vertex we are fusing with
        if (vertices_shared_wall(
                ed->level,
                wall->vertices[1 - vindex],
                vs[1])) {
            wall_delete(ed->level, wall);
            LOG("wall already exists!");
        } else {
            wall_set_vertex(
                ed->level,
                wall,
                vindex,
                vs[1]);
        }
    }

    dynlist_free(walls);

    ASSERT(dynlist_size(vs[0]->walls) == 0);
    vertex_delete(ed->level, vs[0]);

    if (!(ed->buttons[BUTTON_MULTI_SELECT]
          & INPUT_DOWN)) {
        cur->mode = CM_DEFAULT;
    }

    cur->mode_fuse.first = LPTR_NULL;
}

static void cm_fuse_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    snprintf(p, n, "%s", "FUSE");

    if (!LPTR_IS_NULL(cur->mode_fuse.first)) {
        char name[64];
        lptr_to_str(name, sizeof(name), ed->level, cur->mode_fuse.first);
        xnprintf(p, n, " (%s WITH...)", name);
    }
}

static bool cm_split_trigger(editor_t *ed, cursor_t *cur) {
    return cur->mode == CM_DEFAULT
        && (ed->buttons[BUTTON_SPLIT] & INPUT_RELEASE);
}

static void cm_split_update(editor_t *ed, cursor_t *cur) {
    if (!(ed->buttons[BUTTON_SELECT] & INPUT_RELEASE)) {
        return;
    } else if (!LPTR_IS(cur->hover, T_VERTEX)) {
        return;
    }

    vertex_t *va = LPTR_VERTEX(ed->level, cur->hover);

    // try to find a valid location to split to which is:
    // * >0.025 units away
    // * doesn't already have a near a vertex
    // * not in a sector

    const f32 unit = 0.15f;

    ivec2s
        units =
            VEC_TO_I(glms_vec2_maxv(glms_vec2_divs(va->pos, unit), VEC2(0.0f))),
        ok_units = IVEC2(-1, -1);

    sector_t *last_sect = NULL;

    // search in a spiral pattern
    int i = 0, leg = 0, layer = 4;
    while (i < 128) {
        const vec2s pos = glms_vec2_scale(IVEC_TO_V(units), unit);
        f32 nearest_dist;
        level_nearest_vertex(ed->level, pos, &nearest_dist);

        if (nearest_dist < unit) {
            ok_units = units;

            if (!(last_sect =
                    level_find_point_sector(
                        ed->level, pos, last_sect))) {
                // no sector -> good spot
                goto done;
            }
        }

        switch (leg) {
        case 0:
            units.y++;
            if (+units.y == layer) { leg++; }
            break;
        case 1:
            units.x++;
            if (+units.x == layer) { leg++; }
            break;
        case 2:
            units.y--;
            if (-units.y == layer || units.y < 0) { leg++; }
            break;
        case 3:
            units.x--;
            if (-units.x == layer || units.x < 0) { leg = 0; layer++; }
            break;
        }

        units = glms_ivec2_clamp(units, IVEC2(0), IVEC2(INT_MAX));
        i++;
    }
done:;
    if (ok_units.x == -1 || ok_units.y == -1) {
        WARN("could not find good spot for vertex");
        cur->mode = CM_DEFAULT;
        return;
    }

    // create new vertex at okpos
    vertex_t *vb =
        vertex_new(ed->level, glms_vec2_scale(IVEC_TO_V(units), unit));

    // create wall between v and vnew, create sides where there is already a
    // sector
    wall_t *new_wall = wall_new(ed->level, va, vb);

    // calculate wall midpoint and normal
    const vec2s
        midpoint = wall_midpoint(new_wall),
        normal = new_wall->normal;

    for (int i = 0; i < 2; i++){
        sector_t *sect =
            level_find_point_sector(
                ed->level,
                glms_vec2_add(
                    midpoint,
                    glms_vec2_scale(normal, (i == 0 ? 1 : -1) * 0.01f)),
                NULL);

        side_t *side =
            sect ?
                side_new(ed->level, level_find_near_side(ed->level, va, sect))
                : NULL;

        if (side) {
            wall_set_side(ed->level, new_wall, i, side);
            sector_add_side(ed->level, sect, side);
        }
    }

    DYNLIST(wall_t*) to_move = NULL;

    // move walls which would be shorted going to vb from va -> vb
    dynlist_each(va->walls, it) {
        if (*it.el == new_wall) {
            continue;
        }

        // check if len(other -> vb) < len(other -> va)
        const int i = va == (*it.el)->v0 ? 1 : 0;
        const f32 lenb =
            glms_vec2_norm(
                glms_vec2_sub(
                    (*it.el)->vertices[i]->pos,
                    vb->pos));

        if (lenb < (*it.el)->len) {
            *dynlist_push(to_move) = *it.el;
        }
    }

    dynlist_each(to_move, it) {
        wall_set_vertex(ed->level, *it.el, va == (*it.el)->v0 ? 0 : 1, vb);
    }

    dynlist_free(to_move);

    if (!(ed->buttons[BUTTON_MULTI_SELECT] & INPUT_DOWN)) {
        cur->mode = CM_DEFAULT;
    }
}

static void cm_split_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    snprintf(p, n, "%s", "SPLIT");

    if (!LPTR_IS_NULL(cur->hover)) {
        char name[64];
        lptr_to_str(name, sizeof(name), ed->level, cur->hover);
        xnprintf(p, n, "  %s", name);
    }
}

// gets either side if ptr is side OR single side from wall with only one side
// side must have sector
// otherwise returns NULL
static side_t *cm_connect__side_from_ptr(editor_t *ed, lptr_t ptr) {
    side_t *side = NULL;

    if (LPTR_IS(ptr, T_SIDE)) {
        side = LPTR_SIDE(ed->level, ptr);
    } else if (LPTR_IS(ptr, T_WALL)) {
        wall_t *wall = LPTR_WALL(ed->level, ptr);

        if (wall->side0 && !wall->side1) {
            side = wall->side0;
        } else if (wall->side1 && !wall->side0) {
            side = wall->side1;
        }
    }

    return side && side->sector ? side : NULL;
}

static bool cm_connect_trigger(editor_t *ed, cursor_t *cur) {
    if (ed->buttons[BUTTON_CONNECT] & INPUT_PRESS) {
        cur->mode_connect.first = NULL;
        return true;
    }

    return false;
}

static void cm_connect_update(editor_t *ed, cursor_t *cur) {
    if (!(ed->buttons[BUTTON_SELECT] & INPUT_RELEASE)) {
        return;
    }

    side_t *side = cm_connect__side_from_ptr(ed, cur->hover);

    if (!side) {
        return;
    }

    if (!cur->mode_connect.first) {
        cur->mode_connect.first = side;
        return;
    }

    // connect first -> ptr
    cur->mode_connect.first->portal = side;
    side->portal = cur->mode_connect.first;

    side_recalculate(ed->level, cur->mode_connect.first);
    side_recalculate(ed->level, side);

    if (!(ed->buttons[BUTTON_MULTI_SELECT] & INPUT_DOWN)) {
        cur->mode = CM_DEFAULT;
    }
}

static void cm_connect_render(editor_t *ed, cursor_t *cur) {
    if (!cur->mode_connect.first) {
        return;
    }

    side_t *hoverside = cm_connect__side_from_ptr(ed, cur->hover);

    const vec4s color = ABGR_TO_VEC4(hoverside ? 0xFF20FF20 : 0xFF2020FF);
    sgp_set_color(SVEC4(color));
    sgp_draw_thick_line(
        SVEC2(side_normal_point(cur->mode_connect.first)),
        SVEC2(cur->pos.map),
        MAP_LINE_THICKNESS);
}

static void cm_connect_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    snprintf(p, n, "%s", "CONNECT");

    if (cur->mode_connect.first) {
        char name[32];
        lptr_to_str(
            name, sizeof(name), ed->level, LPTR_FROM(cur->mode_connect.first));
        xnprintf(p, n, " %s WITH", name);

        side_t *hoverside = cm_connect__side_from_ptr(ed, cur->hover);
        if (hoverside) {
            lptr_to_str(
                name, sizeof(name), ed->level, LPTR_FROM(hoverside));
            xnprintf(p, n, " %s", name);
        } else {
            xnprintf(p, n, "...");

        }
    } else {
        xnprintf(p, n, "...");
    }
}

static bool cm_decal_trigger(editor_t *ed, cursor_t *cur) {
    return ed->buttons[BUTTON_NEW_DECAL] & INPUT_PRESS;
}

static void cm_decal_update(editor_t *ed, cursor_t *cur) {
    if (!(ed->buttons[BUTTON_SELECT] & INPUT_PRESS)) {
        return;
    }

    side_t *side = NULL;
    f32 x;
    sector_t *sector = NULL;
    vec2s pos, world_pos;
    decal_placement_pos(ed, cur->pos.map, &side, &x, &sector, &pos, &world_pos);

    if (side) {
        decal_t *d = decal_new(ed->level);
        d->side.offsets =
            VEC2(
                x,
                side->sector ?
                    ((side->sector->ceil.z - side->sector->floor.z) / 2)
                    : 0);
        decal_set_side(ed->level, d, side);
    } else if (sector) {
        decal_t *d = decal_new(ed->level);
        d->sector.pos = pos;
        decal_set_sector(ed->level, d, sector, PLANE_TYPE_FLOOR);
    } else {
        return;
    }

    if (!(ed->buttons[BUTTON_MULTI_SELECT] & INPUT_DOWN)) {
        cur->mode = CM_DEFAULT;
    }
}

static void cm_decal_render(editor_t *ed, cursor_t *cur) {
    side_t *side = NULL;
    f32 x;
    sector_t *sector = NULL;
    vec2s pos, world_pos;
    decal_placement_pos(ed, cur->pos.map, &side, &x, &sector, &pos, &world_pos);

    if (side || sector) {
        sgp_set_color(SVEC4(MAP_DECAL_COLOR));
    } else {
        sgp_set_color(0.8f, 0.2f, 0.2f, 0.6f);
    }

    sgp_draw_filled_rect(
        SVEC2(glms_vec2_sub(world_pos, VEC2(MAP_DECAL_SIZE / 2))),
        SVEC2(VEC2(MAP_DECAL_SIZE)));
}

static void cm_decal_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    snprintf(p, n, "%s", "NEW DECAL");
}

static bool cm_object_trigger(editor_t *ed, cursor_t *cur) {
    return ed->buttons[BUTTON_NEW_OBJECT] & INPUT_PRESS;
}

static void cm_object_update(editor_t *ed, cursor_t *cur) {
    if (!(ed->buttons[BUTTON_SELECT] & INPUT_PRESS)) {
        return;
    }

    // create object
    object_t *o = object_new(ed->level);
    object_move(ed->level, o, cur->pos.map);

    if (!(ed->buttons[BUTTON_MULTI_SELECT] & INPUT_DOWN)) {
        cur->mode = CM_DEFAULT;
    }
}

static void cm_object_render(editor_t *ed, cursor_t *cur) {
    const vec2s pos = cur->pos.map;
    sgp_set_color(SVEC4(MAP_OBJECT_COLOR));
    sgp_draw_filled_rect(
        SVEC2(glms_vec2_sub(pos, VEC2(MAP_OBJECT_SIZE / 2))),
        SVEC2(VEC2(MAP_OBJECT_SIZE)));
}

static void cm_object_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    snprintf(p, n, "%s", "NEW OBJECT");
}

static bool cm_move_decal_trigger(editor_t *ed, cursor_t *cur) {
    if (ed->mode == EDITOR_MODE_CAM
        && (ed->buttons[BUTTON_SELECT] & INPUT_PRESS)
        && LPTR_IS(ed->cam.ptr, T_DECAL)) {
        cur->mode_move_decal.decal = LPTR_DECAL(ed->level, ed->cam.ptr);
        return true;
    }

    return false;
}

static void cm_move_decal_update(editor_t *ed, cursor_t *cur) {
    if (!(ed->buttons[BUTTON_SELECT] & INPUT_DOWN)) {
        cur->mode = CM_DEFAULT;
    }

    if (!LPTR_IS(ed->cam.ptr, T_SECTOR | T_SIDE | T_DECAL)) {
        return;
    }

    decal_t *decal = cur->mode_move_decal.decal;

    if (ed->cam.side) {
        if (!decal->is_on_side || ed->cam.side != decal->side.ptr) {
            decal_set_side(ed->level, decal, ed->cam.side);
        }

        decal->side.offsets = ed->cam.side_pos;
        decal_recalculate(ed->level, decal);
    } else if (ed->cam.sect) {
        decal_set_sector(
            ed->level,
            decal,
            ed->cam.sect,
            ed->cam.plane);

        decal->sector.pos = ed->cam.sect_pos;
        decal_recalculate(ed->level, decal);
    }
}

static void cm_move_decal_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    snprintf(p, n, "%s", "MOVE DECAL");
}

static bool cm_move_object_trigger(editor_t *ed, cursor_t *cur) {
    if (ed->mode == EDITOR_MODE_CAM
        && (ed->buttons[BUTTON_SELECT] & INPUT_PRESS)
        && LPTR_IS(ed->cam.ptr, T_OBJECT)) {
        cur->mode_move_object.object = LPTR_OBJECT(ed->level, ed->cam.ptr);
        LOG("trigger!");
        return true;
    }

    return false;
}

static void cm_move_object_update(editor_t *ed, cursor_t *cur) {
    if (!(ed->buttons[BUTTON_SELECT] & INPUT_DOWN)) {
        cur->mode = CM_DEFAULT;
        return;
    }

    if (!LPTR_IS(ed->cam.ptr, T_SECTOR)) {
        return;
    }

    // get world point from sector
    object_t *object = cur->mode_move_object.object;
    object_move(ed->level, object, ed->cam.data.sector.pos);
}

static void cm_move_object_status_line(
    editor_t *ed,
    cursor_t *cur,
    char *p,
    usize n) {
    snprintf(p, n, "%s", "MOVE OBJECT");
}

/* CURSOR MODE TEMPLATE
 *
 * static bool cm_replaceme_trigger(editor_t *ed, cursor_t *cur) {
 *     return false;
 * }
 *
 * static void cm_replaceme_update(editor_t *ed, cursor_t *cur) {
 *
 * }
 *
 * static void cm_replaceme_render(editor_t *ed, cursor_t *cur) {
 *
 * }
 *
 * static void cm_replaceme_status_line(
 *     editor_t *ed,
 *     cursor_t *cur,
 *     char *p,
 *     usize n) {
 * }
 */

cursor_mode_t CURSOR_MODES[CM_COUNT] = {
    [CM_DEFAULT] = {
        .mode = CM_DEFAULT,
        .flags = CMF_CLICK_CANCEL,
        .priority = 0,
        .trigger = NULL,
        .update = NULL,
        .render = NULL,
        .status_line = cm_default_status_line,
        .cancel = NULL
    },
    [CM_SELECT] = {
        .mode = CM_SELECT,
        .flags = CMF_CAM | CMF_EXPLICIT_CANCEL,
        .priority = 1,
        .trigger = NULL,
        .update = cm_select_id_update,
        .render = NULL,
        .status_line = cm_select_id_status_line,
        .cancel = NULL
    },
    [CM_DRAG] = {
        .mode = CM_DRAG,
        .flags = CMF_CLICK_CANCEL,
        .priority = 0,
        .trigger = cm_drag_trigger,
        .update = cm_drag_update,
        .render = NULL,
        .status_line = cm_drag_status_line,
        .cancel = cm_drag_cancel
    },
    [CM_MOVE_DRAG] = {
        .mode = CM_MOVE_DRAG,
        .flags = CMF_NONE,
        .priority = -2,
        .trigger = cm_move_drag_trigger,
        .update = cm_move_drag_update,
        .render = NULL,
        .status_line = cm_move_drag_status_line,
        .cancel = NULL
    },
    [CM_SELECT_AREA] = {
        .mode = CM_SELECT_AREA,
        .flags = CMF_NOHOVER,
        .priority = -1,
        .trigger = cm_select_area_trigger,
        .update = cm_select_area_update,
        .render = cm_select_area_render,
        .status_line = cm_select_area_status_line,
        .cancel = cm_select_area_cancel
    },
    [CM_WALL] = {
        .mode = CM_WALL,
        .flags = CMF_NODRAG,
        .priority = 0,
        .trigger = cm_wall_trigger,
        .update = cm_wall_update,
        .render = cm_wall_render,
        .status_line = cm_wall_status_line,
        .cancel = NULL
    },
    [CM_VERTEX] = {
        .mode = CM_VERTEX,
        .flags = CMF_NODRAG,
        .priority = 0,
        .trigger = cm_vertex_trigger,
        .update = cm_vertex_update,
        .render = cm_vertex_render,
        .status_line = cm_vertex_status_line,
        .cancel = NULL
    },
    [CM_SIDE] = {
        .mode = CM_SIDE,
        .flags = CMF_NODRAG,
        .priority = 0,
        .trigger = cm_side_trigger,
        .update = cm_side_update,
        .render = cm_side_render,
        .status_line = cm_side_status_line,
        .cancel = NULL
    },
    [CM_EZPORTAL] = {
        .mode = CM_EZPORTAL,
        .flags = CMF_NODRAG,
        .priority = 0,
        .trigger = cm_ezportal_trigger,
        .update = cm_ezportal_update,
        .render = cm_ezportal_render,
        .status_line = cm_ezportal_status_line,
        .cancel = NULL
    },
    [CM_SECTOR] = {
        .mode = CM_SECTOR,
        .flags = CMF_NODRAG,
        .priority = 0,
        .trigger = cm_sector_trigger,
        .update = cm_sector_update,
        .render = cm_sector_render,
        .status_line = cm_sector_status_line,
        .cancel = NULL
    },
    [CM_DELETE] = {
        .mode = CM_DELETE,
        .flags = CMF_NODRAG,
        .priority = 0,
        .trigger = cm_delete_trigger,
        .update = cm_delete_update,
        .render = cm_delete_render,
        .status_line = cm_delete_status_line,
        .cancel = NULL
    },
    [CM_FUSE] = {
        .mode = CM_FUSE,
        .flags = CMF_NODRAG,
        .priority = 0,
        .trigger = cm_fuse_trigger,
        .update = cm_fuse_update,
        .render = NULL,
        .status_line = cm_fuse_status_line,
        .cancel = NULL
    },
    [CM_SPLIT] = {
        .mode = CM_SPLIT,
        .flags = CMF_NODRAG,
        .priority = 0,
        .trigger = cm_split_trigger,
        .update = cm_split_update,
        .render = NULL,
        .status_line = cm_split_status_line,
        .cancel = NULL
    },
    [CM_CONNECT] = {
        .mode = CM_CONNECT,
        .flags = CMF_NODRAG,
        .priority = 0,
        .trigger = cm_connect_trigger,
        .update = cm_connect_update,
        .render = cm_connect_render,
        .status_line = cm_connect_status_line,
        .cancel = NULL
    },
    [CM_DECAL] = {
        .mode = CM_DECAL,
        .flags = CMF_NODRAG,
        .priority = 0,
        .trigger = cm_decal_trigger,
        .update = cm_decal_update,
        .render = cm_decal_render,
        .status_line = cm_decal_status_line,
        .cancel = NULL
    },
    [CM_OBJECT] = {
        .mode = CM_OBJECT,
        .flags = CMF_NODRAG,
        .priority = 0,
        .trigger = cm_object_trigger,
        .update = cm_object_update,
        .render = cm_object_render,
        .status_line = cm_object_status_line,
        .cancel = NULL
    },
    [CM_MOVE_DECAL] = {
        .mode = CM_MOVE_DECAL,
        .flags = CMF_NODRAG | CMF_CAM,
        .priority = 0,
        .trigger = cm_move_decal_trigger,
        .update = cm_move_decal_update,
        .render = NULL,
        .status_line = cm_move_decal_status_line,
        .cancel = NULL
    },
    [CM_MOVE_OBJECT] = {
        .mode = CM_MOVE_OBJECT,
        .flags = CMF_NODRAG | CMF_CAM,
        .priority = 0,
        .trigger = cm_move_object_trigger,
        .update = cm_move_object_update,
        .render = NULL,
        .status_line = cm_move_object_status_line,
        .cancel = NULL
    },
};
