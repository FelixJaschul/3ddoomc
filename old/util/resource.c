#include "util/resource.h"
#include "gfx/atlas.h"
#include "level/level.h"
#include "state.h"
#include "util/file.h"

bool resource_name_valid(const char *name) {
    if (state->atlas) {
        dynlist_each(state->atlas->names, it) {
            if (!strcmp(name, *it.el)) {
                return false;
            }
        }
    }

    if (state->level) {
        level_dynlist_each(state->level->sectmats, it) {
            if (!strcmp(RESOURCE_STR((*it.el)->name), name)) {
                return false;
            }
        }

        level_dynlist_each(state->level->sidemats, it) {
            if (!strcmp(RESOURCE_STR((*it.el)->name), name)) {
                return false;
            }
        }
    }

    return true;
}

resource_t resource_next_valid(const char *base_name) {
    int n = 1;
    resource_t name;
    do {
        name = resource_from("%s_%d", base_name, n);
        n++;
    } while (!resource_name_valid(RESOURCE_STR(name)));
    return name;
}

bool resource_find_file(
    resource_t resource, const char *base_dir, const char *ext, char *path, int n) {
    if (strlen(resource.name) == 0) { return false; }

    resource_t upper = resource, lower = resource;
    strtoupper(upper.name);
    strtolower(lower.name);

    char paths[2][1024];
    snprintf(paths[0], sizeof(paths[0]), "%s/%s.%s", base_dir, upper.name, ext);
    snprintf(paths[1], sizeof(paths[1]), "%s/%s.%s", base_dir, lower.name, ext);

    if (file_exists(paths[0])) {
        if (path) { strncpy(path, paths[0], n); }
        return true;
    } else if (file_exists(paths[1])) {
        if (path) { strncpy(path, paths[1], n); }
        return true;
    }

    return false;
}
