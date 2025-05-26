#pragma once

#include "util/math.h"
#include "util/types.h"
#include "defs.h"

// misc. support functions for the ui.c GUI

typedef struct editor editor_t;

typedef int ImGuiInputTextFlags;

// shortcut to keep element on same line
#define sameline() igSameLine(0, -1)

// fixed imgui element sizes
#define INPUT_WIDTH_INT 100
#define INPUT_WIDTH_INT_SMALL 50
#define INPUT_WIDTH_NAME 100
#define ITEM_WIDTH_NAME 130
#define INPUT_WIDTH_LPTR 100

#define INDENT_WIDTH_L0 5
#define INDENT_WIDTH_L1 20

// igInputInt, but you can clamp it
bool input_int_clamped(
    const char *label,
    int *v,
    int step,
    int step_fast,
    ImGuiInputTextFlags flags,
    int mi,
    int ma);

// igInputInt, but it's a u8
bool input_u8(
    const char *label,
    u8 *v,
    int step,
    int step_fast,
    ImGuiInputTextFlags flags);

// igInputInt, but it's a clamped u8
bool input_u8_clamped(
    const char *label,
    u8 *v,
    int step,
    int step_fast,
    ImGuiInputTextFlags flags,
    int mi,
    int ma);

// igInputInt, but it's a u16
bool input_u16(
    const char *label,
    u16 *v,
    int step,
    int step_fast,
    ImGuiInputTextFlags flags);

// igInputInt, but it's a i16
bool input_i16(
    const char *label,
    i16 *v,
    int step,
    int step_fast,
    ImGuiInputTextFlags flags);

// igInputInt, but it's a clamped u16
bool input_u16_clamped(
    const char *label,
    u16 *v,
    int step,
    int step_fast,
    ImGuiInputTextFlags flags,
    int mi,
    int ma);

// igInputFloat, but it's clamped
bool input_f32_clamped(
    const char *label,
    f32 *v,
    f32 step,
    f32 step_fast,
    const char *fmt,
    ImGuiInputTextFlags flags,
    f32 mi,
    f32 ma);

bool input_u8_flags(
    u8 *flags, u8 mask, const char **names, const int *groups, int ngroups);

bool input_flags(
    int *flags, int mask, const char **names, const int *groups, int ngroups);

// button_select_tag flags
enum {
    BST_NONE        = 0,
    BST_ALLOW_MULTI = 1 << 0
};

// button which changes cursor mode to SELECT and returns LPTR_NULL when a
// selection is made
lptr_t button_select_tag(
    editor_t *ed,
    const char *label,
    int tag,
    int flags);

// input which shows an LPTR as a string AND has a "SELECT" button to change it
// returns non-LPTR_NULL when selection is made
lptr_t input_lptr(
    editor_t *ed,
    const char *label,
    int tag,
    lptr_t value);

typedef bool (*input_copy_apply__setfield)(lptr_t, void*, void*, int, void*);

// create a copy button ("CPY"), apply ("APL"), and a label ("label") which
// will select lptr_ts of the specified tag and copy/apply the specified field
// to other lptr_ts of the same type
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
    void *userdata);

// show 20x20 image of texture by name (hover to zoom), returns true if clicked
bool texture_image(resource_t resource);

// flags to control ui_search_select_popup
enum {
    UI_SEARCH_SELECT_POPUP_NONE     = 0,
    UI_SEARCH_SELECT_POPUP_NOLABEL  = (1 << 0),
    UI_SEARCH_SELECT_POPUP_PACK     = (1 << 1),
    UI_SEARCH_SELECT_POPUP_DELETE   = (1 << 2),
};

// returns -1 if no result, otherwise closes itself and returns index into
// base array of selection
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
    int flags);

bool input_enum(
    const char *label,
    void *pvalue,
    int valsize,
    int distinct,
    const char **names,
    int (*nth)(int));

int texture_select_popup(
    editor_t *ed,
    const char *label);

// do popup for a sidemat selection
sidemat_t *sidemat_select_popup(
    editor_t *ed,
    const char *label);

// button which allows selection of sidemat
sidemat_t *sidemat_select_button(
    editor_t *ed,
    const char *label);

// do popup for sectmat selection
sectmat_t *sectmat_select_popup(
    editor_t *ed,
    const char *label);

// button which allows selection of sectmat
sectmat_t *sectmat_select_button(
    editor_t *ed,
    const char *label);

// texture icon which, when clicked, can be changed via a popup list
// also includes x, y offsets
bool texture_select_offset(
    editor_t *ed,
    const char *label,
    resource_t *resource,
    ivec2s *offset);

// texture icon which, when clicked, can be changed via a popup list
bool texture_select(
    editor_t *ed,
    const char *label,
    resource_t *resource);

bool input_tag(editor_t *ed, lptr_t ptr, int *ptag);

bool input_resource_name(editor_t *ed, const char *label, resource_t *res);
