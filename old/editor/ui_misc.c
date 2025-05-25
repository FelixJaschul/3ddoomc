#include "editor/ui_misc.h"
#include "editor/editor.h"
#include "editor/ui.h"
#include "gfx/atlas.h"
#include "level/level.h"
#include "level/sidemat.h"
#include "level/sectmat.h"
#include "level/tag.h"
#include "util/input.h"
#include "state.h"

bool input_int_clamped(
    const char *label,
    int *v,
    int step,
    int step_fast,
    ImGuiInputTextFlags flags,
    int mi,
    int ma) {
    int u = *v;
    if (igInputInt(label, &u, step, step_fast, ImGuiInputTextFlags_None)) {
        u = clamp(u, mi, ma);
        if (u != *v) {
            *v = u;
            return true;
        }
    }

    return false;
}

bool input_u8(
    const char *label,
    u8 *v,
    int step,
    int step_fast,
    ImGuiInputTextFlags flags) {
    int u = *v;
    if (input_int_clamped(label, &u, step, step_fast, flags, 0, U8_MAX)) {
        *v = u;
        return true;
    }
    return false;
}

bool input_u8_clamped(
    const char *label,
    u8 *v,
    int step,
    int step_fast,
    ImGuiInputTextFlags flags,
    int mi,
    int ma) {
    mi = clamp(mi, 0, U8_MAX);
    ma = clamp(ma, 0, U8_MAX);
    int u = *v;
    if (input_int_clamped(label, &u, step, step_fast, flags, mi, ma)) {
        *v = u;
        return true;
    }
    return false;
}

bool input_u16(
    const char *label,
    u16 *v,
    int step,
    int step_fast,
    ImGuiInputTextFlags flags) {
    int u = *v;
    if (input_int_clamped(label, &u, step, step_fast, flags, 0, U16_MAX)) {
        *v = u;
        return true;
    }
    return false;
}

bool input_i16(
    const char *label,
    i16 *v,
    int step,
    int step_fast,
    ImGuiInputTextFlags flags) {
    int u = *v;
    if (input_int_clamped(
            label, &u, step, step_fast, flags, I16_MIN, I16_MAX)) {
        *v = u;
        return true;
    }
    return false;
}

bool input_u16_clamped(
    const char *label,
    u16 *v,
    int step,
    int step_fast,
    ImGuiInputTextFlags flags,
    int mi,
    int ma) {
    int u = *v;
    mi = clamp(mi, 0, U16_MAX);
    ma = clamp(ma, 0, U16_MAX);
    if (input_int_clamped(label, &u, step, step_fast, flags, mi, ma)) {
        *v = u;
        return true;
    }
    return false;
}

bool input_f32_clamped(
    const char *label,
    f32 *v,
    f32 step,
    f32 step_fast,
    const char *fmt,
    ImGuiInputTextFlags flags,
    f32 mi,
    f32 ma) {
    f32 u = *v;
    if (igInputFloat(label, &u, step, step_fast, fmt, flags)) {
        u = clamp(u, mi, ma);
        if (fabsf(*v - u) > GLM_FLT_EPSILON) {
            *v = u;
            return true;
        }
    }
    return false;
}

lptr_t button_select_tag(
    editor_t *ed,
    const char *label,
    int tag,
    int flags) {
    char label_button[64];
    snprintf(label_button, sizeof(label_button), "%s##button", label);

    ImGuiID id = igGetID_Str(label_button);
    if (igButton(label_button, (ImVec2) { 0, 0 })) {
        ed->cur.mode = CM_SELECT;
        ed->cur.mode_select_id.item = id;
        ed->cur.mode_select_id.tag = tag;
        ed->cur.mode_select_id.selected = LPTR_NULL;
    }

    lptr_t res = LPTR_NULL;

    // are we currently selecting?
    if (ed->cur.mode == CM_SELECT
            && ed->cur.mode_select_id.item == id
            && !LPTR_IS_NULL(ed->cur.mode_select_id.selected)) {
        if (LPTR_IS(ed->cur.mode_select_id.selected, tag)) {
            res = ed->cur.mode_select_id.selected;
        }

        // if something was selected, change back to normal mode unless button
        // allows multiple selections and multi select button is down
        if (!(flags & BST_ALLOW_MULTI)
            || !(ed->buttons[BUTTON_MULTI_SELECT] & INPUT_DOWN)) {
            ed->cur.mode = CM_DEFAULT;
        }
    }

    return res;
}

lptr_t input_lptr(
    editor_t *ed,
    const char *label,
    int tag,
    lptr_t value) {
    igBeginGroup();
    igPushID_Str(label);

    char buf[128];
    lptr_to_str(buf, sizeof(buf), ed->level, value);

    igSetNextItemWidth(INPUT_WIDTH_LPTR);
    igAlignTextToFramePadding();
    igText("%s (%s)", label, buf);

    sameline();

    lptr_t newptr = button_select_tag(ed, "SELECT", tag, BST_NONE);

    igPopID();
    igEndGroup();

    if (igIsItemHovered(ImGuiHoveredFlags_None)) {
        editor_highlight_ptr(ed, value);
    }

    return newptr;
}

bool input_copy_apply(
    editor_t *ed,
    const char *label,
    lptr_t ptr,
    void *field,
    int fieldsize,
    const int *tags,
    const int *tagoffsets,
    int ntags,
    input_copy_apply__setfield setfield,
    void *userdata) {
    igPushID_Ptr(field);
    igPushID_Str(label);

    bool change = false;

    // create union of all tags, map tag index T_*_INDEX -> index
    int alltag = 0;

    int tagindices[T_COUNT];
    memset(tagindices, 0xFF, sizeof(tagindices));

    for (int i = 0; i < ntags; i++) {
        alltag |= tags[i];
        tagindices[ctz(tags[i]) + 1] = i;
    }

    lptr_t copy = button_select_tag(ed, "CPY", alltag, BST_NONE);
    if (!LPTR_IS_NULL(copy)) {
        void *other = lptr_raw_ptr(ed->level, copy);
        const int tagindex = tagindices[ctz(LPTR_TYPE(copy)) + 1];
        ASSERT(tagindex != -1);

        if (setfield) {
            change |=
                setfield(
                    ptr,
                    field,
                    &((u8*) other)[tagoffsets[tagindex]],
                    fieldsize,
                    userdata);
        } else {
            u8 old[fieldsize];
            memcpy(old, field, fieldsize);
            memcpy(field, &((u8*) other)[tagoffsets[tagindex]], fieldsize);
            change |= memcmp(old, field, fieldsize);
        }
    }

    sameline();
    lptr_t apply = button_select_tag(ed, "APL", alltag, BST_ALLOW_MULTI);
    if (!LPTR_IS_NULL(apply)) {
        void *other = lptr_raw_ptr(ed->level, apply);
        const int tagindex = tagindices[ctz(LPTR_TYPE(apply)) + 1];
        ASSERT(tagindex != -1);

        if (setfield) {
            setfield(
                apply,
                &((u8*) other)[tagoffsets[tagindex]],
                field,
                fieldsize,
                userdata);
        } else {
            memcpy(&((u8*) other)[tagoffsets[tagindex]], field, fieldsize);
        }

        lptr_recalculate(ed->level, apply);
    }

    if (strlen(label) != 0) {
        sameline();
        igText("%s", label);
    }

    igPopID();
    igPopID();
    return change;
}

bool texture_image(resource_t resource) {
    bool res = false;

    // TODO: going to break with multiple atlas layers
    atlas_lookup_t lookup;
    atlas_lookup(state->atlas, resource, &lookup);

    igImage(
        (ImTextureID) (uintptr_t)
            state->atlas->layer_images[lookup.layer].id,
        (ImVec2) { 20, 20 },
        (ImVec2) { lookup.box_uv.min.x, lookup.box_uv.max.y },
        (ImVec2) { lookup.box_uv.max.x, lookup.box_uv.min.y },
        (ImVec4) { 1, 1, 1, 1 },
        (ImVec4) { 1, 1, 1, 1 });

    if (igIsItemClicked(ImGuiMouseButton_Left)) {
        res = true;
    }

    if (igIsItemHovered(ImGuiHoveredFlags_None)) {
        igBeginTooltip();
        igText(resource.name);
        igImage(
            (ImTextureID) (uintptr_t)
                state->atlas->layer_images[lookup.layer].id,
            (ImVec2) { 128, 128 },
            (ImVec2) { lookup.box_uv.min.x, lookup.box_uv.max.y },
            (ImVec2) { lookup.box_uv.max.x, lookup.box_uv.min.y },
            (ImVec4) { 1, 1, 1, 1 },
            (ImVec4) { 1, 1, 1, 1 });
        igEndTooltip();
    }

    return res;
}

int search_select_popup(
    const char *label,
    void *base,
    usize nel,
    usize width,
    usize maxname,
    bool (*show)(const void*, void*),
    const char *(*getname)(const void*, void*),
    bool (*can_delete)(const void*, void*),
    void (*delete)(const void*, void*),
    bool (*do_ui)(const void*, void*),
    void *userdata,
    int flags) {
    if (!igBeginPopup(label, ImGuiWindowFlags_Popup)) {
        return -1;
    }

    const usize searchsize = maxname ? (maxname + 1) : 256;
    char search[searchsize];
    memset(search, 0, searchsize * sizeof(char));

    // if enter is pressed select first result
    const bool select_first =
        igInputText(
            "search", search, searchsize,
            ImGuiInputTextFlags_EnterReturnsTrue,
            NULL, NULL);

    const bool do_search = search[0] != '\0';

    // width of one row
    const f32 row_width = 200.0f;

    // width of one element
    f32 el_width = 0.0f;

    usize n = 0, num_deleted = 0;
    for (usize i = 0; i < nel - num_deleted; i++) {
        const void *el = base + (i * width);

        if (show && !show(el, userdata)) {
            continue;
        }

        const char *name = getname(el, userdata);

        if (do_search && !strcasestr(name, search)) {
            continue;
        }

        if (select_first && n == 0) {
            igCloseCurrentPopup();
            igEndPopup();
            return i;
        }


        if (!(flags & UI_SEARCH_SELECT_POPUP_NOLABEL)) {
            igText("%s (%d)", name, i);
            igSameLine(100, -1);
        }

        if ((flags & UI_SEARCH_SELECT_POPUP_PACK)
            && n != 0
            && ((int) (((n - 1) * el_width) / row_width))
                == ((int) ((n * el_width) / row_width))) {
            sameline();
        }

        bool clicked = false, deleted = false;
        igPushID_Int(i);
        igBeginGroup();
        {
            clicked = do_ui(el, userdata);

            if (n == 0) {
                ImVec2 size;
                igGetItemRectSize(&size);
                el_width = size.x;
            }
        }
        igEndGroup();
        clicked |= igIsItemClicked(ImGuiMouseButton_Left);

        if ((flags & UI_SEARCH_SELECT_POPUP_DELETE)
            && can_delete(el, userdata)) {
            sameline();

            char label[64];
            snprintf(label, sizeof(label), "X##%d", (int) i);
            if (igButton(label, (ImVec2) { 0, 0 })) {
                delete(el, userdata);
                deleted = true;
                num_deleted++;
            }
        }

        igPopID();

        if (!deleted && clicked) {
            igCloseCurrentPopup();
            igEndPopup();
            return i;
        }

        n++;
    }

    igEndPopup();
    return -1;
}

static const char *input_enum__getname(const void *p, void*) {
    return *((const char **) p);
}

static bool input_enum__do_ui(const void *p, void*) {
    return igSelectable_Bool(
            *((const char **) p),
            false,
            ImGuiSelectableFlags_None,
            (ImVec2) { 0, 0 });
}

bool input_enum(
    const char *label,
    void *pvalue,
    int valsize,
    int distinct,
    const char **names,
    int (*nth)(int)) {
    igPushID_Ptr(pvalue);
    bool change = false;
    // TODO: this might be UB ¯\_(ツ)_/¯
    int ivalue = *(int*) (pvalue);

    igAlignTextToFramePadding();
    igText("%s (%d)", names[ivalue], ivalue);

    char popuplabel[256];
    snprintf(popuplabel, sizeof(popuplabel), "%s##popup", label);

    sameline();
    if (igButton("...", (ImVec2) { 0, 0 })) {
        igOpenPopup_Str(popuplabel, ImGuiPopupFlags_None);
    }

    const int i =
        search_select_popup(
            popuplabel,
            &names[0],
            distinct,
            sizeof(const char*),
            64,
            NULL,
            input_enum__getname,
            NULL,
            NULL,
            input_enum__do_ui,
            NULL,
            UI_SEARCH_SELECT_POPUP_NOLABEL);

    if (i != -1) {
        const int newval = nth ? nth(i) : i;
        if (memcmp(pvalue, &newval, valsize)) {
            memcpy(pvalue, &newval, valsize);
            change = true;
        }
    }

    sameline();
    igText("%s", label);
    igPopID();
    return change;
}

static const char *texture_select_popup__getname(
    const void *p,
    void*) {
    return *((const char**) p);
}

static bool texture_select_popup__do_ui(
    const void *p,
    void*) {
    return texture_image(resource_from("%s", *(const char **)p));
}

int texture_select_popup(
    editor_t *ed,
    const char *label) {
    return
        search_select_popup(
            label,
            state->atlas->names,
            dynlist_size(state->atlas->names),
            sizeof(const char*),
            RESOURCE_NAME_LEN,
            NULL,
            texture_select_popup__getname,
            NULL,
            NULL,
            texture_select_popup__do_ui,
            ed,
            UI_SEARCH_SELECT_POPUP_NOLABEL
            | UI_SEARCH_SELECT_POPUP_PACK);
}

static bool sidemat_select_popup__show(
    const void *p,
    void*) {
    return !!(*((sectmat_t**) p));
}

static const char *sidemat_select_popup__getname(
    const void *p,
    void*) {
    return (*((sidemat_t**) p))->name.name;
}

static bool sidemat_select_popup__can_delete(
    const void *p,
    void *ex) {
    return (*((sidemat_t**) p))->index != 0;
}

static void sidemat_select_popup__delete(
    const void *p,
    void *ex) {
    editor_t *ed = ex;
    sidemat_delete(ed->level, *((sidemat_t**) p));
}

static bool sidemat_select_popup__do_ui(
    const void *p,
    void*) {
    const sidemat_t *mat = *((sidemat_t**) p);
                texture_image(mat->tex_low);
    sameline(); texture_image(mat->tex_mid);
    sameline(); texture_image(mat->tex_high);
    return false;
}

sidemat_t *sidemat_select_popup(
    editor_t *ed,
    const char *label) {
    const int res =
        search_select_popup(
            label,
            &ed->level->sidemats[0],
            dynlist_size(ed->level->sidemats),
            sizeof(sidemat_t*),
            RESOURCE_NAME_LEN,
            sidemat_select_popup__show,
            sidemat_select_popup__getname,
            sidemat_select_popup__can_delete,
            sidemat_select_popup__delete,
            sidemat_select_popup__do_ui,
            ed,
            UI_SEARCH_SELECT_POPUP_DELETE);

    return res == -1 ? NULL : ed->level->sidemats[res];
}

sidemat_t *sidemat_select_button(
    editor_t *ed,
    const char *label) {
    char lbutton[256];
    snprintf(lbutton, sizeof(lbutton), "%s##button", label);
    if (igButton(label, (ImVec2) { 0, 0 })) {
        igOpenPopup_Str(label, ImGuiPopupFlags_None);
    }
    return sidemat_select_popup(ed, label);
}

static bool sectmat_select_popup__show(
    const void *p,
    void*) {
    return !!(*((sectmat_t**) p));
}

static const char *sectmat_select_popup__getname(
    const void *p,
    void*) {
    return (*((sectmat_t**) p))->name.name;
}

static bool sectmat_select_popup__can_delete(
    const void *p,
    void *ex) {
    return (*((sectmat_t**) p))->index != 0;
}

static void sectmat_select_popup__delete(
    const void *p,
    void *ex) {
    editor_t *ed = ex;
    sectmat_delete(
        ed->level,
        *((sectmat_t**) p));
}

static bool sectmat_select_popup__do_ui(
    const void *p,
    void *ex) {
    const sectmat_t *mat = *((sectmat_t**) p);
                texture_image(mat->floor.tex);
    sameline(); texture_image(mat->ceil.tex);
    return false;
}

sectmat_t *sectmat_select_popup(
    editor_t *ed,
    const char *label) {
    const int res =
        search_select_popup(
            label,
            &ed->level->sectmats[0],
            dynlist_size(ed->level->sectmats),
            sizeof(sectmat_t*),
            RESOURCE_NAME_LEN,
            sectmat_select_popup__show,
            sectmat_select_popup__getname,
            sectmat_select_popup__can_delete,
            sectmat_select_popup__delete,
            sectmat_select_popup__do_ui,
            ed,
            UI_SEARCH_SELECT_POPUP_DELETE);

    return res == -1 ? NULL : ed->level->sectmats[res];
}

sectmat_t *sectmat_select_button(
    editor_t *ed,
    const char *label) {
    char lbutton[256];
    snprintf(lbutton, sizeof(lbutton), "%s##button", label);
    if (igButton(label, (ImVec2) { 0, 0 })) {
        igOpenPopup_Str(label, ImGuiPopupFlags_None);
    }
    return sectmat_select_popup(ed, label);
}

bool texture_select_offset(
    editor_t *ed,
    const char *label,
    resource_t *resource,
    ivec2s *offset) {
    igPushID_Str(label);
    bool change = false;

    // image, clicking opens popup
    if (texture_image(*resource)) {
        igOpenPopup_Str(label, ImGuiPopupFlags_None);
    }

    sameline();
    igPushItemWidth(INPUT_WIDTH_INT);
    {
        change |=
            igInputInt(
                "##ox", &offset->x,
                1, 4, ImGuiInputTextFlags_None);

        sameline();
        change |=
            igInputInt(
                "##oy", &offset->y,
                1, 4, ImGuiInputTextFlags_None);
    }
    igPopItemWidth();

    int id;
    if ((id = texture_select_popup(ed, label)) != -1) {
        *resource = resource_from("%s", state->atlas->names[id]);
        change = true;
    }

    igPopID();
    return change;
}

bool texture_select(
    editor_t *ed,
    const char *label,
    resource_t *resource) {
    igPushID_Str(label);
    igBeginGroup();
    bool change = false;

    // image, clicking opens popup
    if (texture_image(*resource)) {
        igOpenPopup_Str(label, ImGuiPopupFlags_None);
    }

    int id;
    if ((id = texture_select_popup(ed, label)) != -1) {
        *resource = resource_from("%s", state->atlas->names[id]);
        change = true;
    }

    igPopID();
    igEndGroup();
    return change;
}

bool input_u8_flags(
    u8 *flags, u8 mask, const char **names, const int *groups, int ngroups) {
    bool change = false;

    int n = 0, ng = 0, g = 0;
    for (int i = 0; i < 8; i++) {
        if (!(mask & (1 << i))) {
            continue;
        }

        if (groups) {
            if (ng >= groups[g]) {
                ng = 0;
                g++;
            } else if (n != 0) {
                sameline();
            }
        }

        bool value = !!(*flags & (1 << i));
        if (igCheckbox(names[n], &value)) {
            change = true;
            *flags = (*flags & ~(1 << i)) | (value ? (1 << i) : 0);
        }

        n++;
        ng++;
    }

    return change;
}

bool input_flags(
    int *flags, int mask, const char **names, const int *groups, int ngroups) {
    bool change = false;

    int n = 0, ng = 0, g = 0;
    for (u32 i = 0; i < 32; i++) {
        if (!(mask & (1u << i))) {
            continue;
        }

        if (groups) {
            if (ng >= groups[g]) {
                ng = 0;
                g++;
            } else if (n != 0) {
                sameline();
            }
        }

        bool value = !!(*flags & (1u << i));
        if (igCheckbox(names[n], &value)) {
            change = true;
            *flags = (*flags & ~(1u << i)) | (value ? (1u << i) : 0);
        }

        n++;
        ng++;
    }

    return change;
}

static bool input_tag__setfield(
    lptr_t ptr, void *dst, void *src, int n, void *userdata) {
    editor_t *ed = userdata;

    u16 tag;
    ASSERT(n == sizeof(tag));
    memcpy(&tag, src, n);
    tag_set(ed->level, ptr, tag);
    return true;
}

bool input_tag(
    editor_t *ed,
    lptr_t ptr,
    int *ptag) {
    bool change = false;
    igPushID_Ptr(ptag);
    igBeginGroup();

    // don't set *ptag directly, we must go through l_set_tag
    int tag = *ptag;

    igSetNextItemWidth(63);
    change |=
        input_int_clamped(
            "##tag",
            &tag,
            0, 0,
            ImGuiInputTextFlags_None,
            0, TAG_MAX - 1);

    sameline();

    // show friends
    DYNLIST(lptr_t) *friends = &ed->level->tag_lists[tag];
    const int nfriends = dynlist_size(*friends);

    if (nfriends <= 2) {
        igBeginDisabled(nfriends <= 1);
        const lptr_t other =
            nfriends <= 1 ?
                LPTR_NULL
                : (*friends)[LPTR_EQ(ptr, (*friends)[0]) ? 1 : 0];

        if (igButton("FRI", (ImVec2) { 0, 0 })) {
            editor_open_for_ptr(ed, other);
        }

        if (nfriends == 2) {
            char name[256];
            lptr_to_str(name, sizeof(name), ed->level, other);

            if (igIsItemHovered(ImGuiHoveredFlags_None)) {
                editor_highlight_ptr(ed, other);
                igBeginTooltip();
                igText(name);
                igEndTooltip();
            }
        }
        igEndDisabled();
    } else {
        if (igButton("...", (ImVec2) { 0, 0 })) {
            igOpenPopup_Str("FRIENDS", ImGuiPopupFlags_None);
        }

        // highlight all friends if hovered
        if (igIsItemHovered(ImGuiHoveredFlags_None)) {
            for (int i = 0; i < nfriends; i++) {
                if (LPTR_EQ(ptr, (*friends)[i])) {
                    continue;
                }

                editor_highlight_ptr(ed, (*friends)[i]);
            }
        }

        if (igBeginPopup("FRIENDS", ImGuiWindowFlags_None)) {
            for (int i = 0; i < nfriends; i++) {
                if (LPTR_EQ(ptr, (*friends)[i])) {
                    continue;
                }

                char name[256];
                lptr_to_str(name, sizeof(name), ed->level, (*friends)[i]);

                if (igButton(name, (ImVec2) { 0, 0 })) {
                    editor_open_for_ptr(ed, (*friends)[i]);
                }

                if (igIsItemHovered(ImGuiHoveredFlags_None)) {
                    editor_highlight_ptr(ed, (*friends)[i]);
                }
            }
            igEndPopup();
        }
    }

    sameline();
    if (igButton("SUG", (ImVec2) { 0, 0 })) {
        const u16 newtag = tag_suggest(ed->level);
        if (newtag == TAG_NONE) {
            editor_show_error(ed, "no more tags :(");
        } else {
            tag = newtag;
            change = true;
        }
    }

    // input_tag__setfield does its own l_set_tag, so l_set_tag if we have any
    // changes up to this point
    if (change) {
        tag_set(ed->level, ptr, tag);
    }

    sameline();
    change |=
        input_copy_apply(
            ed, "TAG",
            ptr,
            &tag,
            sizeof(u16),
            (int[]) { T_DECAL, T_SECTOR, T_SIDE },
            (int[]) {
                offsetof(decal_t, tag),
                offsetof(sector_t, tag),
                offsetof(side_t, tag)
            },
            3,
            input_tag__setfield, ed);

    igEndGroup();
    igPopID();
    return change;
}

bool input_resource_name(editor_t *ed, const char *label, resource_t *res) {
    resource_t r = *res;
    igSetNextItemWidth(ITEM_WIDTH_NAME);
    if (igInputText(
            label, r.name, sizeof(r.name),
            ImGuiInputTextFlags_CharsNoBlank
            | ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL)) {
        if (resource_name_valid(r.name)) {
            *res = r;
            return true;
        }
    }

    return false;
}
