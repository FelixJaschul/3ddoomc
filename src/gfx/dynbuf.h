#pragma once

#include "util/map.h"
#include "util/types.h"
#include "util/bitmap.h"
#include "util/dynlist.h"
#include "util/dlist.h"

typedef struct dynbuf_region dynbuf_region_t;

typedef struct {
    void *ptr;
    usize capacity;

    // used area, that is, the end of "rightmost" used area
    usize used;

    int flags;

    DLIST(dynbuf_region_t) list;

    // void* -> dynbuf_region_t*
    map_t lookup;
} dynbuf_t;

enum {
    DYNBUF_NONE = 0,
    DYNBUF_OWNS_MEMORY = 1 << 0
};

void dynbuf_init(dynbuf_t *buf, void *ptr, usize capacity, int flags);

void dynbuf_destroy(dynbuf_t *buf);

void dynbuf_reset(dynbuf_t *buf);

void *dynbuf_alloc(dynbuf_t *buf, usize n);

void dynbuf_free(dynbuf_t *buf, void *p);
