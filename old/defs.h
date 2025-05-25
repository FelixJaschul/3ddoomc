#pragma once

#include "util/enum.h"
#include "util/types.h"
#include "shared_defs.h"
#include "config.h"

enum { VERSION = 1 };

typedef enum {
    GAMEMODE_MENU,
    GAMEMODE_EDITOR,
    GAMEMODE_GAME
} game_mode;

// level thing flags ("u8 level_flags" on each struct)
enum {
    LF_NONE             = 0,
    LF_DIRTY            = 1 << 0,
    LF_DELETE           = 1 << 1,
    LF_FAILTRACE        = 1 << 2,
    LF_DO_NOT_RECALC    = 1 << 4,
    LF_VISIBILITY       = 1 << 5,
    LF_TRACED           = 1 << 6,
    LF_MARK             = 1 << 7,
};

// type of sector plane
// NOTE: must also function as indices into sector.plane[] array
typedef enum {
    _PLANE_TYPE_FLOOR = PLANE_TYPE_FLOOR,
    _PLANE_TYPE_CEIL = PLANE_TYPE_CEIL
} plane_type;

// object types
#define ENUM_OBJECT_TYPE_INDEX(_F, ...)    \
    _F(PLACEHOLDER,  0, __VA_ARGS__) \
    _F(EDCAM,        1, __VA_ARGS__) \
    _F(PROJ1,        2, __VA_ARGS__) \
    _F(PROJ2,        3, __VA_ARGS__) \
    _F(BOY,          4, __VA_ARGS__) \
    _F(SPAWN,        5, __VA_ARGS__) \
    _F(PLAYER,       6, __VA_ARGS__) \
    _F(JUICE0,       7, __VA_ARGS__) \
    _F(JUICE1,       8, __VA_ARGS__) \
    _F(JUICE2,       9, __VA_ARGS__) \
    _F(VILLAGER0,   10, __VA_ARGS__) \
    _F(PICKUP,      11, __VA_ARGS__) \

ENUM_MAKE(object_type_index, OT, ENUM_OBJECT_TYPE_INDEX)

// object type flags
enum {
    OTF_NONE            = 0,
    OTF_EDITACTIVE      = 1 << 0,
    OTF_INVISIBLE       = 1 << 1,
    OTF_NO_VERSION_BUMP = 1 << 2,
    OTF_PROJECTILE      = 1 << 3,
};

// object flags
enum {
    OF_NONE = 0
};

enum {
    OS_DEFAULT,
    OS_DESTROY,
};

// decal types
#define ENUM_DECAL_TYPE_INDEX(_F, ...)      \
    _F(PLACEHOLDER, 0, __VA_ARGS__)   \
    _F(SWITCH, 1, __VA_ARGS__)        \
    _F(BUTTON, 2, __VA_ARGS__)        \
    _F(RIFT, 3, __VA_ARGS__)        \

ENUM_MAKE(decal_type_index, DT, ENUM_DECAL_TYPE_INDEX)

// decal type flags
#define ENUM_DECAL_TYPE_FLAGS(_F, ...) \
    _F(NONE, 0, __VA_ARGS__) \
    _F(PORTAL, 1 << 0, __VA_ARGS__) \

ENUM_MAKE(decal_type_flags, DTF, ENUM_DECAL_TYPE_FLAGS)

// struct side.cos_type
#define ENUM_SIDE_COS_TYPE_INDEX(_F, ...) \
    _F(NONE, 0, __VA_ARGS__)

ENUM_MAKE(side_cos_type_index, SDCT, ENUM_SIDE_COS_TYPE_INDEX)

// struct side.func_type
#define ENUM_SIDE_FUNC_TYPE_INDEX(_F, ...)   \
    _F(NONE, 0, __VA_ARGS__)          \
    _F(DOOR, 1, __VA_ARGS__)

ENUM_MAKE(side_func_type_index, SDFT, ENUM_SIDE_FUNC_TYPE_INDEX)

// struct side.flags
#define ENUM_SIDE_FLAG(_F, ...)          \
    _F(MATOVERRIDE, 1 << 0, __VA_ARGS__) \
    _F(DISCONNECT, 1 << 1, __VA_ARGS__)

ENUM_MAKE(side_flag, SIDE_FLAG, ENUM_SIDE_FLAG)

// struct sector.flags
#define ENUM_SECTOR_FLAGS(_F, ...)       \
    _F(NONE,       0 << 0,  __VA_ARGS__) \
    _F(SKY,        1 << 0,  __VA_ARGS__) \
    _F(FULLBRIGHT, 1 << 1,  __VA_ARGS__) \
    _F(FLASH3,     1 << 28, __VA_ARGS__) \
    _F(FLASH2,     1 << 29, __VA_ARGS__) \
    _F(FLASH1,     1 << 30, __VA_ARGS__) \
    _F(FLASH,      1 << 31, __VA_ARGS__) \

#define SECTOR_FLAG_ANY_FLASH 0xF0000000

ENUM_MAKE(sector_flags, SECTOR_FLAG, ENUM_SECTOR_FLAGS)

// struct sector.cos_type
#define ENUM_SECTOR_COS_TYPE_INDEX(_F, ...)    \
    _F(NONE,              0, __VA_ARGS__) \
    _F(BLINK_RAND_LOFREQ, 1, __VA_ARGS__) \
    _F(BLINK_RAND_MDFREQ, 2, __VA_ARGS__) \
    _F(BLINK_RAND_HIFREQ, 3, __VA_ARGS__) \

ENUM_MAKE(sector_cos_type_index, SCCT, ENUM_SECTOR_COS_TYPE_INDEX)

// struct sector.func_type
#define ENUM_SECTOR_FUNC_TYPE_INDEX(_F, ...) \
    _F(NONE, 0, __VA_ARGS__)          \
    _F(DIFF, 1, __VA_ARGS__)          \
    _F(DOOR, 2, __VA_ARGS__)          \

ENUM_MAKE(sector_func_type_index, SCFT, ENUM_SECTOR_FUNC_TYPE_INDEX)

// item types
#define ENUM_ITEM_TYPE(_F, ...)           \
    _F(PLACEHOLDER, 0, __VA_ARGS__)  \
    _F(FINGER_GUN,  1, __VA_ARGS__)  \

ENUM_MAKE(item_type, ITEM_TYPE, ENUM_ITEM_TYPE)

// particle types
#define ENUM_PARTICLE_TYPE(_F, ...) \
    _F(PLACEHOLDER, 0, __VA_ARGS__) \
    _F(SPLAT, 1, __VA_ARGS__)       \
    _F(FLARE, 2, __VA_ARGS__)       \

ENUM_MAKE(particle_type_index, PARTICLE_TYPE, ENUM_PARTICLE_TYPE)

typedef union ivec2s ivec2s;
typedef union ivec3s ivec3s;
typedef union vec2s vec2s;
typedef union vec3s vec3s;

typedef struct level level_t;
typedef struct editor editor_t;
typedef struct renderer renderer_t;
typedef struct palette palette_t;
typedef struct atlas atlas_t;
typedef struct input input_t;

typedef union lptr lptr_t;
typedef union lptr_nogen lptr_nogen_t;
typedef struct vertex vertex_t;
typedef struct wall wall_t;
typedef struct side side_t;
typedef struct side_segment side_segment_t;
typedef struct sidemat sidemat_t;
typedef struct sector sector_t;
typedef struct subsector subsector_t;
typedef struct sectmat sectmat_t;
typedef struct object object_t;
typedef struct decal decal_t;
typedef int particle_id;
typedef struct particle particle_t;
typedef struct block block_t;
typedef struct actor actor_t;

typedef struct object_type object_type_t;

typedef struct side_texinfo side_texinfo_t;

typedef struct resource resource_t;

typedef struct sector_render sector_render_t;
typedef struct side_render side_render_t;
typedef struct sprite_render sprite_render_t;
typedef struct decal_render decal_render_t;

typedef struct sprite_instance sprite_instance_t;

typedef struct sound_state sound_state_t;
