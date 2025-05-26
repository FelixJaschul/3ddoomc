#pragma once

#include "defs.h"

// init object tables
void objects_init();

object_t *object_new(level_t *level);
void object_delete(level_t *level, object_t *object);

// destroy an object *during gameplay*
void object_destroy(level_t *level, object_t *obj);

// set object's type
void object_set_type(
    level_t *level,
    object_t *object,
    object_type_index type_index);

// set object position
void object_move(level_t *level, object_t *object, vec2s pos);

// perform per-frame update on object
// dt is expected in seconds
void object_update(level_t *level, object_t *obj, f32 dt);
