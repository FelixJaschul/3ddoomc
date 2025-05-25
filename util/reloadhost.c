#ifndef UTIL_IMPL
#define UTIL_IMPL
#endif // ifndef UTIL_IMPL

#define RELOAD_HOST

#include "../src/util/assert.h"
#include "../src/util/log.h"
#include "../src/util/types.h"
#include "../src/util/map.h"

#include "reloadhost.h"

// PROJECT SPECIFIC
#include "../src/imgui.h"
// END PROJECT SPECIFIC

#include <dlfcn.h>
#include <sys/stat.h>
#include <time.h>

typedef struct {
    char *name;
    void *storage;
} func_storage_t;

typedef struct {
    char *name;
    rh_hook_callback_f func;
    void *userdata;
} hook_t;

// persistent data exposed to client
static reload_host_t rh;

// currently loaded module
static void *module = NULL;

// map of storage address -> char* function name
static map_t funcs;

// next hook id, arbitrary
static int next_hook;

// map of hook id -> hook_t
static map_t hooks;

// see reload_host::reg_fn
static void reg_fn(void **p) {
    Dl_info info;
    dladdr(*p, &info);
    map_insert(&funcs, p, strdup(info.dli_sname));
    /* LOG("registering function %s @ %p", info.dli_sname, p); */
}

// see reload_host::del_fn
static void del_fn(void **p) {
    /* LOG("removing function pointer @ %p", p); */
    map_remove(&funcs, p);
}

static int hook(rh_hook_callback_f func, void *userdata) {
    Dl_info info;
    dladdr((const void*) func, &info);

    hook_t *hook = malloc(sizeof(hook_t));
    *hook = (hook_t) {
        .name = strdup(info.dli_sname),
        .func = func,
        .userdata = userdata
    };

    const int id = ++next_hook;
    map_insert(&hooks, id, hook);
    return id;
}

static void unhook(int id) {
    hook_t **phook = map_find(hook_t*, &hooks, id);
    if (!phook) { WARN("unhooking invalid id %d", id); return; }

    free((*phook)->name);
    free(*phook);

    map_remove(&hooks, id);
}

int main(int argc, char *argv[]) {
    ASSERT(argc >= 2, "usage: reload_host <module> [args...]");

    rh = (reload_host_t) {
        .reg_fn = reg_fn,
        .del_fn = del_fn,
        .hook_fn = hook,
        .unhook_fn = unhook,
        .userdata = NULL
    };

    map_init(
        &funcs,
        map_hash_id,
        NULL,
        NULL,
        NULL,
        map_cmp_id,
        NULL,
        map_default_free,
        NULL);

    map_init(
        &hooks,
        map_hash_id,
        NULL,
        NULL,
        NULL,
        map_cmp_id,
        NULL,
        NULL,
        NULL);

    struct timespec mod_time;
    rh_entry_f func = NULL;
    reload_host_op op = RH_INIT;

    bool first_try = false;

    char path[1024], cmd[1024];
    path[0] = '\0';
    cmd[0] = '\0';

    while (true) {
        struct stat st;
        int res = stat(argv[1], &st);

        if (res < 0) {
            WARN("failed to stat %s", argv[1]);

            if (first_try) {
                ASSERT(false);
            } else {
                continue;
            }
        }

        first_try = false;

        struct timespec ts;
        timespec_get(&ts, TIME_UTC);

        // reload
        if (op == RH_INIT
            || (st.st_mtimespec.tv_sec > mod_time.tv_sec
                && ts.tv_sec > st.st_mtimespec.tv_sec + 1)) {
            if (module) {
                res = func(argc - 1, &argv[1], RH_PRE_RELOAD, &rh);
                LOG("(reloadhost) module is %p", module);
                ASSERT(!res, "failed RH_PRE_RELOAD: %d", res);

                ASSERT(!dlclose(module));
                module = NULL;
            }

            if (strlen(path) != 0) {
                snprintf(cmd, sizeof(cmd), "rm %s", path);
                LOG("(reloadhost) executing %s", cmd);
                res = system(cmd);
                LOG("(reloadhost)  returned %d", res);
            }

            // copy to tmp, open from there
            snprintf(
                path,
                sizeof(path),
                "/tmp/reload-%d.dylib",
                rand());

            snprintf(
                cmd,
                sizeof(cmd),
                "cp %s %s",
                argv[1],
                path);

            LOG("(reloadhost) executing %s", cmd);
            system(cmd);

            dlerror();
            module = dlopen(path, RTLD_LOCAL | RTLD_LAZY);
            LOG("(reloadhost) module is %p", module);
            ASSERT(module, "failed to load: %s", dlerror());

            func = (rh_entry_f) dlsym(module, STRINGIFY(RH_ENTRY_NAME));
            ASSERT(func, "could not find entry");
            mod_time = st.st_mtimespec;

            if (op != RH_INIT) {
                op = RH_RELOAD;
            }
        }

        if (op == RH_RELOAD) {
            // reload function pointers
            map_each(void**, char*, &funcs, it) {
                void *sym = dlsym(module, it.value);
                ASSERT(
                    sym,
                    "could not reload function pointer %s", it.value);
                memcpy(it.key, &sym, sizeof(void*));
            }

            // reload hook function pointers
            map_each(int, hook_t*, &hooks, it) {
                void *sym = dlsym(module, it.value->name);
                ASSERT(
                    sym,
                    "could not reload hook function pointer %s", it.value);
                memcpy(&it.value->func, &sym, sizeof(void*));
            }
        }

        res = func(argc - 1, &argv[1], op, &rh);
        if (res) {
            if (res == RH_CLOSE_REQUESTED) {
                LOG("client requested close, exiting");
                return func(argc - 1, &argv[1], RH_DEINIT, &rh);
            } else {
                LOG("client exited with code %d", res);
                return res;
            }
        }

        // call hooks
        if (op == RH_RELOAD) {
            map_each(int, hook_t*, &hooks, it) {
                it.value->func(it.value->userdata);
            }
        }

        op = RH_STEP;
    }

    // TODO: properly dealloc hooks
    map_destroy(&hooks);
    map_destroy(&funcs);

    return 0;
}
