#include "level/io.h"
#include "gfx/atlas.h"
#include "level/block.h"
#include "level/decal.h"
#include "level/level.h"
#include "level/lptr.h"
#include "level/object.h"
#include "level/sectmat.h"
#include "level/sector.h"
#include "level/side.h"
#include "level/sidemat.h"
#include "level/tag.h"
#include "level/vertex.h"
#include "level/wall.h"
#include "util/math.h"
#include "util/str.h"
#include <ctype.h>

// #define DO_DEBUG_IO

#ifdef DO_DEBUG_IO
#define DEBUG_IO(...) LOG(__VA_ARGS__)
#else
#define DEBUG_IO(...)
#endif // ifdef DO_DEBUG_IO

#define IO_MAGIC ((int) 0xDEADBEEF)
#define IO_ARRAY_MAGIC ((int) 0x50505050)

typedef struct io_type io_type_t;
typedef struct io io_t;

typedef usize (*read_f)(io_t*, const io_type_t*, const void*, usize, void*);
typedef usize (*write_f)(io_t*, const io_type_t*, FILE*, const void*);

typedef enum {
    IOT_NONE = 0,
    IOT_VERTEX = 1,
    IOT_WALL = 2,
    IOT_SIDE = 3,
    IOT_SIDEMAT = 4,
    IOT_SECTOR = 5,
    IOT_SECTMAT = 6,
    IOT_DECAL = 7,
    IOT_OBJECT = 8,

    IOT_F32,
    IOT_BOOL,
    IOT_U8,
    IOT_U64,
    IOT_INT,
    IOT_IVEC2S,
    IOT_VEC2S,
    IOT_VEC3S,
    IOT_RESOURCE,

    IOT_PTR_VERTEX,
    IOT_PTR_WALL,
    IOT_PTR_SIDE,
    IOT_PTR_SIDEMAT,
    IOT_PTR_SECTOR,
    IOT_PTR_SECTMAT,
    IOT_PTR_DECAL,
    IOT_PTR_OBJECT,

    IOT_PTR_SIDE_TEXINFO,
    IOT_SIDE_TEXINFO,

    IOT_COUNT
} io_type;

typedef struct {
    const char *name;

    int offset;
    io_type type;

    int controlling_offset;
    int controlling_size;
    int controlling_value;
} io_field_t;

typedef struct io_type {
    struct io_type *base_type;
    read_f read;
    write_f write;
    io_field_t fields[32];
} io_type_t;

typedef struct {
    io_type type;
    void *ptr;
} io_ptr_patch_t;

typedef usize (*io_upgrade_f)(
    io_t*, const io_type_t*, const void*, usize, void*, bool*);

typedef struct io_upgrade {
    io_upgrade_f read_hook;
} io_upgrade_t;

typedef struct io {
    const io_upgrade_t *upgrade;
    level_t *level;
    DYNLIST(io_ptr_patch_t) patches;
} io_t;

#define MAX_UPGRADES 256

static const io_upgrade_t UPGRADES[MAX_UPGRADES];

#define IOTYPE_OF(_T) _Generic(*((_T*)(NULL)), \
    f32: IOT_F32,                              \
    bool: IOT_BOOL,                            \
    u8: IOT_U8,                                \
    u64: IOT_U64,                              \
    int: IOT_INT,                              \
    ivec2s: IOT_IVEC2S,                        \
    vec2s: IOT_VEC2S,                          \
    vec3s: IOT_VEC3S,                          \
    resource_t: IOT_RESOURCE,                  \
    vertex_t*: IOT_PTR_VERTEX,                 \
    wall_t*: IOT_PTR_WALL,                     \
    side_t*: IOT_PTR_SIDE,                     \
    sidemat_t*: IOT_PTR_SIDEMAT,               \
    sector_t*: IOT_PTR_SECTOR,                 \
    sectmat_t*: IOT_PTR_SECTMAT,               \
    decal_t*: IOT_PTR_DECAL,                   \
    object_t*: IOT_PTR_OBJECT,                 \
    vertex_t: IOT_VERTEX,                      \
    wall_t: IOT_WALL,                          \
    side_t: IOT_SIDE,                          \
    sidemat_t: IOT_SIDEMAT,                    \
    sector_t: IOT_SECTOR,                      \
    sectmat_t: IOT_SECTMAT,                    \
    decal_t: IOT_DECAL,                        \
    object_t: IOT_OBJECT,                      \
    side_texinfo_t: IOT_SIDE_TEXINFO,          \
    object_type_index: IOT_INT,                \
    decal_type_index: IOT_INT,                 \
    side_func_type_index: IOT_INT,             \
    side_cos_type_index: IOT_INT,              \
    sector_func_type_index: IOT_INT,           \
    sector_cos_type_index: IOT_INT,            \
    plane_type: IOT_INT)

static const io_type_t IO_TYPES[IOT_COUNT];

ALWAYS_INLINE io_type io_type_to_non_ptr(io_type type) {
    switch (type) {
    case IOT_PTR_VERTEX: return IOT_VERTEX;
    case IOT_PTR_WALL: return IOT_WALL;
    case IOT_PTR_SIDE: return IOT_SIDE;
    case IOT_PTR_SIDEMAT: return IOT_SIDEMAT;
    case IOT_PTR_SECTOR: return IOT_SECTOR;
    case IOT_PTR_SECTMAT: return IOT_SECTMAT;
    case IOT_PTR_DECAL: return IOT_DECAL;
    case IOT_PTR_OBJECT: return IOT_OBJECT;
    default: ASSERT(false, "%d", type);
    }
}

#define DECL_RWFUNCS_SIMPLE(_T)                                                               \
    static usize read_##_T(io_t*, const io_type_t *type, const void *src, usize n, _T *dst) { \
        ASSERT(n >= sizeof(_T));                                                              \
        memcpy(dst, src, sizeof(_T));                                                         \
        return sizeof(_T);                                                                    \
    }                                                                                         \
    static usize write_##_T(io_t*, const io_type_t*type, FILE *fp, const _T *src) {           \
        ASSERT(                                                                               \
            fwrite(src, sizeof(*src), 1, fp) == 1);                                           \
        return sizeof(_T);                                                                    \
    }

DECL_RWFUNCS_SIMPLE(f32)
DECL_RWFUNCS_SIMPLE(bool)
DECL_RWFUNCS_SIMPLE(u8)
DECL_RWFUNCS_SIMPLE(u64)
DECL_RWFUNCS_SIMPLE(int)
DECL_RWFUNCS_SIMPLE(ivec2s)
DECL_RWFUNCS_SIMPLE(vec2s)
DECL_RWFUNCS_SIMPLE(vec3s)

#define DECL_RWFUNCS_PTR(_T)                                                                  \
    static usize read_ptr_##_T(io_t *io, const io_type_t *type, const void *src, usize n, _T **dst) { \
        u16 index;                                                                            \
        ASSERT(n >= sizeof(index));                                                           \
        memcpy(&index, src, sizeof(index));                                                   \
        *dst = (void*) (uintptr_t) index;                                                     \
        *dynlist_push(io->patches) =                                                          \
            (io_ptr_patch_t) { io_type_to_non_ptr((io_type) (type - &IO_TYPES[0])), dst };    \
        return sizeof(index);                                                                 \
    }                                                                                         \
    static usize write_ptr_##_T(io_t*, const io_type_t*, FILE *fp, const _T **src) {          \
        const u16 zero = 0;                                                                   \
        ASSERT(                                                                               \
            fwrite(*src ? &(*src)->save_index : &zero, sizeof(u16), 1, fp) == 1);             \
        return sizeof(u16);                                                                   \
    }

DECL_RWFUNCS_PTR(vertex_t)
DECL_RWFUNCS_PTR(wall_t)
DECL_RWFUNCS_PTR(side_t)
DECL_RWFUNCS_PTR(sidemat_t)
DECL_RWFUNCS_PTR(sector_t)
DECL_RWFUNCS_PTR(sectmat_t)
DECL_RWFUNCS_PTR(decal_t)
DECL_RWFUNCS_PTR(object_t)

static usize read_resource(
    io_t*, const io_type_t*, const void *src, usize n, resource_t *dst) {
    const int len = ARRLEN(dst->name);
    ASSERT(n >= len);
    memcpy(dst->name, src, len);
    dst->name[len - 1] = '\0';
    return len;
}

static usize write_resource(
    io_t*, const io_type_t*, FILE *fp, const resource_t *src) {
    ASSERT(fwrite(src->name, ARRLEN(src->name), 1, fp) == 1);
    return ARRLEN(src->name);
}

static usize read_generic_field(
    io_t *io,
    const io_type_t *type,
    const void *src,
    usize n,
    void *dst,
    const io_field_t *field) {
    if (field->controlling_offset != -1) {
        // check control value
        u8 bytes[field->controlling_size], value[field->controlling_size];
        memcpy(
            bytes,
            dst + field->controlling_offset,
            field->controlling_size);
        memcpy(
            value,
            &field->controlling_value,
            min(field->controlling_size, sizeof(field->controlling_value)));

        if (memcmp(bytes, value, field->controlling_size)) {
            // skip reading this field
            return 0;
        }
    }

    DEBUG_IO("  reading field +%d of type %d", field->offset, field->type);
    return
        IO_TYPES[field->type].read(
            io, &IO_TYPES[field->type], src, n, dst + field->offset);
}

static usize read_generic(
    io_t *io, const io_type_t *type, const void *src, usize n, void *dst) {
    if (io->upgrade && io->upgrade->read_hook) {
        DEBUG_IO(
            "generic read of type %d overridden",
            ARR_PTR_INDEX(IO_TYPES, type));

        // check for read hook override
        bool skip = false;
        const usize c = io->upgrade->read_hook(io, type, src, n, dst, &skip);
        if (skip) {
            DEBUG_IO("  skipping!");
            return c;
        }
    }

    usize c = 0;
    const io_field_t *field = &type->fields[0];
    DEBUG_IO("reading generic type %d", (int) (type - &IO_TYPES[0]));
    while (field->type != IOT_NONE) {
        const int res = read_generic_field(io, type, src, n - c, dst, field);
        src += res;
        c += res;
        field++;
    }
    DEBUG_IO("  (read %d bytes in total)", c);
    return c;
}

static usize write_generic(
    io_t *io, const io_type_t *type, FILE *fp, const void *src) {
    usize c = 0;
    const io_field_t *field = &type->fields[0];
    while (field->type != IOT_NONE) {
        if (field->controlling_offset != -1) {
            // check control value
            u8 bytes[field->controlling_size], value[field->controlling_size];
            memcpy(
                bytes,
                src + field->controlling_offset,
                field->controlling_size);
            memcpy(
                value,
                &field->controlling_value,
                min(field->controlling_size, sizeof(field->controlling_value)));

            if (memcmp(bytes, value, field->controlling_size)) {
                // skip field with non-matching control
                goto next_field;
            }
        }

        const usize res =
            IO_TYPES[field->type].write(
                io, &IO_TYPES[field->type], fp, src + field->offset);
        c += res;

next_field:
        field++;
    }
    return c;
}

#define MKFIELD(_T, _f)                          \
    ((io_field_t) {                              \
        .offset = offsetof(_T, _f),              \
        .type = IOTYPE_OF(TYPEOF_FIELD(_T, _f)), \
        .name = #_f,                            \
        .controlling_offset = -1,                \
        .controlling_size = -1,                  \
        .controlling_value = -1,                 \
    })

#define MKFIELD_CONTROLLED(_T, _f, _c, _v)       \
    ((io_field_t) {                              \
        .offset = offsetof(_T, _f),              \
        .type = IOTYPE_OF(TYPEOF_FIELD(_T, _f)), \
        .name = #_f,                            \
        .controlling_offset = offsetof(_T, _c),  \
        .controlling_size = sizeof(((_T*)(NULL))->_c),  \
        .controlling_value = (_v),               \
    })

#define MKGENERIC_NOPTR(_T, _l, ...)      \
    [IOT_##_T] = {                        \
        .read = (read_f) read_generic,    \
        .write = (write_f) write_generic, \
        .fields =                         \
            __VA_ARGS__                   \
                                          \
    }                                     \

#define MKGENERIC(_T, _l, ...)            \
    [IOT_PTR_##_T] = {                    \
        .read = (read_f) read_ptr_##_l,   \
        .write = (write_f) write_ptr_##_l,\
    },                                    \
    MKGENERIC_NOPTR(_T, _l, __VA_ARGS__)

static const io_type_t IO_TYPES[IOT_COUNT] = {
    [IOT_F32] = { .read = (read_f) read_f32, .write = (write_f) write_f32 },
    [IOT_BOOL] = { .read = (read_f) read_bool, .write = (write_f) write_bool },
    [IOT_U64] = { .read = (read_f) read_u64, .write = (write_f) write_u64 },
    [IOT_U8] = { .read = (read_f) read_u8, .write = (write_f) write_u8 },
    [IOT_INT] = { .read = (read_f) read_int, .write = (write_f) write_int },
    [IOT_IVEC2S] = { .read = (read_f) read_ivec2s, .write = (write_f) write_ivec2s },
    [IOT_VEC2S] = { .read = (read_f) read_vec2s, .write = (write_f) write_vec2s },
    [IOT_VEC3S] = { .read = (read_f) read_vec3s, .write = (write_f) write_vec3s },
    [IOT_RESOURCE] = { .read = (read_f) read_resource, .write = (write_f) write_resource },

    MKGENERIC_NOPTR(SIDE_TEXINFO, side_texinfo_t, {
        MKFIELD(side_texinfo_t, split_bottom),
        MKFIELD(side_texinfo_t, split_top),
        MKFIELD(side_texinfo_t, offsets),
        MKFIELD(side_texinfo_t, flags),
    }),

    MKGENERIC(VERTEX, vertex_t, {
        MKFIELD(vertex_t, pos)
    }),
    MKGENERIC(WALL, wall_t, {
        MKFIELD(wall_t, vertices[0]),
        MKFIELD(wall_t, vertices[1]),
        MKFIELD(wall_t, sides[0]),
        MKFIELD(wall_t, sides[1]),
    }),
    MKGENERIC(SIDE, side_t, {
        MKFIELD(side_t, sector),
        MKFIELD(side_t, portal),
        MKFIELD(side_t, mat),
        MKFIELD(side_t, cos_type),
        MKFIELD(side_t, func_type),
        MKFIELD(side_t, tag),
        MKFIELD(side_t, flags),
        MKFIELD(side_t, tex_info),
        MKFIELD(side_t, funcdata.raw),
    }),
    MKGENERIC(SIDEMAT, sidemat_t, {
        MKFIELD(sidemat_t, name),
        MKFIELD(sidemat_t, tex_low),
        MKFIELD(sidemat_t, tex_mid),
        MKFIELD(sidemat_t, tex_high),
        MKFIELD(sidemat_t, tex_info),
    }),
    MKGENERIC(SECTOR, sector_t, {
        MKFIELD(sector_t, floor.z),
        MKFIELD(sector_t, floor.offsets),
        MKFIELD(sector_t, ceil.z),
        MKFIELD(sector_t, ceil.offsets),
        MKFIELD(sector_t, base_light),
        MKFIELD(sector_t, cos_type),
        MKFIELD(sector_t, func_type),
        MKFIELD(sector_t, flags),
        MKFIELD(sector_t, mat),
        MKFIELD(sector_t, tag),
        /* MKFIELD(sector_t, funcdata.raw), */
    }),
    MKGENERIC(SECTMAT, sectmat_t, {
        MKFIELD(sectmat_t, name),
        MKFIELD(sectmat_t, floor.tex),
        MKFIELD(sectmat_t, floor.offset),
        MKFIELD(sectmat_t, ceil.tex),
        MKFIELD(sectmat_t, ceil.offset),
    }),
    MKGENERIC(DECAL, decal_t, {
        MKFIELD(decal_t, type),
        MKFIELD(decal_t, is_on_side),
        MKFIELD_CONTROLLED(decal_t, side.ptr, is_on_side, true),
        MKFIELD_CONTROLLED(decal_t, side.offsets, is_on_side, true),
        MKFIELD_CONTROLLED(decal_t, sector.ptr, is_on_side, false),
        MKFIELD_CONTROLLED(decal_t, sector.pos, is_on_side, false),
        MKFIELD_CONTROLLED(decal_t, sector.plane, is_on_side, false),
        MKFIELD(decal_t, tex_base),
        MKFIELD(decal_t, tex_offsets),
        MKFIELD(decal_t, tag),
        MKFIELD(decal_t, funcdata.raw),
    }),
    MKGENERIC(OBJECT, object_t, {
        MKFIELD(object_t, type_index),
        MKFIELD(object_t, pos),
        MKFIELD(object_t, z),
        MKFIELD(object_t, vel_xyz),
        MKFIELD(object_t, angle),
        MKFIELD(object_t, funcdata.raw),
    }),
};

static int io_type_to_level_type(io_type type) {
    switch (type) {
    case IOT_VERTEX: return T_VERTEX;
    case IOT_WALL: return T_WALL;
    case IOT_SIDE: return T_SIDE;
    case IOT_SIDEMAT: return T_SIDEMAT;
    case IOT_SECTOR: return T_SECTOR;
    case IOT_SECTMAT: return T_SECTMAT;
    case IOT_OBJECT: return T_OBJECT;
    case IOT_DECAL: return T_DECAL;
    default: ASSERT(false, "%d", type);
    }
}

static void *array_type_nth(io_t *io, io_type type, int n) {
    switch (type) {
    case IOT_VERTEX: return io->level->vertices[n];
    case IOT_WALL: return io->level->walls[n];
    case IOT_SIDE: return io->level->sides[n];
    case IOT_SIDEMAT: return io->level->sidemats[n];
    case IOT_SECTOR: return io->level->sectors[n];
    case IOT_SECTMAT: return io->level->sectmats[n];
    case IOT_OBJECT: return io->level->objects[n];
    case IOT_DECAL: return io->level->decals[n];
    default: ASSERT(false, "%d", type);
    }

    return NULL;
}

static int array_type_count(io_t *io, io_type type) {
    return level_get_list_count(io->level, io_type_to_level_type(type));
}

// RAW count (including empty indices)
static int array_type_raw_count(io_t *io, io_type type) {
    switch (type) {
    case IOT_VERTEX: return dynlist_size(io->level->vertices);
    case IOT_WALL: return dynlist_size(io->level->walls);
    case IOT_SIDE: return dynlist_size(io->level->sides);
    case IOT_SIDEMAT: return dynlist_size(io->level->sidemats);
    case IOT_SECTOR: return dynlist_size(io->level->sectors);
    case IOT_SECTMAT: return dynlist_size(io->level->sectmats);
    case IOT_OBJECT: return dynlist_size(io->level->objects);
    case IOT_DECAL: return dynlist_size(io->level->decals);
    default: ASSERT(false, "%d", type);
    }

    return 0;
}

static lptr_t array_type_new(io_t *io, io_type type) {
    switch (type) {
    case IOT_VERTEX: return LPTR_FROM(level_alloc(io->level, io->level->vertices));
    case IOT_WALL: return LPTR_FROM(level_alloc(io->level, io->level->walls));
    case IOT_SIDE: return LPTR_FROM(level_alloc(io->level, io->level->sides));
    case IOT_SIDEMAT: return LPTR_FROM(level_alloc(io->level, io->level->sidemats));
    case IOT_SECTOR: return LPTR_FROM(level_alloc(io->level, io->level->sectors));
    case IOT_SECTMAT: return LPTR_FROM(level_alloc(io->level, io->level->sectmats));
    case IOT_OBJECT: return LPTR_FROM(level_alloc(io->level, io->level->objects));
    case IOT_DECAL: return LPTR_FROM(level_alloc(io->level, io->level->decals));
    default: ASSERT(false, "%d", type);
    }
    return LPTR_NULL;
}

static int array_type_read(io_t *io, const u8 **pp, const u8 *end) {
    DEBUG_IO("reading array type...");

    int magic;
    *pp += read_int(io, &IO_TYPES[IOT_INT], *pp, end - *pp, &magic);
    DEBUG_IO("  magic is 0x%08x", magic);

    if (magic != IO_ARRAY_MAGIC) {
        return IO_BAD_ARRAY_MAGIC;
    }

    io_type type;
    *pp += read_int(io, &IO_TYPES[IOT_INT], *pp, end - *pp, (int*) &type);
    DEBUG_IO("  type is %d", type);

    int count;
    *pp += read_int(io, &IO_TYPES[IOT_INT], *pp, end - *pp, &count);
    DEBUG_IO("  count is %d", count);

    for (int i = 0; i < count; i++) {
        lptr_t dst = array_type_new(io, type);
        *LPTR_FIELD(io->level, dst, level_flags) |= LF_DO_NOT_RECALC;
        *pp +=
            IO_TYPES[type].read(
                io,
                &IO_TYPES[type],
                *pp,
                end - *pp,
                lptr_raw_ptr(io->level, dst));
    }

    return IO_OK;
}

static int array_type_write(io_t *io, FILE *fp, io_type type) {
    const int magic = IO_ARRAY_MAGIC;
    write_int(io, &IO_TYPES[IOT_INT], fp, &magic);

    int
        count = array_type_count(io, type),
        raw_count = array_type_raw_count(io, type);

    // first element is not written for these types with NOMAT
    switch (type) {
    case IOT_SECTMAT:
    case IOT_SIDEMAT:
        count -= 1;
        break;
    default:
    }

    write_int(io, &IO_TYPES[IOT_INT], fp, (int*) &type);
    write_int(io, &IO_TYPES[IOT_INT], fp, &count);

    // never write first NULL or NOMAT element
    int n = 0;
    for (int i = 1; i < raw_count; i++) {
        void *ptr = array_type_nth(io, type, i);
        if (!ptr) { continue; }

        DEBUG_IO("  writing #%d (index %d)...", n, i);
        IO_TYPES[type].write(
            io,
            &IO_TYPES[type],
            fp,
            ptr);

        n++;
    }

    ASSERT(n == count);
    return IO_OK;
}

int io_load_level(level_t *level, const u8 *src, usize n) {
    io_t io = { .level = level };
    const u8 *p = src, *end = src + n;

    int magic;
    p += read_int(&io, &IO_TYPES[IOT_INT], p, n, &magic);

    if (magic != IO_MAGIC) {
        return IO_BAD_START_MAGIC;
    }

    // check for version
    int version;
    p += read_int(&io, &IO_TYPES[IOT_INT], p, n, &version);

    if (version == IO_ARRAY_MAGIC) {
        // un-read array magic
        p -= 4;
        io.upgrade = &UPGRADES[0];
        DEBUG_IO("performing initial upgrade 0 -> 1");
    } else if (version < IO_VERSION) {
        // TODO: skipping versions
        io.upgrade = &UPGRADES[version];
        DEBUG_IO("!!! upgrade %d -> %d !!!", version, IO_VERSION);
    } else if (version > IO_VERSION) {
        return IO_UNKNOWN_VERSION;
    } else {
        io.upgrade = NULL;
    }

    // TODO: loop while we keep finding array type magic?
    array_type_read(&io, &p, end); // vertex
    array_type_read(&io, &p, end); // sector
    array_type_read(&io, &p, end); // sectmat
    array_type_read(&io, &p, end); // wall
    array_type_read(&io, &p, end); // side
    array_type_read(&io, &p, end); // sidemat
    array_type_read(&io, &p, end); // decal
    array_type_read(&io, &p, end); // object

    DEBUG_IO("DONE READING ARRAYS:");
    for (io_type type = IOT_VERTEX; type <= IOT_OBJECT; type++) {
        DEBUG_IO("  type %d: %d elements", type, array_type_count(&io, type));
    }

    p += read_int(&io, &IO_TYPES[IOT_INT], src, n, &magic);
    if (magic != IO_MAGIC) {
        return IO_BAD_END_MAGIC;
    }

    if (p != end) {
        return IO_EXTRA_DATA;
    }

    DEBUG_IO("patching %d indices...", dynlist_size(io.patches));
    dynlist_each(io.patches, it) {
        const u16 index = (u16) (uintptr_t) *((void**) (it.el->ptr));

        // SECTMAT and SIDEMAT 0 map to NOMAT ([0]), otherwise 0 indices
        // map to NULL pointers
        void *p = NULL;
        if (index != 0
            || it.el->type == IOT_SECTMAT
            || it.el->type == IOT_SIDEMAT) {
             p = array_type_nth(&io, it.el->type, index);
            if (!p) {
                DEBUG_IO(
                    "  %p (%d) (type %d) -> %p (BAD)",
                    it.el->ptr, index, it.el->type, p);
                return IO_BAD_INDEX;
            }
        }

        DEBUG_IO("  %p (%d) (type %d) -> %p", it.el->ptr, index, it.el->type, p);
        *((void**) (it.el->ptr)) = p;
    }
    dynlist_free(io.patches);

#define ALLOW_RECALC(_t, _list, ...)                \
    level_dynlist_each(_list, it) {                 \
        (*it.el)->level_flags &= ~LF_DO_NOT_RECALC; \
    }

    // allow vertices, walls to recalc
    LEVEL_FOR_LISTS(ALLOW_RECALC, level, T_VERTEX | T_WALL);

    // load wall vertices, sides
    level_dynlist_each(level->walls, it) {
        wall_t *wall = *it.el;

        for (int i = 0; i < 2; i++) {
            vertex_t *v = wall->vertices[i];
            wall->vertices[i] = NULL;
            wall_set_vertex(level, wall, i, v);
        }

        for (int i = 0; i < 2; i++) {
            side_t *s = wall->sides[i];
            wall->sides[i] = NULL;
            wall_set_side(level, wall, i, s);
        }
    }

    // do not recalc for sectors, sides while loading

    // load sector sides
    level_dynlist_each(level->sides, it) {
        side_t *side = *it.el;

        if (!side->sector) {
            WARN("loading with sect-less side %d", it.i);
            continue;
        }

        sector_t *s = side->sector;
        side->sector = NULL;
        sector_add_side(level, s, side);
    }

    // allow sides to be updated
    level_dynlist_each(level->sides, it) {
        (*it.el)->level_flags &= ~LF_DO_NOT_RECALC;
        side_recalculate(level, *it.el);
    }

    // recalculate sectors
    level_dynlist_each(level->sectors, it) {
        (*it.el)->level_flags &= ~LF_DO_NOT_RECALC;
        sector_recalculate(level, *it.el);
    }

    // allow materials, decals, objects, to recalc
    LEVEL_FOR_LISTS(
        ALLOW_RECALC, level, T_SIDEMAT | T_SECTMAT | T_DECAL | T_OBJECT)

#undef ALLOW_RECALC

    // load decal sides and sectors
    level_dynlist_each(level->decals, it) {
        decal_t *decal = *it.el;

        if (decal->is_on_side) {
            side_t *side = decal->side.ptr;
            decal->side.ptr = NULL;
            decal_set_side(level, decal, side);
        } else {
            sector_t *sector = decal->sector.ptr;
            decal->sector.ptr = NULL;
            decal_set_sector(level, decal, sector, decal->sector.plane);
        }
    }

    // load object types, sectors
    level_dynlist_each(level->objects, it) {
        object_t *object = *it.el;
        object_type_index type = object->type_index;
        object->type_index = OT_PLACEHOLDER;
        object_set_type(level, object, type);

        ASSERT(!object->sector);
        const vec2s pos = object->pos;
        object->pos = VEC2(0);
        object_move(level, object, pos);
    }

    // add to tag lists
#define ADD_TAGS(_t, _list, ...)                                 \
        if (_t & T_HAS_TAG) {                                    \
            level_dynlist_each(_list, it) {                      \
                int *ptag = lptr_ptag(level, LPTR_FROM(*it.el)); \
                const int tag = *ptag;                           \
                *ptag = TAG_NONE;                                \
                tag_set(level, LPTR_FROM(*it.el), tag);          \
            }                                                    \
        }

    LEVEL_FOR_ALL_LISTS(ADD_TAGS, level)

#undef ADD_TAGS

    // compute sector visibility
    level_dynlist_each(level->sectors, it) {
        sector_compute_visibility(level, *it.el);
    }

    level_reset_blocks(level);

    // load object sectors
    // TODO: twice??
    level_dynlist_each(level->objects, it) {
        object_t *object = *it.el;
        const vec2s pos = object->pos;
        object->pos = VEC2(0);
        object_move(level, object, pos);
    }

    return IO_OK;
}

int io_save_level(FILE *fp, level_t *level) {
    // TODO: find an alternative to this
    // assign save_index
#define ASSIGN_SAVEINDEX(_t, _list, ...) do {               \
            int i = (_t & (T_SIDEMAT | T_SECTMAT)) ? 0 : 1; \
            level_dynlist_each(_list, it) {                 \
                (*it.el)->save_index = i;                   \
                i++;                                        \
            }                                               \
        } while(0);

    LEVEL_FOR_ALL_LISTS(ASSIGN_SAVEINDEX, level)

#undef ASSIGN_SAVEINDEX

    io_t io = { .level = level };
    const int magic = IO_MAGIC;
    write_int(&io, &IO_TYPES[IOT_INT], fp, &magic);

    const int version = IO_VERSION;
    write_int(&io, &IO_TYPES[IOT_INT], fp, &version);

    array_type_write(&io, fp, IOT_VERTEX);  // vertex
    array_type_write(&io, fp, IOT_SECTOR);  // sector
    array_type_write(&io, fp, IOT_SECTMAT); // sectmat
    array_type_write(&io, fp, IOT_WALL);    // wall
    array_type_write(&io, fp, IOT_SIDE);    // side
    array_type_write(&io, fp, IOT_SIDEMAT); // sidemat
    array_type_write(&io, fp, IOT_DECAL);   // decal
    array_type_write(&io, fp, IOT_OBJECT);  // object

    write_int(&io, &IO_TYPES[IOT_INT], fp, &magic);
    return IO_OK;
}

// TODO
usize upgrade_0_1_read_hook(
    io_t *io,
    const io_type_t *type,
    const void *src,
    usize n,
    void *dst,
    bool *skip) {
    const int type_index = ARR_PTR_INDEX(IO_TYPES, type);

    // upgrade "flags" from u8 -> int on side, sector
    if (type_index == IOT_SIDE || type_index == IOT_SECTOR) {
        const io_field_t *field = &type->fields[0];
        int c = 0;
        while (field->type != IOT_NONE) {
            if (!strcmp(field->name, "flags")) {
                // skip 1 byte read
                src += 1;
                c += 1;
                goto next_field;
            }

            const int res =
                read_generic_field(io, type, src, n - c, dst, field);
            src += res;
            c += res;

        next_field:
            field++;
        }

        *skip = true;
        return c;
    }

    *skip = false;
    return 0;
}

static const io_upgrade_t UPGRADES[MAX_UPGRADES] = {
    [0] = {
        .read_hook = upgrade_0_1_read_hook
    }
};
