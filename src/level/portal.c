#include "level/portal.h"
#include "level/level_defs.h"
#include "level/side.h"
#include "level/wall.h"

f32 portal_angle(level_t *level, side_t *entry, side_t *exit) {
    const vec2s
        na = side_normal(entry),
        nb = side_normal(exit);

    // compute signed angle between -nb, na
    return -(atan2f(-nb.y, -nb.x) - atan2f(na.y, na.x));
}

vec2s portal_transform(level_t *level, side_t *entry, side_t *exit, vec2s p) {
    const f32 angle = portal_angle(level, entry, exit);
    const vec2s ms[2] = {
        wall_midpoint(entry->wall),
        wall_midpoint(exit->wall), };

    // undo translation relative to portal, rotate by difference
    const vec2s v = rotate(glms_vec2_sub(p, ms[0]), -angle);

    // translate relative to exit portal
    return glms_vec2_add(v, ms[1]);
}
