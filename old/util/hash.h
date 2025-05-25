#pragma once

#include "util/types.h"

#define DECL_HASH_ADD(_t)                                                    \
    ALWAYS_INLINE hash_t hash_add_##_t(hash_t hash, _t x) {                  \
        return                                                               \
            (hash ^                                                          \
                (((hash_t) (x)) + 0x9E3779B9u + (hash << 6) + (hash >> 2))); \
    }

DECL_HASH_ADD(u8)
DECL_HASH_ADD(u16)
DECL_HASH_ADD(u32)
DECL_HASH_ADD(u64)

DECL_HASH_ADD(i8)
DECL_HASH_ADD(i16)
DECL_HASH_ADD(i32)
DECL_HASH_ADD(i64)

DECL_HASH_ADD(usize)
DECL_HASH_ADD(isize)

DECL_HASH_ADD(char)
DECL_HASH_ADD(int)

ALWAYS_INLINE hash_t hash_add_str(hash_t hash, const char *s) {
    while (*s) { hash = hash_add_char(hash, *s); s++; }
    return hash;
}

ALWAYS_INLINE hash_t hash_add_uintptr(hash_t hash, uintptr_t x) {
    return
        (hash ^
            (((hash_t) (x)) + 0x9E3779B9u + (hash << 6) + (hash >> 2)));
}

ALWAYS_INLINE hash_t hash_add_ptr(hash_t hash, void *ptr) {
    return hash_add_uintptr(hash, (uintptr_t) ptr);
}

ALWAYS_INLINE hash_t hash_add_ptrs(hash_t hash, void **ptrs, int n) {
    for (int i = 0; i < n; i++) {
        hash = hash_add_ptr(hash, ptrs[i]);
    }
    return hash;
}
