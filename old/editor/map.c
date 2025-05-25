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
#include "util/input.h"
#include "util/aabb.h"
#include "gfx/sokol.h"
#include "gfx/renderer.h"
#include "state.h"

static vec2s center_for_ptr(editor_t *ed, lptr_t ptr) {
    switch (LPTR_TYPE(ptr)) {
    case T_VERTEX:
        return LPTR_VERTEX(ed->level, ptr)->pos;
    case T_WALL:
        return wall_midpoint(LPTR_WALL(ed->level, ptr));
    case T_SIDE:
        return center_for_ptr(ed, LPTR_FROM(LPTR_SIDE(ed->level, ptr)->wall));
    case T_SECTOR: {
        sector_t *s = LPTR_SECTOR(ed->level, ptr);
        return glms_vec2_lerp(s->min, s->max, 0.5f);
    }
    case T_DECAL:
        return decal_worldpos(LPTR_DECAL(ed->level, ptr));
    case T_OBJECT:
        return LPTR_OBJECT(ed->level, ptr)->pos;
    default: ASSERT(false);
    }
}

static void draw_friends(editor_t *ed, lptr_t ptr, u32 color) {
    if (!(LPTR_TYPE(ptr) & T_HAS_TAG)) {
        return;
    }

    int *ptag = lptr_ptag(ed->level, ptr);
    const vec2s center = center_for_ptr(ed, ptr);

    DYNLIST(lptr_t) *friends = &ed->level->tag_lists[*ptag];

    dynlist_each(*friends, it) {
        if (LPTR_EQ(ptr, *it.el)) { continue; }

        sgp_draw_thick_line(
            SVEC2(center),
            SVEC2(center_for_ptr(ed, *it.el)),
            ed->map.scale);
    }
}

// get draw color for lptr_t
static vec4s get_color_for_ptr(
    editor_t *ed,
    lptr_t ptr,
    vec4s normal,
    vec4s select,
    vec4s hover) {
    if (LPTR_IS_NULL(ptr)) { return VEC4(1); }

    vec4s color = normal;
    bool highlight = false;

    if (editor_map_is_select(ed, ptr)) {
        color = select;
        goto done;
    } else if (LPTR_EQ(ptr, ed->cur.hover)) {
        color = hover;
        goto done;
    } else if (editor_ptr_is_highlight(ed, ptr)) {
        highlight = true;
    }

    switch (LPTR_TYPE(ptr)) {
    case T_SIDE: {
        side_t *side = LPTR_SIDE(ed->level, ptr);
        if (side->portal) {
            if (side->portal == side_other(side)) {
                color = MAP_SIDE_PORTAL_COLOR;
            } else {
                color = MAP_SIDE_PORTAL_DISCONNECT_COLOR;
            }
        }
    } break;
    case T_WALL: {
        wall_t *wall = LPTR_WALL(ed->level, ptr);

        // show vertex connections
        for (int i = 0; i < 2; i++) {
            if (LPTR_EQ(ed->cur.hover, LPTR_FROM(wall->vertices[i]))
                || editor_ptr_is_highlight(ed, LPTR_FROM(wall->vertices[i]))) {
                color = MAP_WALL_CONNECT_COLOR;
            }
        }

        // show connection to side or sector highlight
        for (int i = 0; i < 2; i++) {
            side_t *side = wall->sides[i];
            if (!side) { continue; }

            if (editor_map_is_select(ed, LPTR_FROM(side))) {
                color = select;
            } else if (
                LPTR_EQ(ed->cur.hover, LPTR_FROM(side))
                || editor_ptr_is_highlight(ed, LPTR_FROM(side))) {
                color = hover;
            } else if (
                side->sector
                && (ed->cur.sector == side->sector
                    || editor_ptr_is_highlight(ed, LPTR_FROM(side->sector)))) {
                color = color_scale_rgb(color, 0.5f + 0.5f * editor_highlight_value(ed));
            }
        }
    } break;
    }

done:
    // even "done" colors must be highlighted
    if (highlight) {
        color = color_scale_rgb(color, 0.5f + 0.5f * editor_highlight_value(ed));
    }

    return color;
}

static void set_color_for_ptr(
    editor_t *ed,
    lptr_t ptr,
    vec4s normal,
    vec4s select,
    vec4s hover) {
    const vec4s color = get_color_for_ptr(ed, ptr, normal, select, hover);
    sgp_set_color(SVEC4(color));
}

void editor_map_select(editor_t *ed, lptr_t ptr)  {
    if (LPTR_IS_NULL(ptr)) {
        return;
    }

    dynlist_each(ed->map.selected, it) {
        if (LPTR_EQ(*it.el, ptr)) {
            return;
        }
    }

    char name[128];
    lptr_to_str(name, sizeof(name), ed->level, ptr);
    LOG("SELECTING %s", name);

    *dynlist_push(ed->map.selected) = ptr;
}

void editor_map_deselect(editor_t *ed, lptr_t ptr) {
    dynlist_each(ed->map.selected, it) {
        if (LPTR_EQ(*it.el, ptr)) {
            dynlist_remove_it(ed->map.selected, it);
            return;
        }
    }
}

void editor_map_clear_select(editor_t *ed) {
    dynlist_free(ed->map.selected);
}

bool editor_map_is_select(editor_t *ed, lptr_t ptr)  {
    dynlist_each(ed->map.selected, it) {
        if (LPTR_EQ(*it.el, ptr)) {
            return true;
        }
    }

    return false;
}

vec2s editor_map_snap_to_grid(editor_t *ed, vec2s p) {
    return glms_vec2_scale(glms_vec2_round(glms_vec2_divs(p, ed->map.gridsize)), ed->map.gridsize);
}

vec2s editor_map_transform(editor_t *ed, vec2s p) {
    return VEC2(
        (p.x - ed->map.pos.x) * (ed->map.scale),
        (p.y - ed->map.pos.y) * (ed->map.scale));
}

vec2s editor_map_to_screen(editor_t *ed, vec2s p) {
    // TODO: TODO: broken
    const f32 ratio = ed->size.x / (f32) ed->size.y;
    return VEC2(
        (p.x * ((EDITOR_BASE_SCALE / ed->map.scale) / ed->size.x * ratio)) + ed->map.pos.x,
        (p.y * ((EDITOR_BASE_SCALE / ed->map.scale) / ed->size.y)) + ed->map.pos.y);
}

vec2s editor_screen_to_map(editor_t *ed, vec2s p) {
    // TODO: cleanup
    const f32 ratio = ed->size.x / (f32) ed->size.y;
    return VEC2(
        (p.x * ((EDITOR_BASE_SCALE / ed->map.scale) / ed->size.x * ratio)) + ed->map.pos.x,
        (p.y * ((EDITOR_BASE_SCALE / ed->map.scale) / ed->size.y)) + ed->map.pos.y);
}

void editor_map_center_on(editor_t *ed, vec2s p) {
    const vec2s
        center = editor_screen_to_map(ed, glms_vec2_divs(IVEC_TO_V(ed->size), 2.0f)),
        bottom_left = editor_screen_to_map(ed, VEC2(0));

    ed->map.pos = glms_vec2_sub(p, glms_vec2_sub(center, bottom_left));
}

void editor_map_set_scale(editor_t *ed, f32 newscale) {
    newscale = clamp(newscale, MAP_SCALE_MIN, MAP_SCALE_MAX);

    // snap to power of two
    int exp;
    frexp(newscale, &exp);
    newscale = powf(2.0f, exp - 1);

    const vec2s center =
        editor_screen_to_map(
            ed,
            VEC2(
                ed->size.x / 2.0f,
                ed->size.y / 2.0f
            ));

    ed->map.scale = newscale;
    editor_map_center_on(ed, center);
}

int editor_map_ptrs_in_area(
    editor_t *ed,
    DYNLIST(lptr_t) *dst,
    vec2s a,
    vec2s b,
    int tag,
    int flags) {
    const int nstart = dynlist_size(*dst);

    if (tag & T_VERTEX) {
        level_dynlist_each(ed->level->vertices, it) {
            vertex_t *v = *it.el;

            if (point_in_box(v->pos, a, b)) {
                *dynlist_push(*dst) = LPTR_FROM(v);
            }
        }
    }

    if (tag & (T_WALL | T_SIDE)) {
        level_dynlist_each(ed->level->walls, it) {
            wall_t *w = *it.el;

            const vec2s hit =
                intersect_seg_box(
                    w->v0->pos,
                    w->v1->pos,
                    a,
                    b);

            const bool
                i0 = point_in_box(w->v0->pos, a, b),
                i1 = point_in_box(w->v1->pos, a, b);

            const bool inside =
                (flags & E_MPIA_WHOLEWALL) ? (i0 && i1) : (i0 || i1);

            if (inside || (!(flags & E_MPIA_WHOLEWALL) && !isnan(hit.x))) {
                if (tag & T_WALL) {
                    *dynlist_push(*dst) = LPTR_FROM(w);
                } else {
                    for (int j = 0; j < 2; j++) {
                        if (w->sides[j]) {
                            *dynlist_push(*dst) = LPTR_FROM(w->sides[j]);
                        }
                    }
                }
            }
        }
    }

    if (tag & T_OBJECT) {
        level_dynlist_each(ed->level->objects, it) {
            if (point_in_box((*it.el)->pos, a, b)) {
                *dynlist_push(*dst) = LPTR_FROM(*it.el);
            }
        }
    }


    if (tag & T_DECAL) {
        level_dynlist_each(ed->level->decals, it) {
            if (point_in_box(decal_worldpos(*it.el), a, b)) {
                *dynlist_push(*dst) = LPTR_FROM(*it.el);
            }
        }
    }

    return dynlist_size(*dst) - nstart;
}

lptr_t editor_map_locate(editor_t *ed, vec2s pos, sector_t **sector_out) {
    // TODO: use blocks for faster searching

    lptr_t ptr = LPTR_NULL;
    f32 dist = 1e10;
    sector_t *sector = NULL;

    // find sector
    level_dynlist_each(ed->level->sectors, it) {
        sector_t *sect = *it.el;

        if (sector_contains_point(sect, pos)) {
            sector = sect;
            ptr = LPTR_FROM(sect);
            break;
        }
    }

    // check objects
    level_dynlist_each(ed->level->objects, it) {
        object_t *object = *it.el;

        const f32 d = glms_vec2_norm(glms_vec2_sub(pos, object->pos));
        if (d < dist && d <= MAP_OBJECT_SIZE) {
            ptr = LPTR_FROM(object);
            dist = d;
        }
    }

    if (LPTR_IS(ptr, T_OBJECT) && !LPTR_IS_NULL(ptr)) { goto done; }

    // check decals
    level_dynlist_each(ed->level->decals, it) {
        decal_t *decal = *it.el;

        const vec2s p = decal_worldpos(decal);
        const f32 d = glms_vec2_norm(glms_vec2_sub(pos, p));
        if (d <= dist && d <= MAP_DECAL_SIZE) {
            ptr = LPTR_FROM(decal);
            dist = d;
        }
    }

    if (LPTR_IS(ptr, T_DECAL) && !LPTR_IS_NULL(ptr)) { goto done; }

    // check vertices
    level_dynlist_each(ed->level->vertices, it) {
        vertex_t *v = *it.el;

        const f32 d = glms_vec2_norm(glms_vec2_sub(pos, v->pos));
        if (d <= dist && d <= MAP_VERTEX_SIZE) {
            ptr = LPTR_FROM(v);
            dist = d;
        }
    }

    if (LPTR_IS(ptr, T_VERTEX) && !LPTR_IS_NULL(ptr)) { goto done; }

    // check walls
    level_dynlist_each(ed->level->walls, it) {
        wall_t *wall = *it.el;

        const f32 d =
            point_to_segment(
                pos,
                wall->v0->pos,
                wall->v1->pos);

        const int side =
            sign(
                point_side(
                    pos,
                    wall->v0->pos,
                    wall->v1->pos));

        if (d <= dist && d < MAP_SIDE_SELECT_DIST_FOR_SCALE(ed->map.scale)) {
            // check if directly on side normal
            const vec2s
                midpoint = wall_midpoint(wall),
                normal = glms_vec2_scale(
                    wall->normal,
                    side * MAP_NORMAL_LENGTH_FOR_SCALE(ed->map.scale)),
                npoint = glms_vec2_add(midpoint, normal),
                left = VEC2(-normal.y, normal.x),
                phleft = glms_vec2_scale(left, 0.5f * MAP_LINE_THICKNESS),
                nhleft = glms_vec2_scale(phleft, -1.0f);

            const aabbf_t box =
                aabbf_scale_center(
                    aabbf_sort(
                        AABBF_MM(
                            glms_vec2_add(midpoint, nhleft),
                            glms_vec2_add(npoint, phleft))),
                    VEC2(2.0f, 2.0f));

            side_t *s = wall->sides[side > 0 ? 0 : 1];
            if (s && aabbf_contains(box, pos)) {
                ptr = LPTR_FROM(s);
                dist = 0.0f;
            }

            // select wall if close, otherwise select side
            if (d < MAP_WALL_SELECT_DIST) {
                ptr = LPTR_FROM(wall);
                dist = d;
            } else if (side > 0 && wall->side0) {
                ptr = LPTR_FROM(wall->side0);
                dist = d;
            } else if (side <= 0 && wall->side1) {
                ptr = LPTR_FROM(wall->side1);
                dist = d;
            }
        }
    }

    if (LPTR_IS(ptr, T_SIDE | T_WALL) && !LPTR_IS_NULL(ptr)) { goto done; }

done:
    if (sector_out) { *sector_out = sector; }
    return ptr;
}

// draw sector filled with color
// if floor is enabled, color is layered on top of floor
static void sector_draw_filled(
    editor_t *ed,
    sector_t *sect,
    vec4s color) {
    // check if sector is on screen at all
    const vec2s
        pmin = editor_map_transform(ed, sect->min),
        pmax = editor_map_transform(ed, sect->max);

    if (pmax.x < 0
        || pmax.y < 0
        || pmin.x >= ed->size.x
        || pmin.y >= ed->size.x) {
        return;
    }

    sgp_set_color(SVEC4(color));
    dynlist_each(sect->tris, it) {
        sgp_draw_filled_triangle(
            SVEC2(it.el->a->pos),
            SVEC2(it.el->b->pos),
            SVEC2(it.el->c->pos));
    }
}

void editor_map_frame(editor_t *ed) {
    sgp_set_blend_mode(SGP_BLENDMODE_BLEND);

    sgp_viewport(0, 0, ed->size.x, ed->size.y);
    sgp_project(
        0.0f,
        (EDITOR_BASE_SCALE / ed->map.scale) * (ed->size.x / (f32) ed->size.y),
        (EDITOR_BASE_SCALE / ed->map.scale),
        0.0f);

    sgp_set_color(0.0f, 0.0f, 0.0f, 1.0f);
    sgp_clear();

    sgp_push_transform();
    sgp_translate(SVEC2(glms_vec2_scale(ed->map.pos, -1)));
    /* LOG("pos is %" PRIv2, FMTv2(ed->map.pos)); */

    // draw grid
    if (ed->visopt & VISOPT_GRID) {
#define MAX_GRID_POINTS 16384
        const f32 g = ed->map.gridsize;
        const vec2s s =
            editor_map_snap_to_grid(ed, glms_vec2_sub(ed->map.pos, VEC2(g)));

        const vec2s bounds = editor_screen_to_map(ed, IVEC_TO_V(ed->size));

        const f32 size =
            min(MAP_GRID_POINT_SIZE / ed->map.scale, MAP_GRID_POINT_SIZE);

        int i = 0;
        sgp_set_color(SVEC4(MAP_GRID_COLOR));
        for (
            f32 y = s.y;
            i < MAX_GRID_POINTS && y <= bounds.y;
            y += g) {
            for (
                f32 x = s.x;
                i < MAX_GRID_POINTS && x <= bounds.x;
                x += g) {
                if (x < 0 || y < 0) { continue; }

                i++;
                sgp_draw_filled_rect(
                    x - (size / 2.0f),
                    y - (size / 2.0f),
                    size,
                    size);
            }
        }
    }

    // all are ID_NONE if there is no hover allowed for current cursor mode
    if (CURSOR_MODES[ed->cur.mode].flags & CMF_NOHOVER) {
        ed->cur.hover = LPTR_NULL;
        ed->cur.sector = NULL;
    } else {
        ed->cur.hover =
            editor_map_locate(ed, ed->cur.pos.map, &ed->cur.sector);
    }

    char buf[256];
    lptr_to_str(buf, sizeof(buf), ed->level, ed->cur.hover);
    /* LOG("hover: %s", buf); */
    /* LOG("pos screen is %" PRIv2, FMTv2(ed->cur.pos.screen)); */
    /* LOG("pos map is %" PRIv2, FMTv2(ed->cur.pos.map)); */

    // show currently hovered sector
    if ((ed->visopt & VISOPT_HOVERSECT) && ed->cur.sector) {
        sector_draw_filled(ed, ed->cur.sector, MAP_SECTOR_HIGHLIGHT);
    }

    // draw walls
    level_dynlist_each(ed->level->walls, it) {
        wall_t *wall = *it.el;

        // primary wall line
        set_color_for_ptr(
                ed,
                LPTR_FROM(wall),
                MAP_WALL_COLOR,
                MAP_SELECT_COLOR,
                MAP_WALL_HOVER_COLOR);
        sgp_draw_thick_line(
            SVEC2(wall->v0->pos),
            SVEC2(wall->v1->pos),
            MAP_LINE_THICKNESS);

        // draw side normal lines
        const vec2s midpoint = wall_midpoint(wall);

        for (int i = 0; i < 2; i++) {
            side_t *s = wall->sides[i];
            if (!s) { continue; }

            const int sign = i == 0 ? 1 : -1;

            set_color_for_ptr(
                ed,
                LPTR_FROM(s),
                MAP_SIDE_NORMAL_COLOR,
                MAP_SIDE_SELECT_COLOR,
                MAP_SIDE_HOVER_COLOR);

            const vec2s endpoint =
                glms_vec2_add(
                    midpoint,
                    glms_vec2_scale(
                        wall->normal,
                        sign * MAP_NORMAL_LENGTH_FOR_SCALE(ed->map.scale)));
            sgp_draw_thick_line(
                SVEC2(midpoint),
                SVEC2(endpoint),
                MAP_LINE_THICKNESS);
        }
    }

    // draw vertices
    level_dynlist_each(ed->level->vertices, it) {
        vertex_t *v = *it.el;

        const f32 radius = MAP_VERTEX_SIZE / 2.0f;

        set_color_for_ptr(
            ed,
            LPTR_FROM(v),
            MAP_VERTEX_COLOR,
            MAP_SELECT_COLOR,
            MAP_VERTEX_HOVER_COLOR);

        sgp_draw_filled_rect(
            SVEC2(glms_vec2_sub(v->pos, VEC2(radius))),
            SVEC2(VEC2(2 * radius)));
    }

    // draw decals
    level_dynlist_each(ed->level->decals, it) {
        decal_t *decal = *it.el;

        const vec2s pos = decal_worldpos(decal);
        vec2s normal, endpoint;

        if (decal->is_on_side) {
            normal = side_normal(decal->side.ptr),
            endpoint =
                glms_vec2_add(
                    pos, glms_vec2_scale(normal, MAP_DECAL_SIZE * 1.0f));
        } else {
            normal = pos;
            endpoint = pos;
        }

        set_color_for_ptr(
            ed,
            LPTR_FROM(decal),
            MAP_DECAL_COLOR,
            MAP_SELECT_COLOR,
            MAP_DECAL_HOVER_COLOR);

        sgp_draw_filled_rect(
            SVEC2(glms_vec2_sub(pos, VEC2(MAP_DECAL_SIZE / 2))),
            SVEC2(VEC2(MAP_DECAL_SIZE)));

        sgp_set_color(SVEC4(VEC4(1.0f)));
        sgp_draw_thick_line(
            SVEC2(pos), SVEC2(endpoint), MAP_LINE_THICKNESS * 0.75f);
    }

    // draw objects
    level_dynlist_each(ed->level->objects, it) {
        object_t *object = *it.el;

        set_color_for_ptr(
            ed,
            LPTR_FROM(object),
            MAP_OBJECT_COLOR,
            MAP_SELECT_COLOR,
            MAP_OBJECT_HOVER_COLOR);

        // TODO: draw objects with no sector (illegal) in red
        sgp_draw_filled_rect(
            SVEC2(glms_vec2_sub(object->pos, VEC2(MAP_OBJECT_SIZE / 2))),
            SVEC2(VEC2(MAP_OBJECT_SIZE)));
    }

    // draw connecting sectors for non-contiguous portals
    if (LPTR_IS(ed->cur.hover, T_SIDE)) {
        side_t *side = LPTR_SIDE(ed->level, ed->cur.hover);
        if (side->flags & SIDE_FLAG_DISCONNECT) {
            // draw connection
            /* draw_line( */
            /*     &ed->map.drawsurf, */
            /*     editor_map_transform(ed, side_normal_point(side)), */
            /*     editor_map_transform(ed, side_normal_point(side->portal)), */
            /*     MAP_SIDE_PORTAL_DISCONNECT_COLOR, */
            /*     ed->map.wholescale); */
        }
    }

    if (ed->visopt & VISOPT_GEO) {
        level_dynlist_each(ed->level->sectors, it) {
            sector_t *s = *it.el;

            if ((ed->visopt & VISOPT_HOVERSECT)
                && s != ed->cur.sector) {
                continue;
            }

            sgp_set_color(0.3f, 0.3f, 0.3f, 0.7f);
            dynlist_each(s->tris, it) {
                sgp_draw_thick_line(
                    SVEC2(it.el->a->pos), SVEC2(it.el->b->pos),
                    MAP_LINE_THICKNESS * 0.5f);
                sgp_draw_thick_line(
                    SVEC2(it.el->b->pos), SVEC2(it.el->c->pos),
                    MAP_LINE_THICKNESS * 0.5f);
                sgp_draw_thick_line(
                    SVEC2(it.el->c->pos), SVEC2(it.el->a->pos),
                    MAP_LINE_THICKNESS * 0.5f);
            }

            dynlist_each(s->subs, it) {
                dynlist_each(it.el->lines, it_l) {
                    sgp_set_color(0.6f, 0.7f, 1.0f, 0.5f);
                    sgp_draw_thick_line(
                        SVEC2(it_l.el->a->pos), SVEC2(it_l.el->b->pos),
                        MAP_LINE_THICKNESS * 0.5f);
                }

                sgp_set_color(0.5f, 0.5f, 0.5f, 0.7f);
                sgp_draw_aabbf(
                    AABBF_MM(it.el->min, it.el->max),
                    MAP_LINE_THICKNESS * 0.5f);
            }
        }
    }

    if (ed->visopt & VISOPT_SUBNEIGHBORS) {
        if (ed->cur.sector) {
            subsector_t *sub =
                sector_find_subsector(
                    ed->cur.sector,
                    ed->cur.pos.map);


            dynlist_each(sub->neighbors, it) {
                /* printf("%d, ", it.el->id); */
                dynlist_each(ed->level->subsectors[it.el->id]->lines, it_l) {
                    if (sect_line_eq(it_l.el, it.el->line)) {
                        sgp_set_color(1.0f, 1.0f, 1.0f, 1.0f);
                    } else {
                        sgp_set_color(0.8f, 0.8f, 0.2f, 0.5f);
                    }

                    sgp_draw_thick_line(
                        SVEC2(it_l.el->a->pos), SVEC2(it_l.el->b->pos),
                        MAP_LINE_THICKNESS * 0.5f);
                }
            }
            /* printf("\n"); */
        }
    }

    if (ed->visopt & VISOPT_PVS) {
        if (ed->cur.sector) {
            DYNLIST(sector_t*) visible = NULL;
            level_get_visible_sectors(
                ed->level, ed->cur.sector, &visible,
                LEVEL_GET_VISIBLE_SECTORS_NONE);

            dynlist_each(visible, it) {
                llist_each(sector_sides, &(*it.el)->sides, it) {
                    sgp_set_color(0.2f, 0.6f, 1.0f, 0.8f);
                    sgp_draw_thick_line(
                        SVEC2(it.el->wall->v0->pos),
                        SVEC2(it.el->wall->v1->pos),
                        MAP_LINE_THICKNESS);
                }
            }

            dynlist_free(visible);
        }
    }

    // draw hover friends
    if ((ed->visopt & VISOPT_FRIENDS)
        && (LPTR_TYPE(ed->cur.hover) & T_HAS_TAG)) {
        /* draw_friends( */
        /*     ed, */
        /*     ed->cur.hover, */
        /*     MAP_FRIEND_COLOR); */
    }

    // draw previous/following sides
    if ((ed->visopt & VISOPT_SIDELIST) && LPTR_IS(ed->cur.hover, T_SIDE)) {
        /* side_t *s = LPTR_SIDE(ed->level, ed->cur.hover); */

        /* if (s->sector) { */
        /*     bool hit = false; */
        /*     llist_each(sector_sides, &s->sector->sides, it) { */
        /*         if (it.el == s) { hit = true; } */

        /*         draw_fillcircle( */
        /*             &ed->map.drawsurf, */
        /*             editor_map_transform(ed, side_normal_point(it.el)), */
        /*             ed->map.wholescale + 2, */
        /*             hit ? 0xFFFF0000 : 0xFF0000FF); */
        /*     } */
        /* } */
    }

    // show each sector in a different highlight color
    if ((ed->visopt & VISOPT_SHOWSECT) || (ed->visopt & VISOPT_FLOORSECT)) {
        /* level_dynlist_each(ed->level->sectors, it) { */
        /*     sector_t *sect = *it.el; */

        /*     const u32 color = */
        /*         (ed->visopt & VISOPT_SHOWSECT) ? */
        /*             0x50000000 */
        /*             | (state->pal.colors[lptr_rand_palette(LPTR_FROM(sect))] */
        /*                 & 0xFFFFFF) */
        /*             : 0; */

        /*     sector_draw_filled( */
        /*         ed, */
        /*         sect, */
        /*         get_color_for_ptr( */
        /*             ed, */
        /*             LPTR_FROM(sect), */
        /*             color, */
        /*             MAP_SECTOR_HIGHLIGHT, */
        /*             MAP_SECTOR_HIGHLIGHT), */
        /*         ed->visopt & VISOPT_FLOORSECT); */
        /* } */
    }

    // show blocks
    if (ed->visopt & VISOPT_BLOCKS) {
        const vec2s
            pmin = editor_screen_to_map(ed, VEC2(0)),
            pmax = editor_screen_to_map(ed, IVEC_TO_V(ed->size));

        const ivec2s
            boffset = ed->level->blocks.offset,
            bsize = ed->level->blocks.size,
            bmin = IVEC2(pmin.x / BLOCK_SIZE, pmin.y / BLOCK_SIZE),
            bmax = IVEC2(pmax.x / BLOCK_SIZE, pmax.y / BLOCK_SIZE),
            bmcursor = IVEC2(
                ed->cur.pos.map.x / BLOCK_SIZE,
                ed->cur.pos.map.y / BLOCK_SIZE
                );

        for (int by = bmin.y; by <= bmax.y; by++) {
            for (int bx = bmin.x; bx <= bmax.x; bx++) {
                // only draw block if it exists and contains some walls
                if (bx < boffset.x
                    || by < boffset.y
                    || bx >= boffset.x + bsize.x
                    || by >= boffset.y + bsize.y) {
                    continue;
                }

                block_t *block =
                    &ed->level->blocks.arr[
                        (by - boffset.y) * bsize.x + (bx - boffset.x)];

                const bool curbox = bx == bmcursor.x && by == bmcursor.y;

                sgp_set_color(SVEC4(ABGR_TO_VEC4(curbox ? 0x80A0FFFF : 0x40A0A0A0)));
                sgp_draw_aabbf(
                    AABBF_MM(
                        VEC2((bx + 0) * BLOCK_SIZE, (by + 0) * BLOCK_SIZE),
                        VEC2((bx + 1) * BLOCK_SIZE, (by + 1) * BLOCK_SIZE)),
                    MAP_LINE_THICKNESS * 0.5f);

                if (!curbox) {
                    continue;
                }

                // show which walls are a part of block
                dynlist_each(block->walls, it) {
                    wall_t *wall = *it.el;

                    sgp_set_color(SVEC4(ABGR_TO_VEC4(0xFF80FF80)));
                    sgp_draw_thick_line(
                        SVEC2(wall->v0->pos),
                        SVEC2(wall->v1->pos),
                        MAP_LINE_THICKNESS);
                }

                // show subsectors
                dynlist_each(block->subsectors, it) {
                    sgp_set_color(0.2f, 0.2f, 1.0f, 0.4f);
                    dynlist_each(ed->level->subsectors[*it.el]->lines, it_l) {
                        sgp_draw_thick_line(
                            SVEC2(it_l.el->a->pos),
                            SVEC2(it_l.el->b->pos),
                            MAP_LINE_THICKNESS);
                    }
                }

                // show objects which are part of block
                dlist_each(block_list, &block->objects, it) {
                    sgp_set_color(SVEC4(ABGR_TO_VEC4(0xFF80FF80)));
                    sgp_draw_filled_rect(
                        SVEC2(glms_vec2_sub(it.el->pos, VEC2(MAP_OBJECT_SIZE / 2))),
                        SVEC2(VEC2(MAP_OBJECT_SIZE)));
                }
            }
        }
    }

    if (!ed->ui_has_mouse && !ed->ui_has_keyboard) {
        const u8
            bs_select = ed->buttons[BUTTON_SELECT],
            bs_deselect = ed->buttons[BUTTON_DESELECT],
            bs_multi = ed->buttons[BUTTON_MULTI_SELECT],
            bs_edit = ed->buttons[BUTTON_EDIT],
            bs_cancel = ed->buttons[BUTTON_CANCEL];

        if (bs_select & INPUT_PRESS) {
            editor_map_select(ed, ed->cur.hover);
        }

        // click to edit
        // open on RELEASE so as not to conflict with MOVE_DRAG
        if ((ed->cur.mode == CM_DEFAULT
             || (ed->cur.mode == CM_MOVE_DRAG
                 && !ed->cur.mode_move_drag.moved))
            && !LPTR_IS_NULL(ed->cur.hover)
            && (bs_edit & INPUT_RELEASE)) {
            editor_open_for_ptr(ed, ed->cur.hover);
        }

        // multi-select for walls, vertices
        if (ed->cur.mode == CM_DEFAULT
            && LPTR_IS(ed->cur.hover, T_WALL | T_VERTEX)
            && (bs_multi & INPUT_DOWN)) {

            if (bs_select & INPUT_PRESS) {
                editor_map_select(ed, ed->cur.hover);
            }

            if (bs_deselect & INPUT_PRESS) {
                editor_map_deselect(ed, ed->cur.hover);
            }
        }

        // cancel operation with CANCEL
        if (bs_cancel & INPUT_PRESS) {
            editor_map_clear_select(ed);
            ed->cur.mode = CM_DEFAULT;
        }

        // if selected without hover AND click to cancel, cancel
        if ((CURSOR_MODES[ed->cur.mode].flags & CMF_CLICK_CANCEL)
            && LPTR_IS_NULL(ed->cur.hover)
            && (bs_select & INPUT_PRESS)) {
            editor_map_clear_select(ed);
            ed->cur.mode = CM_DEFAULT;
        }
    }

    // draw cursor mode
    const cursor_mode_t *cmd = &CURSOR_MODES[ed->cur.mode];
    if (cmd->render) { cmd->render(ed, &ed->cur); }

    sgp_pop_transform();
}

vec2s editor_map_snapped_cursor_pos(
    editor_t *ed,
    cursor_t *c,
    lptr_t *hover_out,
    sector_t **sector_out) {
    wall_t *snapwall = NULL;

    vec2s pos = c->pos.map;

    if (LPTR_IS(c->hover, T_WALL)) {
        snapwall = LPTR_WALL(ed->level, c->hover);
    } else if (ed->buttons[BUTTON_SNAP_WALL] & INPUT_DOWN) {

        f32 dist = 1e30;

        // look for nearby walls
        level_dynlist_each(ed->level->sectors, it_sect) {
            sector_t *sect = *it_sect.el;

            // TODO: filter sectors ?
            /* if (!point_in_box( */
            /*         pos, it_sect.el->min, it_sect.el->max)) { */
            /*     continue; */
            /* } */

            llist_each(sector_sides, &sect->sides, it_side) {
                side_t *s = it_side.el;

                wall_t *w = s->wall;
                const vec2s proj =
                    point_project_segment(
                        pos,
                        w->v0->pos,
                        w->v1->pos);
                const f32 wdist = glms_vec2_norm(glms_vec2_sub(proj, pos));
                if (wdist < dist && wdist < 64) {
                    dist = wdist;
                    snapwall = w;
                }
            }
        }

        if (snapwall) {
            pos =
                point_project_segment(
                    pos,
                    snapwall->v0->pos,
                    snapwall->v1->pos);
        }
    } else if (ed->buttons[BUTTON_SNAP] & INPUT_DOWN) {
        snapwall = NULL;
        pos = editor_map_snap_to_grid(ed, pos);
    }

    lptr_t ptr = editor_map_locate(ed, pos, sector_out);
    if (hover_out) { *hover_out = ptr; }

    return pos;
}

void editor_map_to_edcam(editor_t *ed) {
    if (!ed->camobj) { return; }
    editor_map_center_on(ed, ed->camobj->pos);
}

void editor_edcam_to_map(editor_t *ed) {
    if (!ed->camobj) { return; }
    ed->camobj->pos =
        editor_map_to_screen(
            ed, VEC2(
                ed->size.x / 2,  // NOLINT
                ed->size.y / 2 // NOLINT
            ));
}
