#pragma once

#include "util/dynlist.h"
#include "defs.h"

typedef struct editor editor_t;
typedef struct cursor cursor_t;

// select ptr if not selected
void editor_map_select(editor_t *ed, lptr_t ptr);

// remove ptr from selections
void editor_map_deselect(editor_t *ed, lptr_t ptr);

// clear current selections
void editor_map_clear_select(editor_t *ed);

// true if ptr is selected
bool editor_map_is_select(editor_t *ed, lptr_t ptr);

// snap point to current grid
vec2s editor_map_snap_to_grid(editor_t *ed, vec2s p);

// un-transform a point p according to map camera (screen to level space)
vec2s editor_screen_to_map(editor_t *ed, vec2s p);

// center camera on point
void editor_map_center_on(editor_t *ed, vec2s p);

// set scale and keep center at the same point
void editor_map_set_scale(editor_t *ed, f32 newscale);

// TODO: holy bad name
// (E)ditor_(M)ap_(P)ointers_(I)n_(A)rea flags
enum {
    E_MPIA_NONE      = 0,

    // if set, only whole walls (walls *entirely* inside the area) are accepted
    E_MPIA_WHOLEWALL = 1 << 0
};

// get things between box (a, b)
int editor_map_ptrs_in_area(
    editor_t *ed,
    DYNLIST(lptr_t) *dst,
    vec2s a,
    vec2s b,
    int tag,
    int flags);

// locate ID which position is over and its sector
lptr_t editor_map_locate(editor_t *ed, vec2s pos, sector_t **sector_out);

// get cursor position according to current snapping rules
vec2s editor_map_snapped_cursor_pos(
    editor_t *ed,
    cursor_t *c,
    lptr_t *hover_out,
    sector_t **sector_out);

void editor_map_frame(editor_t *ed);

// snap editor map location to edcam location
void editor_map_to_edcam(editor_t *ed);

// snap editor edcam location to map location
void editor_edcam_to_map(editor_t *ed);
