#pragma once

#include <stdarg.h>
#include <stdio.h>

#include "util/macros.h"

#define RESOURCE_NAME_LEN 16

typedef struct resource {
    char name[RESOURCE_NAME_LEN];
} resource_t;

#define RESOURCE_STR(_r) (&(_r).name[0])

#define AS_RESOURCE(_s) ((resource_t) { .name = (_s) })

ALWAYS_INLINE void resource_printf(resource_t *r, const char *fmt, ...) {
    va_list ap;
    va_start(ap);
    vsnprintf(r->name, sizeof(r->name), fmt, ap);
    va_end(ap);
}

ALWAYS_INLINE resource_t resource_from(const char *fmt, ...) {
    va_list ap;
    va_start(ap);
    resource_t r;
    vsnprintf(r.name, sizeof(r.name), fmt, ap);
    va_end(ap);
    return r;
}

// checks against global resource name lists
bool resource_name_valid(const char *name);

// appends _%d to name to make it valid, always appending some number (min 1)
resource_t resource_next_valid(const char *base_name);

// searches res/ directory to check if resource is file, optionally outputting
// the full path into "path"
// example: resource_is_file(AS_RESOURCE("SOMETHING"), "png") -> checks if
// res/SOMETHING.png OR res/something.png exist in res/
bool resource_find_file(
    resource_t resource,
    const char *base_dir,
    const char *ext,
    char *path,
    int n);
