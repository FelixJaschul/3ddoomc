#include "level/particle.h"
#include "level/level.h"
#include "level/path.h"
#include "level/side.h"
#include "gfx/renderer.h"
#include "util/math.h"

particle_t *particle_new(level_t *level, vec2s pos) {
    sector_t *sector = level_find_point_sector(level, pos, NULL);
    if (!sector) {
        WARN("attempt to create new particle out of sector at %" PRIv2,
             FMTv2(pos));
        return NULL;
    }

    // try and find free ID
    particle_id id;
    if ((id =
            bitmap_find(
                level->particle_ids,
                dynlist_size(level->particles),
                0, false)) == INT_MAX) {
        // expand table, take most recently added ID
        id = dynlist_size(level->particles);

        // expand by 32 elements
        dynlist_resize(level->particles, dynlist_size(level->particles) + 32);
        level->particle_ids =
            bitmap_realloc_zero(
                level->particle_ids,
                id,
                dynlist_size(level->particles));
    }

    bitmap_set(level->particle_ids, id);

    particle_t *particle = &level->particles[id];
    memset(particle, 0, sizeof(*particle));
    particle->id = id;
    particle->pos = pos;
    particle->sector = sector;
    *dynlist_push(sector->particles) = particle->id;
    return particle;
}

void particle_delete(level_t *level, particle_t *particle) {
    bitmap_clr(level->particle_ids, particle->id);

    // TODO: bad, don't do this, use block list for pointer stability
    bool found = false;
    dynlist_each(particle->sector->particles, it) {
        if (*it.el == particle->id) {
            dynlist_remove_it(particle->sector->particles, it);
            found = true;
            break;
        }
    }

    ASSERT(found);

    // shrink particle ID list
    if (dynlist_size(level->particles) > 32
        && bitmap_find(
            level->particle_ids,
            dynlist_size(level->particles),
            dynlist_size(level->particles) - 32,
            true) == INT_MAX) {
        dynlist_resize(level->particles, dynlist_size(level->particles) - 32);
        level->particle_ids =
            bitmap_realloc(
                level->particle_ids,
                dynlist_size(level->particles));
    }
}

void particle_move(level_t *level, particle_t *p, vec2s pos) {
    if (glms_vec2_eqv_eps(pos, p->pos)) {
        return;
    }

    p->pos.x = max(pos.x, 0);
    p->pos.y = max(pos.y, 0);

    // update sector (move if sector is no longer correct)
    sector_t *new_sect =
        level_find_point_sector(level, p->pos, p->sector);

    if (!new_sect) {
        WARN("particle %d out of sector", p->id);
        particle_delete(level, p);
        return;
    }

    if (p->sector != new_sect) {
        bool found = false;
        dynlist_each(p->sector->particles, it) {
            if (*it.el == p->id) {
                dynlist_remove_it(p->sector->particles, it);
                found = true;
                break;
            }
        }

        ASSERT(found);

        *dynlist_push(new_sect->particles) = p->id;
        p->sector = new_sect;
    }
}

static int resolve_particle(
    level_t *level,
    const path_hit_t *hit,
    vec2s *pfrom,
    vec2s *pto,
    particle_t *p) {
    bool collide = true;

    // allow movement (downward) between portals
    side_t *portal = hit->wall.side->portal;
    if (portal && portal->sector && portal->sector->floor.z <= p->z) {
        collide = false;

        int res = PATH_TRACE_CONTINUE;
        f32 portal_angle = 0.0f;
        if (path_trace_resolve_portal(
                level, hit, pfrom, pto, &res, &portal_angle,
                PATH_TRACE_RESOLVE_PORTAL_NONE)) {
            p->vel = rotate(p->vel, portal_angle);
            return res;
        }
    }

    // do not process further collisions
    if (!collide) {
        return PATH_TRACE_CONTINUE;
    }

    const particle_type_t *type = &PARTICLE_TYPES[p->type];

    // project movement vector onto wall to "slide"
    const vec2s
        projected =
            project_vec2(
                p->vel,
                glms_vec2_scale(
                    glms_vec2_normalize(hit->wall.wall->d),
                    side_is_left(hit->wall.side) ? 1 : -1)),
        normal = side_normal(hit->wall.side);

    // velocity gets + -velocity projected onto -normal * restitution
    const vec2s
        v_towards_wall =
            project_vec2(
                glms_vec2_normalize(p->vel),
                glms_vec2_scale(normal, -1)),
        rest =
            glms_vec2_mul(
                p->vel,
                glms_vec2_mul(
                    v_towards_wall,
                    glms_vec2(type->restitution)));

    p->vel = glms_vec2_add(projected, rest);

    *pto = hit->swept_pos;

    return PATH_TRACE_RETRY;
}

void particle_tick(level_t *level, particle_t *p) {
    p->ticks--;

    if (p->ticks <= 0) {
        particle_delete(level, p);
        return;
    }

    const particle_type_t *type = &PARTICLE_TYPES[p->type];

    // move according to velocity
    if (fabsf(p->vel.x) < 0.001f) { p->vel.x = 0.0f; }
    if (fabsf(p->vel.y) < 0.001f) { p->vel.y = 0.0f; }

    if (glms_vec2_eqv_eps(p->vel, VEC2(0))) {
        goto move_z;
    }

    // attempt xy-axis movement
    vec2s
        dt_vel = glms_vec2_scale(p->vel, TICK_DT),
        from = p->pos,
        to = glms_vec2_add(p->pos, dt_vel);

    path_trace(
        level,
        &from,
        &to,
        0.0f,
        (path_trace_resolve_f) resolve_particle,
        p,
        PATH_TRACE_NONE);

    if (!level_find_point_sector(level, to, p->sector)) {
        /* WARN("particle %d attempting move out of sector", p->id); */
        particle_delete(level, p);
        return;
    } else if (!glms_vec2_eqv_eps(from, to)) {
        particle_move(level, p, to);
    }

move_z:
    p->vel_z += type->gravity.z;

    if (fabsf(p->vel_z) < 0.001f) {
        p->vel_z = 0.0f;
        return;
    } else if (!p->sector) {
        return;
    }

    // attempt z-axis movement
    const f32 z_new = p->z + (TICK_DT * p->vel_z);

    const bool
        grounded = z_new < p->sector->floor.z,
        z_hit = grounded || z_new > p->sector->ceil.z;

    if (z_hit) {
        p->vel_z = -p->vel_z * type->restitution.z;
    }

    p->z = clamp(z_new, p->sector->floor.z, p->sector->ceil.z);

    // apply drag
    const vec3s drag = grounded ? type->floor_drag : type->air_drag;

    // apply drag: vel -= vel * drag * dt
    p->vel =
        glms_vec2_sub(
            p->vel,
            glms_vec2_mul(
                p->vel,
                glms_vec2_scale(
                    glms_vec2(drag),
                    TICK_DT)));
    p->vel_z -= p->vel_z * drag.z * TICK_DT;
}

void particle_instance(level_t *level, particle_t *p, sprite_instance_t *inst) {
    switch (p->type) {
    case PARTICLE_TYPE_FLARE: {
        inst->color = p->flare.color;
    } break;
    default:
    }
}

particle_type_t PARTICLE_TYPES[PARTICLE_TYPE_COUNT] = {
    [PARTICLE_TYPE_SPLAT] = {
        .tex = { .name = "NOTEX" },
        .gravity = (vec3s) {{ 0.0f, 0.0f, -0.45f, }},
        .floor_drag = (vec3s) {{ 10.0f, 10.0f, 0.5f }},
        .air_drag = (vec3s) {{ 1.8f, 1.8f, 0.5f }},
        .restitution = (vec3s) {{ 0.6f, 0.6f, 0.5f, }},
    },
    [PARTICLE_TYPE_FLARE] = {
        .tex = { .name = "PPART1" },
        .gravity = (vec3s) {{ 0.0f, 0.0f, -0.45f, }},
        .floor_drag = (vec3s) {{ 10.0f, 10.0f, 0.5f }},
        .air_drag = (vec3s) {{ 1.8f, 1.8f, 0.5f }},
        .restitution = (vec3s) {{ 0.6f, 0.6f, 0.5f, }},
    }
};
