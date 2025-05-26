#pragma once

#include "defs.h"

particle_t *particle_new(level_t *level, vec2s pos);

void particle_delete(level_t *level, particle_t *particle);

void particle_tick(level_t *level, particle_t *particle);

void particle_instance(level_t *level, particle_t *p, sprite_instance_t *inst);
