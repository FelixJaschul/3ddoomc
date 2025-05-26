#pragma once

#include "util/types.h"
#include <time.h>

ALWAYS_INLINE u64 time_ns() {
    struct timespec ts;
    // TODO: CLOCK_MONOTONIC is not portable (use REALTIME for emscripten)
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * ((u64) 1000000000UL)) + (u64) (ts.tv_nsec);
}
