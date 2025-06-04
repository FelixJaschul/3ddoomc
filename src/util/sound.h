#pragma once

#include "util/map.h"
#include "config.h"
#include "defs.h"

typedef void *Soloud;

typedef struct sound_state {
    Soloud *soloud;
    map_t sounds;
    map_t handles;
} sound_state_t;

typedef unsigned int sound_id;

// init sound system
int sound_init();

// destroy sound system
void sound_destroy();

// clear all loaded sounds
void sound_clear();

// TODO: doc
sound_id sound_play(resource_t name);

sound_id sound_play_loop(resource_t name);

void sound_stop(sound_id id);

void sound_stop_all();
