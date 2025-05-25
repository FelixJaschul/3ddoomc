#pragma once

#include "util/types.h"
#include "config.h"
#include <sokol_gfx.h>

// Game modes
typedef enum {
    GAMEMODE_MENU,
    GAMEMODE_EDITOR,
    GAMEMODE_GAME
} game_mode;

// Object types
typedef enum {
    OT_PLACEHOLDER = 0,
    OT_TRIANGLE = 1,
    OT_CUBE = 2,
    OT_SPHERE = 3,
    OT_CYLINDER = 4,
    OT_CONE = 5,
    OT_TORUS = 6,
    OT_PLANE = 7,
    OT_CAMERA = 8,
    OT_LIGHT = 9,
    OT_COUNT
} object_type_index_t;

// Object type flags
enum {
    OTF_NONE            = 0,
    OTF_EDITACTIVE      = 1 << 0,
    OTF_INVISIBLE       = 1 << 1,
    OTF_NO_VERSION_BUMP = 1 << 2,
    OTF_LIGHT           = 1 << 3,
    OTF_CAMERA          = 1 << 4,
};

// Object flags
enum {
    OF_NONE = 0,
    OF_SELECTED = 1 << 0,
    OF_HIDDEN = 1 << 1,
    OF_LOCKED = 1 << 2,
};

// Forward declarations
typedef struct level level_t;
typedef struct editor editor_t;
typedef struct renderer renderer_t;
typedef struct gfx_state gfx_state_t;
typedef struct object object_t;
typedef struct triangle triangle_t;
typedef struct cube cube_t;
typedef struct sphere sphere_t;
typedef struct cylinder cylinder_t;
typedef struct cone cone_t;
typedef struct torus torus_t;
typedef struct plane plane_t;
typedef struct camera camera_t;
typedef struct light light_t;

// Object type structure
typedef struct object_type {
    const char *name;
    uint32_t flags;
    void (*init)(object_t *obj);
    void (*update)(object_t *obj);
    void (*render)(object_t *obj);
    void (*cleanup)(object_t *obj);
} object_type_t; 
