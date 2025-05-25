#pragma once

#include "defs.h"

// get angle between two portal  sides
f32 portal_angle(level_t *level, side_t *entry, side_t *exit);

// transform point as travelling between two portals
vec2s portal_transform(level_t *level, side_t *entry, side_t *exit, vec2s p);
