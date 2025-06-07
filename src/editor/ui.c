#include "editor/ui.h"
#include "editor/ui_misc.h"
#include "editor/editor.h"
#include "editor/cursor.h"
#include "editor/map.h"
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
#include "util/file.h"
#include "util/input.h"
#include "state.h"

// edit flags, pass to "flags" of "edit_*"
enum {
    EF_NONE                 = 0,
    EF_NEW                  = 1 << 0,
    EF_SELECTED             = 1 << 1,
    EF_VERTEX_LIST_WALLS    = 1 << 2
};

static bool edit_vertex(
    editor_t *ed,
    vertex_t *v,
    int flags) {
    bool change = false;
    igPushID_Ptr(v);
    igPushItemWidth(INPUT_WIDTH_INT);
    {
        change |= igInputFloat("##x", &v->pos.x, 0.25f, 1.0f, "%.3f", 0);
        sameline();
        change |= igInputFloat("##y", &v->pos.y, 0.25f, 1.0f, "%.3f", 0);
    }
    igPopItemWidth();
    igPopID();

    if (flags & EF_VERTEX_LIST_WALLS) {
        if (igTreeNode_Str("WALLS")) {
            dynlist_each(v->walls, it) {
                wall_t *wall = *it.el;

                igText("%d", wall->index);

                sameline();
                if (igButton("EDIT", (ImVec2) { 0, 0 })) {
                    editor_open_for_ptr(ed, LPTR_FROM(wall));
                }

                sameline();
                if (igButton("SELECT", (ImVec2) { 0, 0 })) {
                    editor_try_select_ptr(ed, LPTR_FROM(wall));
                }
            }

            igTreePop();
        }
    }

    if (change) {
        vertex_recalculate(ed->level, v);
    }

    return change;
}

static bool edit_side_tex_info(
    editor_t *ed,
    side_texinfo_t *info,
    int flags) {
    igPushID_Ptr(info);
    bool change = false;
    igPushItemWidth(INPUT_WIDTH_INT);
    {
        igText("SPLITS ");
        sameline();
        change |=
            igInputFloat("##sb", &info->split_bottom, 0.1f, 0.25f, "%.3f", 0);
        sameline();
        change |=
            igInputFloat("##st", &info->split_top, 0.1f, 0.25f, "%.3f", 0);

        igText("OFFSETS");
        sameline();
        change |=
            igInputInt("##ox", &info->offsets.x, 1, 32, 0);
        sameline();
        change |=
            igInputInt("##oy", &info->offsets.y, 1, 32, 0);
    }
    igPopItemWidth();
    igPushItemWidth(80);
    {
        change |=
            input_u8_flags(
                &info->flags,
                (STF_UNPEG << 1) - 1,
                // must correspond with definitions in shared_defs.h
                ((const char *[]) {
                    "BOT ABS",
                    "TOP ABS",
                    "Y ABS  ",
                    "EZPORT",
                    "UNPEG",
                }),
                ((const int []) { 2, 3 }), 2);
    }
    igPopItemWidth();
    igPopID();
    return change;
}

static bool edit_sidemat(
    editor_t *ed,
    sidemat_t *mat,
    int flags) {
    igPushID_Ptr(mat);
    igBeginDisabled(mat->index == SIDEMAT_NOMAT);
    igBeginGroup();
    bool change = false;

    igSetNextItemWidth(ITEM_WIDTH_NAME);

    change |= input_resource_name(ed, "##name", &mat->name);

    // copy from another side or sidemat
    sameline();
    lptr_t copy = button_select_tag(ed, "CPY", T_SIDE | T_SIDEMAT, BST_NONE);
    if (!LPTR_IS_NULL(copy)) {
        sidemat_copy_props(
            mat,
            LPTR_IS(copy, T_SIDE) ?
                LPTR_SIDE(ed->level, copy)->mat
                : LPTR_SIDEMAT(ed->level, copy));
        change = true;
    }

    // apply to a side
    sameline();
    lptr_t apply = button_select_tag(ed, "APL", T_SIDE, BST_ALLOW_MULTI);
    if (!LPTR_IS_NULL(apply)) {
        side_t *side = LPTR_SIDE(ed->level, apply);
        side->mat = mat;
        side_recalculate(ed->level, side);
    }

    for (int i = 0; i < (int) ARRLEN(mat->texs); i++) {
        const char *label = ((const char*[3]){ "slow", "smid", "shigh" })[i];
        if (i != 0) { sameline(); }
        change |= texture_select(ed, label, &mat->texs[i]);
    }

    igSpacing();

    change |= edit_side_tex_info(ed, &mat->tex_info, EF_NONE);

    igEndGroup();
    igEndDisabled();
    igPopID();

    if (change) {
        sidemat_recalculate(ed->level, mat);
    }

    return change;
}

static bool edit_side(
    editor_t *ed,
    side_t *side,
    wall_t *wall,
    int flags) {
    igPushID_Ptr(side);
    bool change = false;

    const ImVec4 color =
        (flags & EF_SELECTED) ?
            (ImVec4) { 1, 1, 0.2, 1 }
            : (ImVec4) { 1, 1, 1, 1 };

    if (!side) {
        igAlignTextToFramePadding();
        igTextColored(color, "%s", "SIDE:");

        sameline();
        if (igButton("CREATE##side", (ImVec2) { 0, 0 })) {
            ASSERT(!wall->side0 || !wall->side1);

            side =
                side_new(
                    ed->level,
                    wall->side0 ? wall->side1 : wall->side0);

            wall_set_side(ed->level, wall, wall->side0 ? 1 : 0, side);
            change = true;
        }

        goto done;
    }

    igAlignTextToFramePadding();
    igTextColored(
        color,
        "SIDE (%d)%s",
        side->index,
        (flags & EF_SELECTED) ? " (SELECTED)" : "");

    sameline();
    if (igButton("SELECT", (ImVec2) { 0, 0 })) {
        editor_try_select_ptr(ed, LPTR_FROM(side));
    }

    if ((flags & EF_NEW) && (flags & EF_SELECTED)) {
        igTreeNodeSetOpen(igGetID_Str("PROPS"), true);
    }

    if (igTreeNode_Str("PROPS")) {
        igPushID_Str("sector");
        igBeginGroup();
        {
            sector_t *sect = side->sector;

            char sectname[64];
            lptr_to_str(sectname, sizeof(sectname), ed->level, LPTR_FROM(sect));
            igAlignTextToFramePadding();
            igText(sectname);

            sameline();
            if (igButton("EDIT", (ImVec2) { 0, 0 })) {
                editor_open_for_ptr(ed, LPTR_FROM(sect));
            }

            if (sect != side->sector) {
                if (side->sector) {
                    sector_remove_side(ed->level, side->sector, side);
                }

                if (sect) {
                    sector_add_side(ed->level, sect, side);
                }

                change = true;
            }
        }
        igEndGroup();
        igPopID();

        // portal
        igPushID_Str("portal");
        igBeginGroup();
        {
            side_t *portal = side->portal;

            const lptr_t newptr =
                input_lptr(ed, "PORTAL", T_SIDE, LPTR_FROM(side->portal));
            if (!LPTR_IS_NULL(newptr)) {
                portal = LPTR_SIDE(ed->level, newptr);
            }

            sameline();
            if (igButton("MAKE", (ImVec2) { 0, 0 })) {
                side_t *other = side_other(side);

                // if no other side, make it
                if (!other) {
                    other = side_new(ed->level, side);
                    wall_set_side(
                        ed->level, wall, side == wall->side0 ? 1 : 0, other);
                    other->portal = side;
                    level_update_side_sector(ed->level, other);
                }

                portal = other;
            }

            sameline();
            if (igButton("REMOVE", (ImVec2) { 0, 0 })) {
                portal = NULL;
            }

            if (portal != side->portal) {
                side_t *old_portal = side->portal;
                side->portal = portal;
                if (old_portal) {
                    side_recalculate(ed->level, old_portal);
                }
                change = true;
            }
        }
        igEndGroup();
        igPopID();

        change |=
            input_enum(
                "COSTYPE",
                &side->cos_type,
                sizeof(side_cos_type_index),
                SDCT_DISTINCT,
                side_cos_type_index_names(),
                side_cos_type_index_nth_value);

        change |=
            input_enum(
                "FUNCTYPE",
                &side->func_type,
                sizeof(side_func_type_index),
                SDFT_DISTINCT,
                side_func_type_index_names(),
                side_func_type_index_nth_value);

        change |= input_tag(ed, LPTR_FROM(side), &side->tag);

        igTreePop();
    }

    if ((flags & EF_NEW) && (flags & EF_SELECTED)) {
        igTreeNodeSetOpen(igGetID_Str("MAT"), true);
    }

    if (igTreeNode_Str("MAT")) {
        char matname[64];
        lptr_to_str(
            matname, sizeof(matname), ed->level, LPTR_FROM(side->mat));
        igText("%s", matname);

        igAlignTextToFramePadding();
        igText("%s", side->mat->name.name);
        igSameLine(ITEM_WIDTH_NAME, -1);

        if (igButton("...", (ImVec2) { 0, 0 })) {
            igOpenPopup_Str("...", ImGuiPopupFlags_None);
        }

        sidemat_t *mat = sidemat_select_popup(ed, "...");
        if (mat) {
            side->mat = mat;
            change = true;
        }

        const side_t *other = side_other(side);
        sameline();
        igBeginDisabled(!other);
        if (igButton("SAME", (ImVec2) { 0, 0 })) {
            side->mat = other->mat;
        }
        igEndDisabled();

        // copy function
        sameline();
        lptr_t copy = button_select_tag(ed, "CPY", T_SIDE, BST_NONE);
        if (!LPTR_IS_NULL(copy)) {
            side_t *copyside = LPTR_SIDE(ed->level, copy);
            side->mat = copyside->mat;
            side->flags = copyside->flags;
            side->tex_info = copyside->tex_info;
            change = true;
        }

        // apply function
        sameline();
        lptr_t apply = button_select_tag(ed, "APL", T_SIDE, BST_ALLOW_MULTI);
        if (!LPTR_IS_NULL(apply)) {
            side_t *apply_side = LPTR_SIDE(ed->level, apply);
            apply_side->mat = side->mat;
            apply_side->flags = side->flags;
            apply_side->tex_info = side->tex_info;
            side_recalculate(ed->level, apply_side);
            change = true;
        }

        sameline();
        if (igButton("NEW", (ImVec2) { 0, 0 })) {
            sidemat_t *oldmat = side->mat;
            side->mat = sidemat_new(ed->level);
            sidemat_copy_props(side->mat, oldmat);
        }

        sameline();
        change |=
            input_flags(
                &side->flags,
                SIDE_FLAG_MATOVERRIDE,
                ((const char*[]) { "OVR?" }),
                NULL, 0);

        igSpacing();

        if (igTreeNode_Str("OVERRIDES")) {
            igBeginDisabled(!(side->flags & SIDE_FLAG_MATOVERRIDE));
            change |= edit_side_tex_info(ed, &side->tex_info, EF_NONE);
            igEndDisabled();
            igTreePop();
        }

        if (igTreeNode_Str("EDIT")) {
            change |= edit_sidemat(ed, side->mat, EF_NONE);
            igTreePop();
        }

        igTreePop();
    }
done:
    igPopID();

    if (side && change) {
        side_recalculate(ed->level, side);
    }

    return change;
}

static bool edit_wall(
    editor_t *ed,
    wall_t *wall,
    side_t *side,
    int flags) {
    bool change = false;
    char namebuf[64];

    igPushID_Ptr(wall);
    igBeginGroup();

    igAlignTextToFramePadding();
    lptr_to_str(namebuf, sizeof(namebuf), ed->level, LPTR_FROM(wall->v0));
    igText("%s: A", namebuf);
    sameline();
    change |= edit_vertex(ed, wall->v0, flags);

    igAlignTextToFramePadding();
    lptr_to_str(namebuf, sizeof(namebuf), ed->level, LPTR_FROM(wall->v1));
    igText("%s: B", namebuf);
    sameline();
    change |= edit_vertex(ed, wall->v1, flags);

    igSeparator();
    change |=
        edit_side(
            ed,
            wall->side0,
            wall,
            flags | ((side && side == wall->side0) ? EF_SELECTED : EF_NONE));
    igSeparator();
    change |=
        edit_side(
            ed,
            wall->side1,
            wall,
            flags | ((side && side == wall->side1) ? EF_SELECTED : EF_NONE));
    igEndGroup();
    igPopID();

    if (change) {
        wall_recalculate(ed->level, wall);
    }

    return change;
}

static bool edit_sectmat(
    editor_t *ed,
    sectmat_t *mat,
    int flags) {
    igPushID_Ptr(mat);
    igBeginDisabled(mat->index == SECTMAT_NOMAT);
    igBeginGroup();

    bool change = false;

    igSetNextItemWidth(ITEM_WIDTH_NAME);

    change |= input_resource_name(ed, "##name", &mat->name);

    // copy from another sect or sectmat
    sameline();
    lptr_t copy = button_select_tag(ed, "CPY", T_SECTOR | T_SECTMAT, BST_NONE);
    if (!LPTR_IS_NULL(copy)) {
        sectmat_copy_props(
            mat,
            LPTR_IS(copy, T_SECTOR) ?
                LPTR_SECTOR(ed->level, copy)->mat
                : LPTR_SECTMAT(ed->level, copy));
        change = true;
    }

    // apply to a sect
    sameline();
    lptr_t apply = button_select_tag(ed, "APL", T_SECTOR, BST_ALLOW_MULTI);
    if (!LPTR_IS_NULL(apply)) {
        sector_t *apply_sect = LPTR_SECTOR(ed->level, apply);
        apply_sect->mat = mat;
        sector_recalculate(ed->level, apply_sect);
    }

    for (int i = 0; i < 2; i++) {
        const char *label =
            ((const char*[2]){ "floor", "ceil" })[i];

        planemat_t *pmat = &mat->mats[i];
        change |=
            texture_select_offset(
                ed,
                label,
                &pmat->tex,
                &pmat->offset);
    }

    igEndGroup();
    igEndDisabled();
    igPopID();

    if (change) {
        sectmat_recalculate(ed->level, mat);
    }

    return change;
}

static bool edit_plane(
    editor_t *ed,
    plane_t *plane,
    plane_type type,
    sector_t *sector,
    int flags) {
    igPushID_Ptr(plane);
    igBeginGroup();
    bool change = false;

    char label[64];
    snprintf(
        label, sizeof(label),
        "%s",
        type == PLANE_TYPE_CEIL ? "CEIL" : "FLOOR");

    igSetNextItemWidth(INPUT_WIDTH_INT);
    change |= igInputFloat("##z", &plane->z, 0.25f, 1.0f, "%.3f", 0);
    sameline();
    change |=
        input_copy_apply(
            ed, label,
            LPTR_FROM(sector),
            &plane->z,
            sizeof(plane->z),
            (int[]) { T_SECTOR },
            (int[]) { ((u8*) &plane->z) - ((u8*) sector) },
            1,
            NULL, NULL);

    // add slope controls on the next line
    /* igSpacing();

    // create a label for the slope
    char slope_label[64];
    snprintf(
        slope_label, sizeof(slope_label),
        "%s SLOPE",
        type == PLANE_TYPE_CEIL ? "CEIL" : "FLOOR");

    // store old values to detect changes
    float old_angle = plane->slope_angle_deg;
    float old_direction = plane->slope_direction_deg;
    bool old_enabled = plane->slope_enabled;

    // slope height input
    igSetNextItemWidth(INPUT_WIDTH_INT);
    change |= igInputFloat("##slope", &plane->slope_angle_deg, 0.25f, 1.0f, "%.3f", 0);

    // If angle changed, update slope_enabled
    if (old_angle != plane->slope_angle_deg) {
        plane->slope_enabled = (plane->slope_angle_deg > 0.001f);
        change = true;
    }

    sameline();
    change |=
        input_copy_apply(
            ed, slope_label,
            LPTR_FROM(sector),
            &plane->slope_angle_deg,
            sizeof(plane->slope_angle_deg),
            (int[]) { T_SECTOR },
            (int[]) { ((u8*) &plane->slope_angle_deg) - ((u8*) sector) },
            1,
            NULL, NULL);

    // SIDE
    sameline();
    igSetNextItemWidth(80);
    const char* sides[] = { "SIDE 1", "SIDE 2", "SIDE 3", "SIDE 4" };
    int current_side = (int)(plane->slope_direction_deg / 90.0f) % 4;

    if (igBeginCombo("##side", sides[current_side], 0)) {
        for (int i = 0; i < 4; i++) {
            bool is_selected = (current_side == i);
            if (igSelectable_Bool(sides[i], is_selected, 0, (ImVec2){0,0})) {
                plane->slope_direction_deg = i * 90.0f;

                // If we have a non-zero angle, make sure slope is enabled
                if (plane->slope_angle_deg > 0.001f) {
                    plane->slope_enabled = true;
                }

                change = true;
            }
            if (is_selected) {
                igSetItemDefaultFocus();
            }
        }
        igEndCombo();
    }

    // always ensure slope_enabled is consistent with angle
    if (plane->slope_angle_deg > 0.001f) {
        if (!plane->slope_enabled) {
            plane->slope_enabled = true;
            change = true;
        }
    } else {
        if (plane->slope_enabled) {
            plane->slope_enabled = false;
            change = true;
        }
    }

    // debug output if values changed
    if (old_angle != plane->slope_angle_deg ||
        old_direction != plane->slope_direction_deg ||
        old_enabled != plane->slope_enabled) {
        printf("Slope updated: enabled=%d, angle=%.2f, direction=%.2f\n",
               plane->slope_enabled, plane->slope_angle_deg, plane->slope_direction_deg);
    } */

    igEndGroup();
    igPopID();
    return change;
}

static bool edit_sector_funcvals(
    editor_t *ed,
    sector_t *sector,
    int flags) {
    bool change = false;
    switch (sector->func_type) {
    case SCFT_NONE: return false;
    case SCFT_DIFF: {
        // TODO
        /*TYPEOF(sector->funcdata.diff) *fd = &sector->funcdata.diff; 
        igSetNextItemWidth(INPUT_WIDTH_INT); 
        change |= input_i16("DIFF", &fd->diff, 1, 16, 0); 
        igSetNextItemWidth(INPUT_WIDTH_INT); 
        change |= input_i16("TICKS", &fd->ticks, 1, 16, 0); 
        change |= 
             input_enum( 
                 "PLANE", 
                 &fd->plane, 
                 sizeof(plane_type), 
                 2, 
                 (const char *[]) { "FLOOR", "CEIL" }, 
                 NULL);  */
    } break;
    case SCFT_DOOR: {
        // tODO
        /* TYPEOF(sector->funcdata.door) *fd = &sector->funcdata.door; */

        /* const lptr_t */
        /*     ptr = lptr_from_typed_index(ed->level, fd->hinge), */
        /*     newptr = input_lptr(ed, "HINGE", T_VERTEX, ptr); */

        /* if (!LPTR_IS_NULL(newptr)) { */
        /*     fd->hinge = lptro_typed_index(newptr); */
        /*     change = true; */
        /* } */

        /* sameline(); */
        /* if (igButton("CLEAR", (ImVec2) { 0, 0 })) { */
        /*     fd->hinge = LEVEL_TYPED_INDEX_NULL; */
        /*     change = true; */
        /* } */

        /* change |= igCheckbox("CCW?", &fd->ccw); */
    } break;
    default:
        WARN("invalid sector func_type %d", (int) sector->func_type);
        return false;
    }

    change |=
        input_copy_apply(
            ed, "",
            LPTR_FROM(sector),
            &sector->funcdata,
            sizeof(sector->funcdata),
            (int[]) { T_SECTOR },
            (int[]) { offsetof(sector_t, funcdata) },
            1,
            NULL,
            NULL);

    return change;
}

static bool edit_sector(
    editor_t *ed,
    sector_t *sector,
    int flags) {
    bool change = false;

    igPushID_Ptr(sector);
    igBeginGroup();

    if (igButton("SELECT", (ImVec2) { 0, 0 })) {
        editor_try_select_ptr(ed, LPTR_FROM(sector));
    }

    sameline();
    if (igButton("DELETE", (ImVec2) { 0, 0 })) {
        sector_delete(ed->level, sector);
        goto done;
    }

    igBeginGroup();
    {
        change |=
            input_flags(
                &sector->flags,
                SECTOR_FLAG_MASK,
                &sector_flags_names()[1],
                ((const int[]) { 2, 4 }),
                2);

        change |=
            input_enum(
                "COSTYPE",
                &sector->cos_type,
                sizeof(sector_cos_type_index),
                SCCT_DISTINCT,
                sector_cos_type_index_names(),
                sector_cos_type_index_nth_value);

        change |=
            input_enum(
                "FUNCTYPE",
                &sector->func_type,
                sizeof(sector_func_type_index),
                SCFT_DISTINCT,
                sector_func_type_index_names(),
                sector_func_type_index_nth_value);

        change |= input_tag(ed, LPTR_FROM(sector), &sector->tag);

        if (igTreeNode_Str("FUNCVALS")) {
            change |= edit_sector_funcvals(ed, sector, flags);
            igTreePop();
        }

        igSetNextItemWidth(INPUT_WIDTH_INT);
        change |=
            input_u8_clamped(
                "##base_light",
                &sector->base_light,
                1, 4,
                ImGuiInputTextFlags_None,
                0, LIGHT_MAX);
        sameline();
        change |=
            input_copy_apply(
                ed, "BASE LIGHT",
                LPTR_FROM(sector),
                &sector->base_light,
                sizeof(sector->base_light),
                (int[]) { T_SECTOR },
                (int[]) { offsetof(sector_t, base_light) },
                1,
                NULL, NULL);
    }
    igEndGroup();

    igSeparator();

    igText("PLANES");
    igSpacing();
    change |= edit_plane(ed, &sector->ceil, PLANE_TYPE_CEIL, sector, flags);
    change |= edit_plane(ed, &sector->floor, PLANE_TYPE_FLOOR, sector, flags);
    igSeparator();

    igText("MAT");
    igSpacing();
    igSameLine(INDENT_WIDTH_L0, -1);

    igBeginGroup();
    {
        igAlignTextToFramePadding();
        igText("%s", sector->mat->name.name);
        igSameLine(ITEM_WIDTH_NAME, -1);

        // select material
        if (igButton("...", (ImVec2) { 0, 0 })) {
            igOpenPopup_Str("...", ImGuiPopupFlags_None);
        }

        sectmat_t *mat = sectmat_select_popup(ed, "...");
        if (mat) {
            sector->mat = mat;
            change = true;
        }

        sameline();
        input_copy_apply(
            ed, "",
            LPTR_FROM(sector),
            &sector->mat,
            sizeof(sector->mat), // NOLINT
            (int[]) { T_SECTOR },
            (int[]) { offsetof(sector_t, mat) },
            1,
            NULL, NULL);

        sameline();
        if (igButton("NEW", (ImVec2) { 0, 0 })) {
            sectmat_t *oldmat = sector->mat;
            sector->mat = sectmat_new(ed->level);
            sectmat_copy_props(sector->mat, oldmat);
        }

        if (igTreeNode_Str("EDIT")) {
            change |= edit_sectmat(ed, sector->mat, flags);
            igTreePop();
        }
    }
    igEndGroup();

    igSeparator();

    if (igTreeNode_Str("SIDES")) {
        ImVec2 avail;
        igGetContentRegionAvail(&avail);
        igBeginChild_Str(
            "SIDES##child",
            (ImVec2){ 256, 128 },
            false,
            ImGuiWindowFlags_None);

        llist_each(sector_sides, &sector->sides, it) {
            side_t
                *side = it.el,
                *sides[2] = { side, side_other(side) };

            igBeginGroup();
            igPushID_Ptr(side);
            igAlignTextToFramePadding();
            igText("%d", side->index);
            igSameLine(60, -1);
            for (int i = 0; i < 2; i++) {
                if (i != 0) { sameline(); }
                igBeginGroup();
                igBeginDisabled(!sides[i]);
                sameline();
                if (igButton(
                        i == 0 ? "EDT" : "EDT (O)", (ImVec2) { 0, 0 })) {
                    editor_open_for_ptr(ed, LPTR_FROM(sides[i]));
                }

                sameline();
                if (igButton(
                        i == 0 ? "SEL" : "SEL (O)", (ImVec2) { 0, 0 })) {
                    editor_try_select_ptr(ed, LPTR_FROM(sides[i]));
                }
                igEndDisabled();
                igEndGroup();

                if (sides[i] && igIsItemHovered(ImGuiHoveredFlags_None)) {
                    // TODO
                    /* igBeginTooltip(); */
                    /* { */
                    /*     igAlignTextToFramePadding(); */
                    /*     igText( */
                    /*         "%d (%4d,%4d) -> (%4d,%4d):", */
                    /*         sides[i]->index, */
                    /*         sides[i]->wall->v0->x, sides[i]->wall->v0->y, */
                    /*         sides[i]->wall->v1->x, sides[i]->wall->v1->y); */
                    /* } */
                    /* igEndTooltip(); */

                    editor_highlight_ptr(ed, LPTR_FROM(sides[i]));
                }
            }
            igEndGroup();
            igPopID();
        }

        igEndChild();
        igTreePop();
    }

    if (change) {
        sector_recalculate(ed->level, sector);
    }

done:
    igEndGroup();
    igPopID();

    return change;
}

static bool edit_decal_funcvals(
    editor_t *ed,
    decal_t *decal,
    int flags) {
    bool change = false;
    switch (decal->type) {
    case DT_PLACEHOLDER: break;
    case DT_SWITCH: break;
    case DT_BUTTON: break;
    case DT_RIFT: { 
    } break;
    default: WARN("inavlid decal type %d", decal->type); break;
    }
    return change;
}

static bool edit_decal(editor_t *ed, decal_t *decal, int flags) {
    igPushID_Ptr(decal);
    igBeginGroup();
    bool change = false;

    if (igButton("SELECT", (ImVec2) { 0, 0 })) {
        editor_try_select_ptr(ed, LPTR_FROM(decal));
    }

    sameline();
    if (igButton("DELETE", (ImVec2) { 0, 0 })) {
        decal->level_flags |= LF_DELETE;
        change = true;
    }

    change |= texture_select(ed, "TEXTURE", &decal->tex_base);

    sameline();
    change |=
        input_enum(
            "TYPE",
            &decal->type,
            sizeof(decal_type_index),
            DT_DISTINCT,
            decal_type_index_names(),
            decal_type_index_nth_value);

    change |= input_tag(ed, LPTR_FROM(decal), &decal->tag);

    igSpacing();
    edit_decal_funcvals(ed, decal, flags);

    igPushItemWidth(INPUT_WIDTH_INT);
    {
        igText("TEX OFFSETS");
        sameline();
        change |= igInputInt("##tx", &decal->tex_offsets.x, 1, 16, 0);
        sameline();
        change |= igInputInt("##ty", &decal->tex_offsets.y, 1, 16, 0);

        igSpacing();

        const char *pos_title;
        vec2s *pos;
        if (decal->is_on_side) {
            pos_title = "POS OFFSETS";
            pos = &decal->side.offsets;
        } else {
            pos_title = "WORLD POS  ";
            pos = &decal->sector.pos;
        }

        igText(pos_title);
        sameline();
        change |= igInputFloat("##ox", &pos->x, 0.25f, 1.0f, "%.3f", 0);
        sameline();
        change |= igInputFloat("##oy", &pos->y, 0.25f, 1.0f, "%.3f", 0);
    }
    igPopItemWidth();

    igBeginDisabled(decal->is_on_side);
    {
        igText("PLANE      ");
        sameline();

        plane_type p = decal->sector.plane;
        change |=
            input_enum(
                "", &p, sizeof(plane_type),
                2,
                (const char *[]) { "FLOOR", "CEIL" },
                NULL);
        if (!decal->is_on_side) { decal->sector.plane = p; }
    }
    igEndDisabled();

    const lptr_t newptr =
        input_lptr(
            ed,
            "PARENT",
            T_SIDE | T_SECTOR,
            decal->is_on_side ?
                LPTR_FROM(decal->side.ptr)
                : LPTR_FROM(decal->sector.ptr));

    if (!LPTR_IS_NULL(newptr)) {
        if (LPTR_TYPE(newptr) == T_SIDE) {
            decal_set_side(ed->level, decal, LPTR_SIDE(ed->level, newptr));
        } else {
            decal_set_sector(
                ed->level,
                decal,
                LPTR_SECTOR(ed->level, newptr),
                PLANE_TYPE_FLOOR);
        }

        change = true;
    }

    // TODO: edit plane for sector decals

    if (change) {
        decal_recalculate(ed->level, decal);
    }

    igEndGroup();
    igPopID();

    return change;
}

static bool edit_object(editor_t *ed, object_t *object, int flags) {
    igPushID_Ptr(object);
    igBeginGroup();
    bool change = false;

    if (igButton("SELECT", (ImVec2) { 0, 0 })) {
        editor_try_select_ptr(ed, LPTR_FROM(object));
    }

    sameline();
    if (igButton("DELETE", (ImVec2) { 0, 0 })) {
        object->level_flags |= LF_DELETE;
        change = true;
    }

    char sectname[64];
    lptr_to_str(sectname, sizeof(sectname), ed->level, LPTR_FROM(object->sector));
    igText("%s", sectname);

    texture_image(object->type->sprite);
    sameline();

    object_type_index itype = object->type_index;
    if (input_enum(
            "TYPE",
            &itype, sizeof(object_type_index),
            OT_DISTINCT,
            object_type_index_names(),
            object_type_index_nth_value)) {
        object_set_type(ed->level, object, itype);
        change = true;
    }

    switch (object->type_index) {
    case OT_PICKUP: { 
    } break;
    default: break;
    }

    igPushItemWidth(INPUT_WIDTH_INT);
    {
        bool poschange = false;
        vec2s pos = object->pos;
        struct { const char *label; f32 *v; } values[2] =
            {{ "##tx", &pos.x }, { "##ty", &pos.y }};

        for (int i = 0; i < 2; i++) {
            if (i != 0) { sameline(); }
            poschange |=
                input_f32_clamped(
                    values[i].label,
                    values[i].v,
                    1, 32,
                    "%.1f",
                    ImGuiInputTextFlags_None,
                    0, U16_MAX);
        }
        sameline();
        igText("%s", "POS");

        if (poschange) {
            object_move(ed->level, object, pos);
            change = true;
        }
    }
    igPopItemWidth();

    igEndGroup();
    igPopID(); 

    return change;
}

// igText, but flashes yellow briefly when contents are changed
// "current" is pointer to buffer, "last" is a backup of buffer
// saves change tick in *changetime
static void change_highlight_text(
    editor_t *ed,
    const char *label,
    const char *current,
    char *last,
    usize n,
    u64 *changetime,
    bool colortext) {

    if (strcmp(current, last)) {
        *changetime = state->time.now;
    }

    u32
        col_bg = IM_COL32(64, 64, 64, 64),
        col_text =
            colortext ?
                IM_COL32(255, 255, 32, 255) : IM_COL32(255, 255, 255, 255);

    // flash on change
    if (*changetime != 0) {
        if ((state->time.now - *changetime) <= 6000) {
            col_bg = IM_COL32(64, 64, 64, 240);
        }
    }

    igPushID_Ptr(label);
    igPushStyleColor_U32(ImGuiCol_TableRowBg, col_bg);
    igPushStyleColor_U32(ImGuiCol_Text, col_text);
    if (igBeginTable(
            label, 1, ImGuiTableFlags_RowBg,
            (ImVec2) { 0, 0 }, 0)) {
        igTableNextRow(ImGuiTableRowFlags_None, 0);
        igTableSetColumnIndex(0);
        igText("%s", current);
        igEndTable();
    }
    igPopStyleColor(2);
    igPopID();

    snprintf(last, n, "%s", current);
}

static void edit_status(editor_t *ed) {
    igPushID_Str("status");

    if (igBeginMenuBar()) {
        if (igBeginMenu("FILE", true)) {
            if (igMenuItem_Bool("OPEN", NULL, false, true)) {
                editor_ui_show_open_level(ed);
            }
            if (igMenuItem_Bool("SAVE", NULL, false, true)) {
                int res;
                if (!(res = editor_save_level(ed, NULL))) {
                    editor_show_error(ed, "error saving level: %d", res);
                }
            }
            if (igMenuItem_Bool("SAVE AS", NULL, false, true)) {
                editor_ui_show_saveas(ed);
            }
            if (igMenuItem_Bool("NEW", NULL, false, true)) {
                editor_ui_show_new_level(ed);
            }
            igEndMenu();
        }

        igEndMenuBar();
    }

    // file status
    char filestatus[sizeof(ed->filestatus.last)];
    snprintf(
        filestatus,
        sizeof(filestatus),
        "%s%s",
        ed->levelpath,
        ed->savedversion == ed->level->version ? "" : "*");
    change_highlight_text(
        ed,
        "filestatus",
        filestatus,
        ed->filestatus.last,
        sizeof(filestatus),
        &ed->filestatus.changetick,
        ed->savedversion != ed->level->version);

    // status line
    char statusline[sizeof(ed->statusline.last)];
    CURSOR_MODES[ed->cur.mode].status_line(
        ed, &ed->cur, statusline, sizeof(statusline));
    change_highlight_text(
        ed,
        "statusline",
        statusline,
        ed->statusline.last,
        sizeof(statusline),
        &ed->statusline.changetick,
        ed->cur.mode != CM_DEFAULT);

    // current mouse pos
    /*igPushStyleColor_U32(ImGuiCol_TableRowBg, IM_COL32(64, 64, 64, 64));
    igPushStyleColor_U32(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    if (igBeginTable(
            "cursor", 1, ImGuiTableFlags_RowBg,
            (ImVec2) { 0, 0 }, 0)) {
        igTableNextRow(ImGuiTableRowFlags_None, 0);
        igTableSetColumnIndex(0);
        char text[512];
        lptr_to_fancy_str(text, sizeof(text), ed->level, ed->cur.hover);
        igText("CURSOR: %s, %s", text, text);
        igEndTable();
    }
    igPopStyleColor(2); */

    // current hover
    igPushStyleColor_U32(ImGuiCol_TableRowBg, IM_COL32(64, 64, 64, 64));
    igPushStyleColor_U32(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    if (igBeginTable(
            "hover", 1, ImGuiTableFlags_RowBg,
            (ImVec2) { 0, 0 }, 0)) {
        igTableNextRow(ImGuiTableRowFlags_None, 0);
        igTableSetColumnIndex(0);
        char text[512];
        lptr_to_fancy_str(text, sizeof(text), ed->level, ed->cur.hover);
        igText("HOVER: %s", text);
        igEndTable();
    }
    igPopStyleColor(2);

    igPushItemWidth(INPUT_WIDTH_INT);
    {
        input_f32_clamped(
            "GRID SIZE",
            &ed->map.gridsize,
            0.10f, 0.5f,
            "%.2f",
            0,
            0.10f, 16.0f);
        ed->map.gridsize = roundf(ed->map.gridsize / 0.05f) * 0.05f;

        f32 scale = ed->map.scale;
        igSetNextItemWidth(INPUT_WIDTH_INT - 46);
        igInputFloat(
            "##scale", &scale, 0.0f, 0.0f,
            "%.3f", ImGuiInputTextFlags_None);

        igSameLine(0, igGetStyle()->ItemInnerSpacing.x);
        if (igButton("-", (ImVec2) { igGetFrameHeight(), igGetFrameHeight() })) {
            scale /= 2.0f;
        }

        igSameLine(0, igGetStyle()->ItemInnerSpacing.x);
        if (igButton("+", (ImVec2) { igGetFrameHeight(), igGetFrameHeight() })) {
            scale *= 2.0f;
        }

        // snap to nearest power of two
        int exp;
        frexp(scale, &exp);
        scale = powf(2.0f, exp - 1);

        if (fabsf(scale - ed->map.scale) > 0.0001f) {
            editor_map_set_scale(ed, scale);
        }

        sameline();
        igSetCursorPosX(igGetCursorPosX() - 4);
        igText("SCALE");
    }
    igPopItemWidth();


    igPushItemWidth(48);
    {
        igInputFloat("##camx", &ed->map.pos.x, 0, 0, "%.3f", 0);
        igSameLine(60, -1);
        igInputFloat("##camy", &ed->map.pos.y, 0, 0, "%.3f", 0);
        sameline();
        igSetCursorPosX(igGetCursorPosX() - 4);
        igText("CAMERA POS");
    }
    igPopItemWidth();

    igPushItemWidth(48);
    {
        if (igButton("MAP TO CAM", (ImVec2) { 0, 0 })) {
            editor_map_to_edcam(ed);
        }

        sameline();
        if (igButton("CAM TO MAP", (ImVec2) { 0, 0 })) {
            editor_edcam_to_map(ed);
        }
    }
    igPopItemWidth();

    if (igButton("NEW SIDEMAT", (ImVec2) { 0, 0 })) {
        editor_open_for_ptr(
            ed,
            LPTR_FROM(sidemat_new(ed->level)));
    }

    sameline();
    sidemat_t *o_sidemat = sidemat_select_button(ed, "EDIT SIDEMAT...");
    if (o_sidemat) {
        editor_open_for_ptr(ed, LPTR_FROM(o_sidemat));
    }

    if (igButton("NEW SECTMAT", (ImVec2) { 0, 0 })) {
        editor_open_for_ptr(
            ed,
            LPTR_FROM(sectmat_new(ed->level)));
    }

    sameline();
    sectmat_t *o_sectmat = sectmat_select_button(ed, "EDIT SECTMAT...");
    if (o_sectmat) {
        editor_open_for_ptr(ed, LPTR_FROM(o_sectmat));
    }

    if (igTreeNodeEx_Str("VISOPTS", ImGuiTreeNodeFlags_None)) {
        input_flags(
            &ed->visopt,
            VISOPT_MASK,
            visopt_names(),
            NULL,
            0);
        igTreePop();
    }

    igPushStyleColor_U32(ImGuiCol_TableRowBg, IM_COL32(64, 64, 64, 64));
    if (igBeginTable(
            "details", 1, ImGuiTableFlags_RowBg,
            (ImVec2) { 0, 0 }, 0)) {
        igTableNextRow(ImGuiTableRowFlags_None, 0);
        igTableSetColumnIndex(0);
        igText(
            "FRAME AVG (ms): %.1f | TPS: %" PRIu64,
            state->time.frame_avg / 1000000.0f,
            state->time.tps);
        igEndTable();
    }
    igPopStyleColor(1);
    igPopID();
}

static void show_stats(editor_t *ed) {
    igBegin("STATS", NULL, ImGuiWindowFlags_None);

/* #define STAT(_n, _fmt, ...)  \ */
/*     igTableNextRow(0, 0);    \ */
/*     igTableSetColumnIndex(0);\ */
/*     igText("%s", _n);        \ */
/*     igTableSetColumnIndex(1);\ */
/*     igText(_fmt, __VA_ARGS__) */

/*     if (igTreeNode_Str("RENDER")) { */
/*         if (igBeginTable( */
/*                 "RENDER", 2, ImGuiTableFlags_None, (ImVec2) { 0, 0 }, 0.0f)) { */
/*             STAT("TIME", "%.3f ms", state->stats.render.time / (f64) 1000000.0f); */
/*             STAT("FRAME (KiB)", "%f", state->stats.render.framebytes / 1024.0f); */
/*             STAT("AVG (KiB)", "%f", state->stats.render.avgbytes / 1024.0f); */
/*             STAT("NRPLANE", "%d", state->stats.render.nrplane); */
/*             STAT("NRPLANEDRAW", "%d", state->stats.render.nrplanedraw); */
/*             STAT("NRPLANEMERGE", "%d", state->stats.render.nrplanemerge); */
/*             STAT("NRPLANECOL", "%d", state->stats.render.nrplanecol); */
/*             STAT("NRSIDE", "%d", state->stats.render.nrside); */
/*             STAT("NRSIDECLIP", "%d", state->stats.render.nrsideclip); */
/*             STAT("NRSIDEDRAW", "%d", state->stats.render.nrsidedraw); */
/*             STAT("NRSPAN", "%d", state->stats.render.nrspan); */
/*             STAT("NPORTAL", "%d", state->stats.render.nportal); */
/*             STAT("NSECT", "%d", state->stats.render.nsect); */
/*             STAT("NSECTDISTINCT", "%d", state->stats.render.nsectdistinct); */
/*             STAT("NOBJECT", "%d", state->stats.render.nobject); */
/*             STAT("NSPRITECLIP", "%d", state->stats.render.nspriteclip); */
/*             STAT("NSPRITE", "%d", state->stats.render.nsprite); */
/*             STAT("NTCHAIN", "%d", state->stats.render.ntchain); */
/*             STAT("NTCHAINCOL", "%d", state->stats.render.ntchaincol); */
/*             igEndTable(); */
/*         } */

/*         igTreePop(); */
/*     } */


    igEnd();
#undef STAT
}

enum {
    PATH_MODAL_MUST_EXIST,
    PATH_MODAL_SAME_OKAY,
    PATH_MODAL_WARN_EXISTS
};

enum {
    PATH_MODAL_FLAG_NONE = 0,
    PATH_MODAL_FLAG_SAME = 1 << 0
};

typedef struct {
    const char *name;
    modal_state *pstate;
    char *existing;
    int type;
    void (*callback)(editor_t*, const char*, int);
} path_modal_info_t;

// modal dialog helper for status window menu items
static char *path_modal(
    const char *label,
    char *buf,
    usize n,
    modal_state *pstate,
    int type,
    const char *existing) {
    char *res = NULL;

    if (*pstate == MS_OPEN_NEW) {
        igSetKeyboardFocusHere(0);
    }

    if (igInputText(
            label,
            buf, n,
            ImGuiInputTextFlags_EnterReturnsTrue,
            NULL, NULL)) {
        res = buf;
    }
    sameline();

    const bool exists = file_exists(buf), isdir = file_isdir(buf);

    const char *warnreason = NULL;
    enum { ST_OKAY, ST_WARN, ST_ERR } state = ST_OKAY;

    if (type == PATH_MODAL_MUST_EXIST) {
        state = exists && !isdir ? ST_OKAY : ST_ERR;
    } else if (type == PATH_MODAL_SAME_OKAY) {
        warnreason = "THIS WILL OVERWRITE THE EXISTING FILE";
        state =
            (exists && isdir) ?
                ST_ERR :
                (exists && strcmp(buf, existing) ? ST_WARN : ST_OKAY);
    } else if (type == PATH_MODAL_WARN_EXISTS) {
        warnreason = "PATH ALREADY EXISTS";
        state = (exists && isdir) ? ST_ERR : (exists ? ST_WARN : ST_OKAY);
    }

    const ImVec4 colors[3] = {
        [ST_OKAY] = { 0.2f, 1.0f, 0.2f, 1.0f },
        [ST_WARN] = { 1.0f, 1.0f, 0.2f, 1.0f },
        [ST_ERR] = { 1.0f, 0.0f, 0.2f, 1.0f },
    };
    igTextColored(colors[state], "%s", exists ? "FOUND" : "NOT FOUND");
    igSpacing();

    // "GO" button
    {
        ImVec2 winsize;
        igGetWindowSize(&winsize);
        igSetCursorPosX((winsize.x - 50) / 2);
        if (igButton("GO", (ImVec2) { 50, 0 })) {
            res = buf;
        }
    }

    if (res) {
        if (state == ST_WARN) {
            igOpenPopup_Str("ARE YOU SURE?", ImGuiPopupFlags_None);
            res = NULL;
        } else if (state == ST_ERR) {
            res = NULL;
        }
    }

    // "are you sure" can overwrite previous res, returning even though there
    // is a warning
    if (igBeginPopupModal(
            "ARE YOU SURE?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (warnreason) {
            igText("%s", warnreason);
            igSpacing();
        }

        ImVec2 winsize, bsize;
        igGetWindowSize(&winsize);
        igCalcItemSize(&bsize, (ImVec2) { 50, 0 }, 0, 0);

        ImVec2 centerpos = {
            (winsize.x - bsize.x * 2 - igGetStyle()->ItemSpacing.x) / 2,
            igGetCursorPosY()
        };

        igSetCursorPos(centerpos);
        if (igButton("OK", (ImVec2) { 50, 0 })) {
            res = buf;
        }

        sameline();
        if (igButton("NOPE", (ImVec2) { 50, 0 })) {
            igCloseCurrentPopup();
        }
        igEndPopup();
    }

    if (res) { *pstate = MS_CLOSED; }
    return res;
}

static void path_modal_open(
    editor_t *ed, const char *path, int flags) {
    ed->reload.on = true;
    ed->reload.is_new = false;
    snprintf(
        ed->reload.level_path, sizeof(ed->reload.level_path), "%s", path);
}

static void path_modal_saveas(
    editor_t *ed, const char *path, int flags) {
    int res;
    if ((res = editor_save_level(ed, path))) {
        editor_show_error(ed, "error saving level: %d", res);
    }
}

static void path_modal_new(
    editor_t *ed, const char *path, int flags) {
    ed->reload.on = true;
    ed->reload.is_new = true;
    snprintf(
        ed->reload.level_path, sizeof(ed->reload.level_path), "%s", path);
}

static void show_modals(editor_t *ed) {
    // show error box if open
    if (igBeginPopupModal("ERROR", NULL, ImGuiWindowFlags_None)) {
        igTextColored((ImVec4) { 0.9f, 0.3f, 0.3f, 1.0f }, "%s", ed->errmsg);
        sameline();
        if (igButton("OK", (ImVec2) { 0, 0 })) {
            igCloseCurrentPopup();
        }
        igEndPopup();
    }

    // show path modals
    union {
        path_modal_info_t arr[3];
        struct {
            path_modal_info_t open, saveas, new;
        };
    } modals = {
        .arr = {
            {
                "FILE_OPEN",
                &ed->modal.open,
                ed->levelpath,
                PATH_MODAL_MUST_EXIST,
                path_modal_open,
            },
            {
                "FILE_SAVEAS",
                &ed->modal.saveas,
                ed->levelpath,
                PATH_MODAL_SAME_OKAY,
                path_modal_saveas,
            },
            {
                "FILE_NEW",
                &ed->modal.new,
                ed->levelpath,
                PATH_MODAL_WARN_EXISTS,
                path_modal_new,
            }
        }
    };

    for (int i = 0; i < (int) ARRLEN(modals.arr); i++) {
        path_modal_info_t *pm = &modals.arr[i];

        if (*pm->pstate == MS_OPEN_NEW) {
            igOpenPopup_Str(pm->name, ImGuiPopupFlags_None);
        }

        bool state = *pm->pstate != MS_CLOSED;
        if (igBeginPopupModal(
                pm->name, &state, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (path_modal(
                    "##path",
                    ed->modal.pathbuf,
                    sizeof(ed->modal.pathbuf),
                    pm->pstate,
                    pm->type,
                    pm->existing)) {
                pm->callback(
                    ed,
                    ed->modal.pathbuf,
                    !strcmp(ed->modal.pathbuf, pm->existing) ?
                        PATH_MODAL_FLAG_SAME : PATH_MODAL_FLAG_NONE);
                igCloseCurrentPopup();
            }

            if (*pm->pstate == MS_CLOSED) {
                igCloseCurrentPopup();
            }

            igEndPopup();
        }

        if (!state) {
            *pm->pstate = MS_CLOSED;
        } else if (*pm->pstate == MS_OPEN_NEW) {
            *pm->pstate = MS_OPEN;
        }
    }
}

void editor_show_error(editor_t *ed, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(ed->errmsg, sizeof(ed->errmsg), fmt, ap);
    va_end(ap);
    igOpenPopup_Str("ERROR", ImGuiPopupFlags_None);
}

void editor_show_ui(editor_t *ed) {
    show_modals(ed);

    if (ed->visopt & VISOPT_STATS) {
        show_stats(ed);
    }

    // open toopenptrs, remove tocloseptrs
    dynlist_each(ed->toopenptrs, it) {
        *dynlist_push(ed->openptrs) = *it.el;
    }
    dynlist_resize(ed->toopenptrs, 0);

    dynlist_each(ed->tocloseptrs, it0) {
        dynlist_each(ed->openptrs, it1) {
            if (LPTR_EQ(*it0.el, *it1.el)) {
                dynlist_remove_it(ed->openptrs, it1);
                break;
            }
        }
    }
    dynlist_resize(ed->tocloseptrs, 0);

    // show all open editors
    dynlist_each(ed->openptrs, it) {
        // remove from open list if closed, deleted or invalid
        if (!lptr_is_valid(ed->level, *it.el)
            || (*LPTR_FIELD(ed->level, *it.el, level_flags) & LF_DELETE)) {
            dynlist_remove_it(ed->openptrs, it);
            continue;
        }

        char label[64];
        lptr_to_str(label, sizeof(label), ed->level, *it.el);

        bool isnew = false;
        dynlist_each(ed->frontptrs, it_btf) {
            if (LPTR_EQ(*it.el, *it_btf.el)) {
                igSetNextWindowFocus();
                isnew = true;
                break;
            }
        }

        const bool colchange = editor_ptr_is_highlight(ed, *it.el);
        if (colchange) {
            const ImVec4 color = {
                editor_highlight_value(ed),
                0.03f,
                0.03f,
                1.0f,
            };

            igPushStyleColor_Vec4(ImGuiCol_TitleBg, color);
            /* igPushStyleColor_Vec4(ImGuiCol_TitleBgActive, color); */
            igPushStyleColor_Vec4(ImGuiCol_TitleBgCollapsed, color);
        }

        bool in_window = true;

        const int winflags =
            ImGuiWindowFlags_AlwaysAutoResize;

        bool is_open = true;
        igBegin(label, &is_open, winflags);

        // set EF_NEW if window has just been opened
        const int editflags = isnew ? EF_NEW : EF_NONE;

        if (LPTR_IS(*it.el, T_VERTEX)) {
            edit_vertex(
                ed,
                LPTR_VERTEX(ed->level, *it.el),
                editflags | EF_VERTEX_LIST_WALLS);
        } else if (LPTR_IS(*it.el, T_WALL | T_SIDE)) {
            side_t *side = LPTR_SIDE(ed->level, *it.el);
            wall_t *wall = LPTR_WALL(ed->level, *it.el);

            if (side) {
                wall = side->wall;
            } else {
                side = NULL;
            }

            lptr_to_str(label, sizeof(label), ed->level, LPTR_FROM(wall));
            edit_wall(ed, wall, side, editflags);
        } else if (LPTR_IS(*it.el, T_SECTOR)) {
            edit_sector(ed, LPTR_SECTOR(ed->level, *it.el), editflags);
        } else if (LPTR_IS(*it.el, T_SIDEMAT)) {
            edit_sidemat(ed, LPTR_SIDEMAT(ed->level, *it.el), editflags);
        } else if (LPTR_IS(*it.el, T_SECTMAT)) {
            edit_sectmat(ed, LPTR_SECTMAT(ed->level, *it.el), editflags);
        } else if (LPTR_IS(*it.el, T_DECAL)) {
            edit_decal(ed, LPTR_DECAL(ed->level, *it.el), editflags);
        } else if (LPTR_IS(*it.el, T_OBJECT)) {
            edit_object(ed, LPTR_OBJECT(ed->level, *it.el), editflags);
        } else {
            ASSERT(false, "trying to edit invalid lptr_t");
        }

        if (in_window
            && igIsWindowFocused(ImGuiFocusedFlags_None)) {
            editor_highlight_ptr(ed, *it.el);
        }

        if (in_window) {
            igEnd();

            if (!is_open) {
                editor_close_for_ptr(ed, *it.el);
            }
        }

        if (colchange) {
            igPopStyleColor(2);
        }
    }

    // reset bring to front pointers
    dynlist_resize(ed->frontptrs, 0);

    // show "STATUS" menu
    igBegin(
        "STATUS",
        NULL,
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_AlwaysAutoResize);
    {
        edit_status(ed);
    }
    igEnd();
}

void editor_ui_show_open_level(editor_t *ed) {
    snprintf(
        ed->modal.pathbuf, sizeof(ed->modal.pathbuf),
        "%s",
        ed->levelpath);
    ed->modal.open = MS_OPEN_NEW;
}

void editor_ui_show_saveas(editor_t *ed) {
    snprintf(
        ed->modal.pathbuf, sizeof(ed->modal.pathbuf),
        "%s",
        ed->levelpath);
    ed->modal.saveas = MS_OPEN_NEW;
}

void editor_ui_show_new_level(editor_t *ed) {
    snprintf(
        ed->modal.pathbuf, sizeof(ed->modal.pathbuf),
        "%s",
        ed->levelpath);
    ed->modal.new = MS_OPEN_NEW;
}
