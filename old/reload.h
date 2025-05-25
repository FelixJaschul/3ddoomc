#pragma once

#include "config.h"

#ifdef RELOADABLE
#include "../util/reloadhost.h"
extern reload_host_t *g_reload_host;

#define RELOAD_VISIBLE
#else
#define RELOAD_VISIBLE static
#endif // ifdef RELOADABLE

