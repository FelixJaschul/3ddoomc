#pragma once

#include "util/bitmap.h"
#include "util/dynlist.h"
#include "level/level_defs.h"

#include "defs.h"

// apply _F to all level type dynlists in _l
#define LEVEL_FOR_ALL_LISTS(_F, _l, ...)       \
    _F(T_SECTMAT, (_l)->sectmats, __VA_ARGS__) \
    _F(T_SIDEMAT, (_l)->sidemats, __VA_ARGS__) \
    _F(T_SECTOR,  (_l)->sectors,  __VA_ARGS__) \
    _F(T_VERTEX,  (_l)->vertices, __VA_ARGS__) \
    _F(T_WALL,    (_l)->walls,    __VA_ARGS__) \
    _F(T_SIDE,    (_l)->sides,    __VA_ARGS__) \
    _F(T_DECAL,   (_l)->decals,   __VA_ARGS__) \
    _F(T_OBJECT,  (_l)->objects,  __VA_ARGS__)

// apply _F to level type dynlists in _T in _l
#define LEVEL_FOR_LISTS(_F, _l, _T, ...)                                 \
    if ((_T) & T_SECTMAT) { _F(T_SECTMAT, (_l)->sectmats, __VA_ARGS__) } \
    if ((_T) & T_SIDEMAT) { _F(T_SIDEMAT, (_l)->sidemats, __VA_ARGS__) } \
    if ((_T) & T_SECTOR)  { _F(T_SECTOR,  (_l)->sectors,  __VA_ARGS__) } \
    if ((_T) & T_VERTEX)  { _F(T_VERTEX,  (_l)->vertices, __VA_ARGS__) } \
    if ((_T) & T_WALL)    { _F(T_WALL,    (_l)->walls,    __VA_ARGS__) } \
    if ((_T) & T_SIDE)    { _F(T_SIDE,    (_l)->sides,    __VA_ARGS__) } \
    if ((_T) & T_DECAL)   { _F(T_DECAL,   (_l)->decals,   __VA_ARGS__) } \
    if ((_T) & T_OBJECT)  { _F(T_OBJECT,  (_l)->objects,  __VA_ARGS__) }

void *_level_alloc_impl(
    level_t *level, DYNLIST(void*) *list, int tsize, u8 *gen, u16 *index);

void _level_free_impl(
    level_t *level, DYNLIST(void*) *list, void *ptr, int index);

// allocate an object in a level dynlist
// level_alloc(level_t*, DYNLIST(...))
#define level_alloc(_l, _list) ({                                      \
        u8 gen; u16 index;                                         \
        TYPEOF((_list)[0]) _p =                                    \
            _level_alloc_impl(                                         \
                _l,                                                \
                (DYNLIST(void*)*) &_list,                          \
                sizeof(*_p),                                       \
                &gen,                                              \
                &index);                                           \
        _p->gen = gen;                                             \
        _p->index = index;                                         \
        _p;                                                        \
    })

// free an object in a level dynlist
#define level_free(_l, _list, _p) ({                                   \
        _level_free_impl(_l, (DYNLIST(void*)*) &_list, _p, _p->index); \
    })

// internal use only
// seek in list _d with iterator _it to next non-NULL element
#define _level_dynlist_seek(_d, _it) ({                                \
        const int _size = dynlist_size((_d));                      \
        while (_it.i < _size && !(_d)[_it.i]) _it.i++;             \
        _it.el = _it.i == _size ? NULL : &(_d)[_it.i];             \
        _it;                                                       \
    })

#define _level_dynlist_each_impl(_d, _it, _pname, _itname)             \
    typedef struct {                                               \
        int i;                                                     \
        UNCONST(TYPEOF(&(_d)[0])) el; } _itname;                   \
    TYPEOF(&(_d)) _pname = &(_d);                                  \
    for (_itname _it = ({                                          \
            _itname _i = { 0, NULL };                              \
            _level_dynlist_seek(*_pname, _i);                          \
        });                                                        \
        _it.i < dynlist_size(*_pname);                             \
        _it.i++,_level_dynlist_seek(*_pname, _it))                     \

// iterate a level dynlist (skips NULL elements)
#define level_dynlist_each(_list, _p)                                  \
    _level_dynlist_each_impl(                                          \
         _list,                                                    \
         _p,                                                       \
         CONCAT(_lde, __COUNTER__),                                \
         CONCAT(_ldi, __COUNTER__))

// TODO: actors, etc. are NOT ALLOWED at edit time - only in-game.

// T_* (| T_* ...) -> string
int level_types_to_str(char *dst, usize n, u8 tag);

// get level dynlist for specified type
DYNLIST(void*) *level_get_list(const level_t *level, int type);

// get number of elements in level dynlist
int level_get_list_count(const level_t *level, int type);

// get generation of specified level object
u8 level_get_gen(const level_t *level, int type, int i);

// init level
void level_init(level_t *level);

// destroy level
void level_destroy(level_t *level);

// update level
void level_update(level_t *level, f32 dt);

// tick level
void level_tick(level_t *level);

// traces a continous chain of sides with a minimal inner angle starting from
// startside. returns 0 on failure to trace a complete shape.
int level_trace_sides(
    level_t *level,
    side_t *startside,
    DYNLIST(side_t*) *sides);

// finds sector vertex and wall share, NULL if they share no sector
// find a side near vertex in sector, otherwise null
side_t *level_find_near_side(
    const level_t *level,
    const vertex_t *vertex,
    const sector_t *preferred_sector);

// find a sector near to a point, otherwise NULL
sector_t *level_find_near_sector(
    const level_t *level,
    vec2s point);

// update the sector of a side, trying to form new sectors as appropriate
sector_t *level_update_side_sector(
    level_t *level,
    side_t *side);

// intersect all level walls along p0 -> p1, returning a list of walls hit AND
// a list of pointers to which sides of each wall the "exit" is on (side whose
// normal is closest to that of p0 -> p1, regardless of if it exists)
int level_intersect_walls_on_line(
    level_t *level,
    vec2s p0,
    vec2s p1,
    wall_t **walls,
    side_t ***sides,
    int n);

// returns side in list which has an other side which is on the same wall
// if none, returns NULL
side_t *level_sides_find_double(
    level_t *level,
    side_t **sides,
    int n);

// returns side in sector which has other which is also in sector
// if none, returns NULL
side_t *level_sector_find_double_side(
    level_t *level,
    sector_t *sector);

// a sector is "coherent" iff:
// * all sides which can be traced from other sector sides are also part of the
//   sector
// * all holes are also part of the sector, and follow the above criteria
bool level_sector_is_coherent(
    level_t *level,
    sector_t *sector);

// finds nearest side to point, NULL if there is no such side
side_t *level_nearest_side(
    const level_t *level,
    vec2s point);

// finds nearest vertex to point, NULL if there is no such vertex
vertex_t *level_nearest_vertex(
    const level_t *level,
    vec2s point,
    f32 *dist);

// "updates" the sector of a point by searching in nearby sectors first to see
// if they contain the point
// returns SECTOR_NONE if the point is not in a sector
sector_t *level_find_point_sector(
    level_t *level,
    vec2s point,
    sector_t *sector);

// nearest position to pos which is on a side
void level_nearest_side_pos(
    const level_t *level,
    vec2s pos,
    side_t **pside,
    vec2s *pw,
    f32 *px);

// returns true if sector a is visible from sector b
bool level_is_sector_visible_from(level_t *level, sector_t *a, sector_t *b);

enum {
    LEVEL_GET_VISIBLE_SECTORS_NONE = 0,
    LEVEL_GET_VISIBLE_SECTORS_PORTALS = 1 << 0
};

// get sectors visible from sector s
int level_get_visible_sectors(
    level_t *level, sector_t *s, DYNLIST(sector_t*) *out, int flags);

// get all walls in specified radius
int level_walls_in_radius(
    level_t *level, vec2s pos, f32 r, DYNLIST(wall_t*) *out);

int level_sectors_in_radius(
    level_t *level, vec2s pos, f32 r, DYNLIST(sector_t*) *out);
