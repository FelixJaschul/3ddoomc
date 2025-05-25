#pragma once

#include "gfx/renderer_types.h"
#include "imgui.h"

// cimgui is missing some imgui macros :(
#ifndef IM_COL32
#define IM_COL32(R, G, B, A)         \
    ((((u32) ((A) & 0xFF)) << 24) |  \
     (((u32) ((B) & 0xFF)) << 16) |  \
     (((u32) ((G) & 0xFF)) <<  8) |  \
     (((u32) ((R) & 0xFF)) <<  0))
#endif // ifndef IM_COL32

#include "defs.h"
#include "level/level_defs.h"
#include "level/lptr.h"
#include "util/dynlist.h"
#include "util/map.h"
#include "util/color.h"

#define EDITOR_SETTINGS_PATH "editor.ini"

#define EDITOR_BASE_SCALE 16.0f

// map configuration
#define MAP_DEFAULT_GRIDSIZE 1.0f

#define MAP_NORMAL_LENGTH_FOR_SCALE(_scale) min(0.3f / (_scale), 0.3f)
#define MAP_VERTEX_SIZE 0.20f
#define MAP_DECAL_SIZE 0.45f
#define MAP_OBJECT_SIZE 0.45f
#define MAP_SIDE_SELECT_DIST_FOR_SCALE(_scale) min(0.4f / (_scale), 0.4f)
#define MAP_WALL_SELECT_DIST 0.1f
#define MAP_GRID_POINT_SIZE 0.1f
#define MAP_LINE_THICKNESS 0.1f

#define MAP_SCALE_MAX 16.0f
#define MAP_SCALE_MIN 0.125f

// 2D map view colors
#define MAP_GRID_COLOR                   ABGR_TO_VEC4(0xFF333333)
#define MAP_VERTEX_COLOR                 ABGR_TO_VEC4(0xFFDD3311)
#define MAP_VERTEX_HOVER_COLOR           ABGR_TO_VEC4(0xFFFF9944)
#define MAP_SELECT_COLOR                 ABGR_TO_VEC4(0xFFCCCCCC)
#define MAP_WALL_COLOR                   ABGR_TO_VEC4(0xFF3333AA)
#define MAP_WALL_HOVER_COLOR             ABGR_TO_VEC4(0xFF8080FF)
#define MAP_WALL_CONNECT_COLOR           ABGR_TO_VEC4(0xFF44CC22)
#define MAP_SIDE_NORMAL_COLOR            ABGR_TO_VEC4(0xFF20C020)
#define MAP_SIDE_HOVER_COLOR             ABGR_TO_VEC4(0xFFFFFFFF)
#define MAP_SIDE_SELECT_COLOR            ABGR_TO_VEC4(0xFFFFFFFF)
#define MAP_SIDE_PORTAL_COLOR            ABGR_TO_VEC4(0xFF44FF11)
#define MAP_SIDE_PORTAL_DISCONNECT_COLOR ABGR_TO_VEC4(0xFFFF22FF)
#define MAP_SIDE_PORTAL_BAD_COLOR        ABGR_TO_VEC4(0xFF4040FF)
#define MAP_SELECT_BORDER_COLOR          ABGR_TO_VEC4(0xFFFFFFFF)
#define MAP_NEW_WALL_COLOR               ABGR_TO_VEC4(0x80999999)
#define MAP_NEW_VERTEX_COLOR             ABGR_TO_VEC4(0xFF999999)
#define MAP_SECTOR_HIGHLIGHT             ABGR_TO_VEC4(0x5533AAAA)
#define MAP_SIDE_ARROW_COLOR             ABGR_TO_VEC4(0xFFFF44FF)
#define MAP_DECAL_COLOR                  ABGR_TO_VEC4(0xFFFFAA66)
#define MAP_DECAL_HOVER_COLOR            ABGR_TO_VEC4(0xFFFFDD99)
#define MAP_OBJECT_COLOR                 ABGR_TO_VEC4(0xFF66AAFF)
#define MAP_OBJECT_HOVER_COLOR           ABGR_TO_VEC4(0xFF99DDFF)
#define MAP_FRIEND_COLOR                 ABGR_TO_VEC4(0xFF4080FF)

// controls
enum {
    BUTTON_NONE = 0,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_EDIT,
    BUTTON_SELECT,
    BUTTON_DESELECT,
    BUTTON_ZOOM_IN,
    BUTTON_ZOOM_OUT,
    BUTTON_SNAP,
    BUTTON_SNAP_WALL,
    BUTTON_MULTI_SELECT,
    BUTTON_CANCEL,
    BUTTON_MOVE_DRAG,
    BUTTON_SELECT_AREA,
    BUTTON_SWITCH_MODE,
    BUTTON_NEW_WALL,
    BUTTON_NEW_SIDE,
    BUTTON_NEW_VERTEX,
    BUTTON_EZPORTAL,
    BUTTON_SECTOR,
    BUTTON_DELETE,
    BUTTON_FUSE,
    BUTTON_SPLIT,
    BUTTON_CONNECT,
    BUTTON_PLANE_UP,
    BUTTON_PLANE_DOWN,
    BUTTON_SECTOR_UP,
    BUTTON_SECTOR_DOWN,
    BUTTON_BIGADJUST,
    BUTTON_SAVE,
    BUTTON_OPEN,
    BUTTON_NEW_DECAL,
    BUTTON_NEW_OBJECT,
    BUTTON_MOUSEMODE,
    BUTTON_MAP_TO_EDCAM,
    BUTTON_EDCAM_TO_MAP,
    BUTTON_CLOSE_ALL,
    BUTTON_COUNT
};

// see editor.c
extern const char *BUTTON_KEYS[BUTTON_COUNT];

// see editor::visopt
#define ENUM_VISOPT(_F, ...)             \
    _F(STATS,        1 << 0, __VA_ARGS__)  \
    _F(GRID,         1 << 1, __VA_ARGS__)  \
    _F(FULLBRIGHT,   1 << 2, __VA_ARGS__)  \
    _F(FLATSHADE,    1 << 3, __VA_ARGS__)  \
    _F(HIGHLIGHT,    1 << 4, __VA_ARGS__)  \
    _F(HOVERSECT,    1 << 5, __VA_ARGS__)  \
    _F(SHOWSECT,     1 << 6, __VA_ARGS__)  \
    _F(FLOORSECT,    1 << 7, __VA_ARGS__)  \
    _F(SIDELIST,     1 << 8, __VA_ARGS__)  \
    _F(SIDEWIND,     1 << 9, __VA_ARGS__)  \
    _F(DECALLIST,    1 << 10, __VA_ARGS__) \
    _F(OBJECTLIST,   1 << 11, __VA_ARGS__) \
    _F(BLOCKS,       1 << 12, __VA_ARGS__) \
    _F(FRIENDS,      1 << 13, __VA_ARGS__) \
    _F(GEO,          1 << 14, __VA_ARGS__) \
    _F(SUBNEIGHBORS, 1 << 15, __VA_ARGS__) \
    _F(PVS,          1 << 16, __VA_ARGS__) \

ENUM_MAKE(visopt, VISOPT, ENUM_VISOPT)

typedef struct editor editor_t;
typedef struct cursor cursor_t;

typedef enum {
    CM_DEFAULT,
    CM_SELECT,
    CM_DRAG,
    CM_MOVE_DRAG,
    CM_SELECT_AREA,
    CM_WALL,
    CM_VERTEX,
    CM_SIDE,
    CM_EZPORTAL,
    CM_SECTOR,
    CM_DELETE,
    CM_FUSE,
    CM_SPLIT,
    CM_CONNECT,
    CM_DECAL,
    CM_OBJECT,
    CM_MOVE_DECAL,
    CM_MOVE_OBJECT,
    CM_COUNT
} cursor_mode_index;

// cursor mode flags
enum {
    CMF_NONE            = 0,
    CMF_CLICK_CANCEL    = 1 << 0,
    CMF_NOHOVER         = 1 << 1,
    CMF_NODRAG          = 1 << 2,
    CMF_CAM             = 1 << 3,
    CMF_EXPLICIT_CANCEL = 1 << 4
};

typedef struct {
    cursor_mode_index mode;
    int flags, priority;

    bool (*trigger)(editor_t*, cursor_t*);
    void (*update)(editor_t*, cursor_t*);
    void (*render)(editor_t*, cursor_t*);
    void (*status_line)(editor_t*, cursor_t*, char*, usize);
    void (*cancel)(editor_t*, cursor_t*);
} cursor_mode_t;

// editor mode
typedef enum {
    EDITOR_MODE_MAP,
    EDITOR_MODE_CAM
} editor_mode;

typedef struct cursor {
    // current cursor position
    struct {
        vec2s screen;
        vec2s map;
    } pos;

    lptr_t hover;
    sector_t *sector;

    cursor_mode_index mode;

    // state for each mode
    union {
        struct {
            // ID (igGetID*) of GUI item is selecting
            ImGuiID item;

            // selected, NULL if nothing
            lptr_t selected;

            // accepted thing tags
            int tag;
        } mode_select_id;

        struct {
            // map of raw lptr_t -> vec2s start pos
            map_t *startmap;

            // statring cursor position
            vec2s start;
        } mode_drag;

        struct {
            // last SCREEN pos of cursor
            vec2s last;

            // true if any movement has occurred
            bool moved;
        } mode_move_drag;

        struct {
            // everything currently in area
            DYNLIST(lptr_t) ptrs;
            vec2s start, end;
        } mode_select_area;

        struct {
            lptr_t startptr;
            vec2s start;
            bool started;
        } mode_wall;

        struct {
            vec2s start;
            bool started;
        } mode_side;

        struct {
            vec2s start;
            bool started;
        } mode_ezportal;

        struct {
            lptr_t first;
        } mode_fuse;

        struct {
            side_t *first;
        } mode_connect;

        struct {
            decal_t *decal;
        } mode_move_decal;

        struct {
            object_t *object;
        } mode_move_object;

        struct {
            bool drag;
            vec2s dragstart;
        } mode_delete;
    };
} cursor_t;

// modal dialog states
// on OPEN_NEW, a new version of the modal is opened
typedef enum {
    MS_CLOSED,
    MS_OPEN_NEW,
    MS_OPEN
} modal_state;

typedef struct editor {
    level_t *level;

    ivec2s size;

    // button states for BUTTON_*
    u8 buttons[BUTTON_COUNT];

    // current file path
    char levelpath[1024];

    // current editor mode, determines which view is shown
    editor_mode mode;

    // current cursor state
    cursor_t cur;

    // current state of map editor
    struct {
        vec2s pos;
        f32 scale;
        f32 gridsize;

        // list of things which are selected
        DYNLIST(lptr_t) selected;
    } map;

    // object with OT_EDCAM in level
    object_t *camobj;

    // if true, mouse is grabbed in 3D mode (does not affect map mode where
    // mouse is never grabbed)
    bool mousegrab;

    // current (3D) mouse cursor info
    struct {
        lptr_t ptr;         // pointer (or LPTR_NULL)
        frag_data_t data;   // frag data
        sector_t *sect;     // sector associated with ptr (or NULL)
        plane_type plane;   // plane if directly pointed
        vec2s sect_pos;     // position on sector (world coords)
        side_t *side;       // side or NULL
        vec2s side_pos;     // position on side
    } cam;

    // TODO: convert lists below to dynlists

    // pointers which should be highlighted
    // new_highlight_ptrs are the pointers for the *next* frame to use
    struct {
        DYNLIST(lptr_t) newptrs, ptrs;
    } highlight;

    // currentl open ptrs for editing
    DYNLIST(lptr_t) openptrs;

    // ptrs to open on next frame
    DYNLIST(lptr_t) toopenptrs;

    // ptrs to close next frame
    DYNLIST(lptr_t) tocloseptrs;

    // things whose window should be brought to front
    DYNLIST(lptr_t) frontptrs;

    // statusline and filestatus change tracking
    struct {
        u64 changetick;
        char last[256];
    } filestatus, statusline;

    // if true, ui has mouse (don't interact with 3D world or 2D editor)
    bool ui_has_mouse;

    // if true, ui has keyboard focus (don't do any buttons)
    bool ui_has_keyboard;

#define ERRMSG_LENGTH 1024
    // current error message
    char errmsg[ERRMSG_LENGTH];

    // last saved level->version
    int savedversion;

    // TODO: persistent
    // visual options
    // see enum { VISOPT_* }
    int visopt;

    // state for modal dialogs
    struct {
        // persistent path buffer
        char pathbuf[1024];

        // modal dialog states
        modal_state open, saveas, new;
    } modal;

    // if "on", then level is loaded from levelpath on next editor frame
    struct {
        bool on;
        bool is_new;
        char level_path[1024];
    } reload;
} editor_t;

// "highlight" value which oscillates between 0..1 at a fixed rate.
// used to synchronize color highlights across editor components
f32 editor_highlight_value(editor_t *ed);

// returns true if ID was actually used to select something
bool editor_try_select_ptr(editor_t *ed, lptr_t ptr);

// open an editor for a specific ptr, bringing it to front if already open
void editor_open_for_ptr(editor_t *ed, lptr_t ptr);

// close all editors for specific ptr
void editor_close_for_ptr(editor_t *ed, lptr_t ptr);

// add pointer to list of currently highlighted pointers
void editor_highlight_ptr(editor_t *ed, lptr_t ptr);

// true if pointer is currently highlighted
bool editor_ptr_is_highlight(editor_t *ed, lptr_t ptr);

// resets editor (called in between level loads)
void editor_reset(editor_t *ed);

// initializes editor, only call once per program!
void editor_init(editor_t *ed);

// destroy/deinitialize editor
void editor_destroy(editor_t *ed);

// load editor settings from default path
void editor_load_settings(editor_t *ed);

// save editor settings to default path
void editor_save_settings(editor_t *ed);

// "do" editor frame
void editor_frame(editor_t *ed, level_t *level);

// opens level at path, if NULL reopens current level
// returns 0 on success
int editor_open_level(editor_t *ed, const char *path);

// saves level at path, if NULL saves to current level path
// returns 0 on success
int editor_save_level(editor_t *ed, const char *path);

// creates new level with path, cannot be NULL
// returns 0 on success
int editor_new_level(editor_t *ed, const char *path);
