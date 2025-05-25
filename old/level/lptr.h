#pragma once

#include "defs.h"
#include "util/macros.h"

#define LPTR_NULL ((lptr_t) { .type = 0, .index = 0, .gen = 0 })
#define LPTR_IS_NULL(_lp) (!(_lp).gen)

// true if lptr is T_* (works also for sets of tags)
#define LPTR_IS(_lp, _L) (!!((_lp).type & (_L)))

// internal use only
#define _LPTR_ACCESS(_l, _lp, _L, _n)                                        \
    ({ lptr_t __lp = (_lp); \
        TYPEOF(*(_l)->_n) _p = __lp.type == (_L) ? (_l)->_n[__lp.index] : NULL; \
        _p && _p->gen == __lp.gen ? _p : NULL; })

// internal use only
#define _LPTR_FROM(_p, _L) ({                                                \
        TYPEOF(_p) __p = (_p);                                               \
        __p ?                                                                \
            ((lptr_t) { .type = _L, .index = __p->index, .gen = __p->gen }) \
            : LPTR_NULL;                                                     \
    })

// lptr access functions
#define LPTR_VERTEX(_l, _lp) _LPTR_ACCESS(_l, _lp, T_VERTEX, vertices)
#define LPTR_WALL(_l, _lp) _LPTR_ACCESS(_l, _lp, T_WALL, walls)
#define LPTR_SIDE(_l, _lp) _LPTR_ACCESS(_l, _lp, T_SIDE, sides)
#define LPTR_SIDEMAT(_l, _lp) _LPTR_ACCESS(_l, _lp, T_SIDEMAT, sidemats)
#define LPTR_SECTOR(_l, _lp) _LPTR_ACCESS(_l, _lp, T_SECTOR, sectors)
#define LPTR_SECTMAT(_l, _lp) _LPTR_ACCESS(_l, _lp, T_SECTMAT, sectmats)
#define LPTR_DECAL(_l, _lp) _LPTR_ACCESS(_l, _lp, T_DECAL, decals)
#define LPTR_OBJECT(_l, _lp) _LPTR_ACCESS(_l, _lp, T_OBJECT, objects)

// lptr creation function
#define LPTR_FROM(_p)                                                        \
    _Generic((_p),                                                           \
        const vertex_t *: _LPTR_FROM(_p, T_VERTEX),                     \
        vertex_t *: _LPTR_FROM(_p, T_VERTEX),                           \
        const wall_t *: _LPTR_FROM(_p, T_WALL),                         \
        wall_t *: _LPTR_FROM(_p, T_WALL),                               \
        const side_t *: _LPTR_FROM(_p, T_SIDE),                         \
        side_t *: _LPTR_FROM(_p, T_SIDE),                               \
        const sidemat_t*: _LPTR_FROM(_p, T_SIDEMAT),                    \
        sidemat_t *: _LPTR_FROM(_p, T_SIDEMAT),                         \
        const sector_t *: _LPTR_FROM(_p, T_SECTOR),                     \
        sector_t *: _LPTR_FROM(_p, T_SECTOR),                           \
        const sectmat_t*: _LPTR_FROM(_p, T_SECTMAT),                    \
        sectmat_t *: _LPTR_FROM(_p, T_SECTMAT),                         \
        const decal_t *: _LPTR_FROM(_p, T_DECAL),                       \
        decal_t *: _LPTR_FROM(_p, T_DECAL),                             \
        const object_t *: _LPTR_FROM(_p, T_OBJECT),                     \
        object_t *: _LPTR_FROM(_p, T_OBJECT))

// true if lptrs are equal
#define LPTR_EQ(_lpa, _lpb) ((_lpa).raw == (_lpb).raw)

// lptr -> T_*
#define LPTR_TYPE(_lp) ((_lp).type)

// get a generic field which exists on all level objects (i.e. flags)
// DO NOT USE in real (game) code. this induces T_COUNT branches per access (!)
#define LPTR_FIELD(_l, _p, _name) ({                                         \
        LPTR_IS(_p, T_VERTEX) ?                                              \
            &LPTR_VERTEX(_l, _p)->_name :                                    \
        (LPTR_IS(_p, T_WALL) ?                                               \
            &LPTR_WALL(_l, _p)->_name :                                      \
        (LPTR_IS(_p, T_SIDE) ?                                               \
            &LPTR_SIDE(_l, _p)->_name :                                      \
        (LPTR_IS(_p, T_SIDEMAT) ?                                            \
            &LPTR_SIDEMAT(_l, _p)->_name :                                   \
        (LPTR_IS(_p, T_SECTOR) ?                                             \
            &LPTR_SECTOR(_l, _p)->_name :                                    \
        (LPTR_IS(_p, T_SECTMAT) ?                                            \
            &LPTR_SECTMAT(_l, _p)->_name :                                   \
        (LPTR_IS(_p, T_DECAL) ?                                              \
            &LPTR_DECAL(_l, _p)->_name :                                     \
         (LPTR_IS(_p, T_OBJECT) ?                                            \
            &LPTR_OBJECT(_l, _p)->_name :                                    \
            NULL)))))));                                                     \
    })

// convert lptr -> "TYPE (INDEX)" string
// returns number of characters written, not including null terminator
int lptr_to_str(char *dst, usize n, const level_t *level, lptr_t ptr);

// convert lptr -> "TYPE (INDEX) (extra info...)" string
// returns number of characters written, not including null terminator
int lptr_to_fancy_str(char *dst, usize n, const level_t *level, lptr_t ptr);

// convert lptr to its respective level list index
u16 lptr_to_index(const level_t *level, lptr_t ptr);

// returns true if lptr is to a valid index
bool lptr_is_valid(const level_t *level, lptr_t ptr);

// generate a random (but ID-consistent) palette index based on an lptr
u8 lptr_rand_palette(lptr_t ptr);

// delete underlying object for lptr
void lptr_delete(level_t *level, lptr_t ptr);

// run *_recalculate function for underlying ptr
void lptr_recalculate(level_t *level, lptr_t ptr);

// get raw underlying pointer from lptr
void *lptr_raw_ptr(level_t *level, lptr_t ptr);

// get raw underying integer from lptr
u32 lptr_raw(lptr_t ptr);

// convert raw underyling integer to lptr
lptr_t lptr_from_raw(u32 raw);

// convert lptr to no-gen lptr
lptr_nogen_t lptr_to_nogen(lptr_t ptr);

// convert no-gen lptr -> lptr
lptr_t lptr_from_nogen(level_t *level, lptr_nogen_t ptr);

// raw -> lptr_nogen
lptr_nogen_t lptr_nogen_from_raw(u32 raw);
