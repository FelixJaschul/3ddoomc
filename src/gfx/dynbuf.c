#include "gfx/dynbuf.h"

// TODO: allocations should start looking from the last known free node (rover)
// TODO: worry about alignment? theoretically not necessary if you only ever
// allocate the same type with same padding

// #define DO_DEBUG_DYNBUFS

#ifdef DO_DEBUG_DYNBUFS
#define DEBUG_DYNBUFS(...) DEBUG_DYNBUFS(__VA_ARGS__)
#else
#define DEBUG_DYNBUFS(...)
#endif // ifdef DO_DEBUG_DYNBUFS

typedef struct dynbuf_region {
    void *ptr;
    u32 size;
    bool free;
    DLIST_NODE(struct dynbuf_region) node;
} dynbuf_region_t;

void dynbuf_init(dynbuf_t *buf, void *ptr, usize capacity, int flags) {
    *buf = (dynbuf_t) {
        .ptr = ptr,
        .capacity = capacity,
        .flags = flags,
    };

    map_init(
        &buf->lookup,
        map_hash_id,
        NULL,
        NULL,
        NULL,
        map_cmp_id,
        NULL,
        NULL,
        NULL);

    dlist_init(&buf->list);

    dynbuf_region_t *base = calloc(1, sizeof(dynbuf_region_t));
    *base = (dynbuf_region_t) {
        .ptr = ptr,
        .size = capacity,
        .free = true
    };
    dlist_prepend(node, &buf->list, base);
}

void dynbuf_destroy(dynbuf_t *buf) {
    if (buf->flags & DYNBUF_OWNS_MEMORY) {
        free(buf->ptr);
    }

    dlist_each(node, &buf->list, it) {
        dlist_remove(node, &buf->list, it.el);
        free(it.el);
    }

    map_destroy(&buf->lookup);
}

void dynbuf_reset(dynbuf_t *buf) {
    dlist_each(node, &buf->list, it) {
        dlist_remove(node, &buf->list, it.el);
        free(it.el);
    }
    map_clear(&buf->lookup);

    dlist_init(&buf->list);
    dynbuf_region_t *base = calloc(1, sizeof(dynbuf_region_t));
    *base = (dynbuf_region_t) {
        .ptr = buf->ptr,
        .size = buf->capacity,
        .free = true
    };
    dlist_prepend(node, &buf->list, base);
}

void *dynbuf_alloc(dynbuf_t *buf, usize n) {
    if (n == 0) {
        WARN("dynbuf @ %p got 0 byte alloc", buf);
        return NULL;
    }

    n = round_up_to_mult(n, max(alignof(max_align_t), 16));

    DEBUG_DYNBUFS("allocating %p: %" PRIusize, buf, n);
    // search for a large enough space
    dlist_each(node, &buf->list, it) {
        if (!it.el->free || it.el->size < n) { continue; }

        if (it.el->size > n) {
            // add new region on tail
            dynbuf_region_t *region = malloc(sizeof(dynbuf_region_t));
            *region = (dynbuf_region_t) {
                .ptr = it.el->ptr + n,
                .size = it.el->size - n,
                .free = true
            };
            DEBUG_DYNBUFS("%p: new node %p (%d)", buf, region, it.el->size - n);

            dlist_insert_after(node, &buf->list, it.el, region);
        } else if (it.el->size == n) {
            DEBUG_DYNBUFS("found exact!");
        }

        it.el->size = n;
        it.el->free = false;
        map_insert(&buf->lookup, it.el->ptr, it.el);

        buf->used =
            buf->list.tail->node.prev ?
                (buf->list.tail->ptr - buf->ptr)
                : 0;

        return it.el->ptr;
    }

    WARN(
        "dynbuf @ %p: failed to allocate %" PRIusize " bytes",
         buf,
         n);
    return NULL;
}

void dynbuf_free(dynbuf_t *buf, void *p) {
    // ensure region exists
    dynbuf_region_t **pslot = map_find(dynbuf_region_t*, &buf->lookup, p);
    ASSERT(pslot, "dynbuf @ %p: invalid free %p", buf, p);

    // ensure region is in list
    dynbuf_region_t *region = *pslot;
    ASSERT(
        region == buf->list.head
        || region == buf->list.tail
        || region->node.prev
        || region->node.next,
        "dynbuf @ %p: region %p not in list",
        buf,
        region);

    map_remove(&buf->lookup, p);
    region->free = true;

    dynbuf_region_t
        *left = region->node.prev,
        *right = region->node.next;

    // left coalesce
    if (left && left->free) {
        if (left->node.prev) {
            left->node.prev->node.next = region;
        } else if (left == buf->list.head) {
            buf->list.head = region;
        }

        region->node.prev = left->node.prev;
        region->ptr = left->ptr;
        region->size += left->size;
        DEBUG_DYNBUFS("%p: REMOVE (left) node %p", buf, left);
        free(left);
    }

    // right coalesce
    if (right && right->free) {
        if (right->node.next) {
            right->node.next->node.prev = region;
        } else if (right == buf->list.tail) {
            buf->list.tail = region;
        }

        region->node.next = right->node.next;
        region->size += right->size;
        free(right);
        DEBUG_DYNBUFS("%p: REMOVE (right) node %p", buf, right);
    }

    buf->used =
        buf->list.tail->node.prev ?
            (buf->list.tail->ptr - buf->ptr)
            : 0;
}
