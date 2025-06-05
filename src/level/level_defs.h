#pragma once

#include "util/bitmap.h"
#include "util/resource.h"
#include "util/types.h"
#include "util/math.h"
#include "util/enum.h"
#include "util/dynlist.h"
#include "util/dlist.h"
#include "util/llist.h"
#include "config.h"
#include "defs.h"

typedef union lptr lptr_t;
typedef union lptr_nogen lptr_nogen_t;
typedef struct vertex vertex_t;
typedef struct wall wall_t;
typedef struct side side_t;
typedef struct sidemat sidemat_t;
typedef struct sector sector_t;
typedef struct sectmat sectmat_t;
typedef struct object object_t;
typedef struct decal decal_t;
typedef struct block block_t;

// TODO: below breaks on big endian, standardize "raw" formats

// "tagged pointer" (really a typed index) for anything in level arrays
// only interact by from LPTR_* functions
typedef union lptr {
    struct {
        u16 index;
        u8 type;
        u8 gen;
    };
    u32 raw;
} lptr_t;

// lptr with no generation (used for save/load)
typedef union lptr_nogen {
    struct {
        u16 index;
        u8 type;
        u8 _padding;
    };
    u32 raw;
} lptr_nogen_t;

// fields which all level must_t have
#define LEVEL_DECL_STRUCT_FIELDS()  \
    int version;                    \
    u8  level_flags;                \
    u8  gen;                        \
    u16 index;                      \
    u16 save_index;                 \

typedef struct vertex {
    // READ VALUES
    vec2s pos;

#ifdef MAPEDITOR
    DYNLIST(wall_t*) walls;
#endif // ifdef MAPEDITOR

    LEVEL_DECL_STRUCT_FIELDS()
} vertex_t;

typedef struct wall {
    // READ VALUES

    // wall vertices, "front" is left of v0 -> v1
    // cannot be NULL
    union {
        struct { vertex_t *v0, *v1; };
        vertex_t *vertices[2];
    };

    // sides, NULL if no side
    union {
        struct { side_t *side0, *side1; };
        side_t *sides[2];
    };

    // COMPUTED VALUES
    vec2s normal;          // normal (right of v0 -> v1)
    vec2s d;               // v1 - v0
    f32 len;            // length(d)
    vec2s last_pos[2];

    LEVEL_DECL_STRUCT_FIELDS()
} wall_t;

// TODO: doc
typedef struct side_texinfo {
    f32 split_bottom, split_top;
    ivec2s offsets;
    u8 flags;
} side_texinfo_t;

// one side of a wall
typedef struct side {
    // READ VALUES
    side_t *portal;                  // side to which this side is portal
    sidemat_t *mat;                  // side material
    side_cos_type_index cos_type;    // cosmetic type
    side_func_type_index func_type;  // functional type

    int tag;                         // function tag

    int flags;                       // see SIDE_FLAG_*
    side_texinfo_t tex_info;         // sidetex overrides

#define SIDE_FUNCDATA_MAXSIZE 8
    union {
        u64 raw;
    } funcdata;

    // COMPUTED VALUES
    LLIST_NODE(side_t) sector_sides; // sector side linked list
    wall_t *wall;                    // wall to which this side belongs
    sector_t *sector;                // sector which this side belongs to
    actor_t *actor;
    int n_decals;                    // number of decals on side
    LLIST(decal_t) decals;           // list of all decals on side

                                     // render info, see renderer.c
    side_render_t *render;

    LEVEL_DECL_STRUCT_FIELDS()
} side_t;

STATIC_ASSERT(
    sizeof(((side_t*) NULL)->funcdata) <= SIDE_FUNCDATA_MAXSIZE,
    "side funcdata is too large!");

// one segment (bottom, top, middle, wall) of a side
typedef struct side_segment {
    bool present : 1;
    bool portal  : 1;
    bool mesh    : 1;
    int index;
    f32 z0, z1;
    f32 z0_v0; // bottom edge (wall->v0 side)
    f32 z0_v1; // bottom edge (wall->v1 side)
    f32 z1_v0; // top edge (first vertex)
    f32 z1_v1; // top edge (second vertex)
} side_segment_t;

#define SIDEMAT_NOMAT 0

// material of side, can be shared across multiple sides
typedef struct sidemat {
    // READ VALUES

    // name of this side material
    resource_t name;

    // low/middle/high textures
    union {
        struct {
            resource_t tex_low, tex_mid, tex_high;
        };

        resource_t texs[3];
    };

    // sidetex information (can be overriden by individual sides)
    side_texinfo_t tex_info;

    LEVEL_DECL_STRUCT_FIELDS()
} sidemat_t;



// floor or ceiling plane, belongs to sector
typedef struct plane {
    // READ VALUES
    f32 z;
    ivec2s offsets;

    vec3s normal;
    f32 d_constant;

    // user-editable slope parameters
    bool slope_enabled;
    f32 slope_angle_deg;
    f32 slope_direction_deg;
} plane_t;

// material of plane
typedef struct planemat {
    resource_t tex;
    ivec2s offset;
} planemat_t;

// where is slope for plane
typedef enum {
    PLANE_TYPE_SLOPE_FLOOR,
    PLANE_TYPE_SLOPE_CEIL
} plane_slope_type_t;

typedef struct sect_tri {
    union {
        struct { vertex_t *a, *b, *c; };
        vertex_t *vs[3];
    };
} sect_tri_t;

typedef struct sect_line {
    union {
        struct { vertex_t *a, *b; };
        vertex_t *vs[2];
    };

    // arbitrary value used in sector combining
    bool tag : 1;
} sect_line_t;

typedef struct subsector_neighbor {
    // id of neighbor
    int id;

    // the adjacent line
    sect_line_t *line;
} subsector_neighbor_t;

typedef struct subsector {
    sector_t *parent;
    int id;
    DYNLIST(sect_line_t) lines;
    DYNLIST(subsector_neighbor_t) neighbors;
    vec2s min, max;
    bool tag : 1;

    // sector this sector was accessed from, used only when computing visibility
    subsector_t *from;
} subsector_t;

// one "area" of the level as defined by a collection of sides
typedef struct sector {
    // READ VALUES
    LLIST(side_t) sides;        // NULL if sector has no sides

    // floor/ceiling planes
    // NOTE: indices must match with plane_type
    union {
        struct { plane_t floor, ceil; };
        plane_t planes[2];
    };

    u8 base_light;                    // sector light value
    sector_cos_type_index cos_type;   // cosmetic type
    sector_func_type_index func_type; // functional type
    int flags;                         // see SECTOR_FLAG_*
    sectmat_t *mat;                   // sector material
    int tag;                          // function tag, TAG_NONE if none

#define SECTOR_FUNCDATA_MAXSIZE 16
    union {
        struct { f32 diff; int ticks; plane_type plane; } diff;
        u8 raw[16];
    } funcdata;

    // COMPUTED VALUES
    int n_sides;                // number of sides in sector
    vec2s min, max;             // minimum/maximum bounds of this sector
    DLIST(object_t) objects;    // objects in sector
    int n_decals;               // number of decals on sector
    LLIST(decal_t) decals;      // decals on sector
    actor_t *actor;             // actor related to this sector, if any

    DYNLIST(particle_id) particles;
    DYNLIST(sect_tri_t) tris;
    DYNLIST(subsector_t) subs;

    // direct neighbors
    // NOTE: does not include neighbors via disconnected portals
    DYNLIST(struct sector*) neighbors;

    // see renderer.c
    sector_render_t *render;

    LEVEL_DECL_STRUCT_FIELDS()
} sector_t;

STATIC_ASSERT(
    sizeof(((sector_t*) NULL)->funcdata) <= SECTOR_FUNCDATA_MAXSIZE,
    "sector funcdata is too large!");

#define SECTMAT_NOMAT 0

// material of sector, can be shared across multiple sectors
typedef struct sectmat {
    // name of this sector material
    resource_t name;

    union {
        planemat_t mats[2];
        struct { planemat_t floor, ceil; };
    };

    LEVEL_DECL_STRUCT_FIELDS()
} sectmat_t;

typedef struct decal {
    // READ VALUES
    decal_type_index type;

    bool is_on_side;

    union {
        struct {
            side_t *ptr;
            vec2s offsets;
        } side;

        struct {
            sector_t *ptr;
            vec2s pos;
            plane_type plane;
        } sector;
    };

    resource_t tex_base;          // base texture (does not change)
    ivec2s tex_offsets;           // texture offsets
    int tag;                      // function tag, TAG_NONE if none

#define DECAL_FUNCDATA_MAXSIZE 8
    union {
        struct { lptr_nogen_t index; } portal;
        u64 raw;
    } funcdata;

    // ticks to live, -1 if permanent
    int ticks;

    // COMPUTED VALUES
    LLIST_NODE(decal_t) node;      // linked list of decals on side or sector
    resource_t tex;             // current render texture

    // see renderer.c
    decal_render_t *render;

    LEVEL_DECL_STRUCT_FIELDS()
} decal_t;

STATIC_ASSERT(
    sizeof(((decal_t*) NULL)->funcdata) <= DECAL_FUNCDATA_MAXSIZE,
    "sector funcdata is too large!");

// see object.h
typedef struct object {
    // READ VALUES
    object_type_index type_index;
    vec2s pos;
    f32 z;
    union { struct { vec2s vel; f32 vel_z; }; vec3s vel_xyz; };
    f32 angle;

    union {
        struct { item_type item; } pickup;
        u64 raw;
    } funcdata;

    // (ex)tra runtime data, by object type
    union {
        /* object_boy *boy; */
        /* object_player *player; */
        /* object_projectile *projectile; */
        void *ex;
    };

    // OS_*
    int state;

    // OF_*
    int flags;

    // COMPUTED VALUES
    object_type_t *type;
    sector_t *sector;
    DLIST_NODE(object_t) sector_list;
    block_t *block;
    DLIST_NODE(object_t) block_list;
    actor_t *actor;

    sprite_render_t *render;

    LEVEL_DECL_STRUCT_FIELDS()
} object_t;

typedef struct particle {
    int id;
    particle_type_index type;

    int ticks;
    sector_t *sector;
    DLIST_NODE(struct particle) node;

    union {
        struct {
            vec2s pos;
            f32 z;
        };

        vec3s pos_xyz;
    };

    union {
        struct {
            vec2s vel;
            f32 vel_z;
        };

        vec3s vel_xyz;
    };

    union {
        struct {
            vec4s color;
        } flare;
    };
} particle_t;

typedef struct particle_type {
    resource_t tex;
    vec3s gravity;
    vec3s air_drag;
    vec3s floor_drag;
    vec3s restitution;
} particle_type_t;

extern particle_type_t PARTICLE_TYPES[PARTICLE_TYPE_COUNT];

// one map "block" containing:
// * walls which collide with it
// * sectors which collide with it
// * subsectors which collide with it
// * objects inside of it
typedef struct block {
    DYNLIST(sector_t*) sectors;
    DYNLIST(wall_t*) walls;
    DYNLIST(int) subsectors;
    DLIST(object_t) objects;
} block_t;

typedef struct level {
    // arbitrary "version" number, bumped whenever anything is recalc'd
    int version;

    // TODO: doc
    DYNLIST(lptr_t) dirty_sides, dirty_vis_sectors;

    DYNLIST(object_t*) destroy_objects;

    DYNLIST(vertex_t*) vertices;
    DYNLIST(wall_t*) walls;
    DYNLIST(side_t*) sides;
    DYNLIST(sidemat_t*) sidemats;
    DYNLIST(sector_t*) sectors;
    DYNLIST(sectmat_t*) sectmats;
    DYNLIST(decal_t*) decals;
    DYNLIST(object_t*) objects;

    // all actors in level
    DLIST(actor_t) actors;

    // TODO: doc for subsectors
    BITMAP *subsector_ids;
    DYNLIST(subsector_t*) subsectors;

    // TODO: doc for particles
    BITMAP *particle_ids;
    DYNLIST(particle_t) particles;

    // TODO: tags should probably be a hash map?

    // table of tag values
    int tag_values[TAG_MAX];

    // lptr tag lists
    DYNLIST(lptr_t) tag_lists[TAG_MAX];

    // n x n visibility matrix bitmap
    // where n is number of sectors rounded up to 64
    struct {
        int n;

        // BITMAP *matrix[n];
        u8 *matrix;
    } visibility;

    // level bounds, also define bounds for "blocks" array
    struct {
        ivec2s min, max;
    } bounds;

    // 2D array of collision blocks
    struct {
        block_t *arr;
        ivec2s offset, size;
    } blocks;
} level_t;

// actor flags
enum {
    AF_NONE         = 0,
    AF_PARAM_OWNED  = 1 << 0,
    AF_DONE         = 1 << 1
};

// actor "act" function
typedef void (*actor_f)(level_t*, actor_t*, void*);

// actor, see l_add_actor
// act is called at level tick rate until flags & AF_DONE
typedef struct actor {
    DLIST_NODE(struct actor) listnode;
    actor_t **storage; // pointer to where actor is stored on its acted-on
                       // level element. fx. sector_t::actor
    void *param;
    u8 flags;
    actor_f act;
} actor_t;

typedef void (*object_update_f)(level_t*, object_t*);
typedef resource_t (*object_sprite_f)(
    const level_t*,
    const object_t*);

// type information for each OT_*
typedef struct object_type {
    resource_t sprite;

    f32 radius;
    f32 height;

    actor_f act_fn;

    // (VERY OPTIONAL!) update function, run once/frame
    // intended only for use with things which require player input
    object_update_f update_fn;

    // retreives the sprite for this object, if NULL "sprite" is always used
    object_sprite_f sprite_fn;

    // OTF*
    int flags;

    // size of allocated memory for object.ex
    int ex_size;

    // initial value for object.ex
    void *ex_init;
} object_type_t;

// see object.c
extern object_type_t OBJECT_TYPES[OT_COUNT];

// type information for DT_*
typedef struct decal_type {
    // DTF_*
    int flags;

    void (*interact)(
        level_t*,
        decal_t*,
        object_t*);
} decal_type_t;

// see l_decal.c
extern decal_type_t DECAL_TYPES[DT_COUNT];

// type information for SFT_*
typedef struct sector_func_type {
    void (*tag_change)(
        level_t*,
        sector_t*);
} sector_func_type_t;

// see l_sector.c
extern sector_func_type_t SECTOR_FUNC_TYPE[SCFT_COUNT];
