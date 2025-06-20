#pragma once

// operations for f_rh_entry
// RH_INIT: client should initialize
// RH_DEINIT: client should close
// RH_STEP: client step (loop operation)
// RH_RELOAD: client has been reloaded
typedef enum {
    RH_INIT,
    RH_DEINIT,
    RH_STEP,
    RH_PRE_RELOAD,
    RH_RELOAD
} reload_host_op;

// f_rh_entry returns this to request that it be called with RH_DEINIT
#define RH_CLOSE_REQUESTED INT_MAX

// name of f_rh_entry function in client
#define RH_ENTRY_NAME rh_entry

typedef struct reload_host reload_host_t;

// see reload_host::regfunc
typedef void (*rh_regfunc_f)(void **p);

// see reload_host::delfunc
typedef void (*rh_delfunc_f)(void **p);

// type of entry function in client
typedef int (*rh_entry_f)(int, char*[], reload_host_op, reload_host_t*);

typedef void (*rh_hook_callback_f)(void*);

// see reload_host::hook_fn
typedef int (*rh_hook_f)(rh_hook_callback_f, void*);

// see reload_host::unhook_fn
typedef void (*rh_unhook_f)(int);

typedef struct reload_host {
    // pointer to function pointer registry function
    //
    // usage:
    //
    // struct foo { int (*funcptr)(); }
    // int myfunc() { ... }
    // struct foo f = malloc(sizeof(struct foo));
    // f->funcptr = myfunc;
    // reload_host->regfunc(&f->funcptr);
    //
    // now f->funcptr is properly changed on code reload
    rh_regfunc_f reg_fn;

    // see regfunc
    // when heap storage for function pointer is free'd, call delfunc() with the
    // same address to free the function pointer in the reload_host
    rh_delfunc_f del_fn;

    // register some function as a reload "hook" to be called after reload with
    // the specified data pointer as an argument. returns a unique id.
    rh_hook_f hook_fn;

    // removes a previously registered hook_fn
    rh_unhook_f unhook_fn;

    // userdata pointer, can be used by client for arbitrary storage on reload
    // host
    void *userdata;

    // project specific!
    void *imgui_ctx;
} reload_host_t;

