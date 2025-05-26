#include "level/object.h"
#include "editor/editor.h"
#include "level/level_defs.h"
#include "level/level.h"
#include "level/block.h"
#include "level/lptr.h"
#include "level/actor.h"
#include "level/path.h"
#include "level/portal.h"
#include "level/side.h"
#include "state.h"
#include "util/input.h"

enum {
    AI_STATE_SLEEP = 0,
    AI_STATE_IDLE = 1,
    AI_STATE_WALK = 2,
};

typedef struct {
    vec2s dir;

    int dir_tick;
    int hit_tick;
    int see_tick;
    int cooldown;
    int health;
    int ai_state;
} object_boy_t;

typedef struct {

} object_projectile_t;

void objects_init() {
    for (int i = 0; i < OT_COUNT; i++) {
        // TODO
        /* object_type_t *ot = &OBJECT_TYPE_INFO[i]; */
        /* atlas_update_nameindex(&state->textures, &ot->sprite); */
    }
}

object_t *object_new(level_t *level) {
    object_t *o = level_alloc(level, level->objects);
    o->state = OS_DEFAULT;
    o->type_index = OT_PLACEHOLDER;
    o->type = &OBJECT_TYPES[OT_PLACEHOLDER];
    return o;
}

void object_delete(level_t *level, object_t *o) {
#ifdef MAPEDITOR
    if (state->mode == GAMEMODE_EDITOR) {
        editor_close_for_ptr(state->editor, LPTR_FROM(o));
    }
#endif //ifdef MAPEDITOR

    // remove object from list
    if (o->sector) {
        dlist_remove(sector_list, &o->sector->objects, o);
    }

    if (o->block) {
        dlist_remove(block_list, &o->block->objects, o);
    }

    if (o->ex) {
        free(o->ex);
        o->ex = NULL;
    }

    if (o->actor) {
        o->actor->flags |= AF_DONE;
        o->actor->storage = NULL;
    }

    level_blocks_remove_object(level, o);
    level_free(level, level->objects, o);
}

void object_destroy(level_t *level, object_t *obj) {
    *dynlist_push(level->destroy_objects) = obj;
}

void object_set_type(
    level_t *level,
    object_t *object,
    object_type_index type) {
    if (!object_type_index_is_valid(type)) {
        WARN(
            "invalid object type for object @ %p, resetting",
            object);
        type = OT_PLACEHOLDER;
    } else if (object->type_index == type) {
        WARN("setting type for object @ %p to the same?", object);
    }

    object->type_index = type;
    object->type = &OBJECT_TYPES[type];
    object->funcdata.raw = 0;

    if (object->type->ex_size > 0) {
        object->ex = realloc(object->ex, object->type->ex_size);

        if (object->type->ex_init) {
            memcpy(object->ex, object->type->ex_init, object->type->ex_size);
        } else {
            memset(object->ex, 0, object->type->ex_size);
        }
    } else if (object->ex) {
        free(object->ex);
        object->ex = NULL;
    }

    // complete current actor
    if (object->actor) {
        object->actor->storage = NULL;
        object->actor->flags |= AF_DONE;
        object->actor = NULL;
    }

    // don't add actors when editing
    if ((state->mode == GAMEMODE_GAME
        || (object->type->flags & OTF_EDITACTIVE))
        && object->type->act_fn) {
        actor_add(
            level,
            (actor_t) {
                .act = object->type->act_fn,
                .param = object,
                .flags = AF_NONE
            },
            LPTR_FROM(object));
    }
}

static int resolve_normal(
    level_t *level,
    const path_hit_t *hit,
    vec2s *from,
    vec2s *to,
    object_t *obj) {
    bool collide = true;

    // allow movement between portals
    side_t *portal = hit->wall.side->portal;

    // TODO: configurable step height
    if (portal
        && portal->sector
        && (fabsf(obj->z - portal->sector->floor.z) <= 24
            || obj->z > portal->sector->floor.z)
        && portal->sector->floor.z + obj->type->height
            < portal->sector->ceil.z) {
        collide = false;
    }

    int res = PATH_TRACE_CONTINUE;
    f32 portal_angle = 0.0f;
    if (path_trace_resolve_portal(
            level, hit, from, to, &res, &portal_angle,
            PATH_TRACE_RESOLVE_PORTAL_FORCE_AWAY)) {
        if (hit->wall.side->flags & SIDE_FLAG_DISCONNECT) {
            obj->angle += portal_angle;
            obj->vel = rotate(obj->vel, -portal_angle);

            const f32
                z_floor = hit->wall.side->sector->floor.z,
                nz_floor = hit->wall.side->portal->sector->floor.z;

            if (nz_floor < z_floor) {
                obj->z -= z_floor - nz_floor;
            }
        }
        return res;
    }

    // do not process further collisions
    if (!collide) {
        return PATH_TRACE_CONTINUE;
    }

    // project movement vector onto wall to "slide"
    vec2s
        projected =
            glms_vec2_scale(
                glms_vec2_scale(obj->vel, state->time.dt),
                1.0f +
                    glms_vec2_dot(
                        glms_vec2_normalize(obj->vel),
                        side_normal(hit->wall.side))),
        normal = side_normal(hit->wall.side);

    projected = glms_vec2_add(projected, glms_vec2_scale(normal, 0.0005f));
    obj->vel = glms_vec2_divs(projected, state->time.dt);

    *to = glms_vec2_add(hit->swept_pos, projected);

    /* // TODO: remove */
    /* if (obj->itype == OT_BOY) { */
    /*     object_boy_t *ex = obj->ex; */
    /*     ex->lasthit = *p; */
    /*     ex->lasthittick = state->tick; */
    /* } */

    // TODO: bad bad bad! store last hit on object, but in a good way
    if (obj->type_index == OT_BOY) {
        ((object_boy_t*)obj->ex)->hit_tick = state->time.tick;
    }

    return PATH_TRACE_RETRY;
}

static void update_move(level_t *level, object_t *obj, f32 dt) {
    if (!(obj->type->flags &  OTF_PROJECTILE)) {
        const f32 drag = 8.0f;
        obj->vel.x -= obj->vel.x * drag * dt;
        obj->vel.y -= obj->vel.y * drag * dt;
    }

    if (fabsf(obj->vel.x) < 0.001f) { obj->vel.x = 0.0f; }
    if (fabsf(obj->vel.y) < 0.001f) { obj->vel.y = 0.0f; }

    if (glms_vec2_eqv_eps(obj->vel, VEC2(0))) {
        goto move_z;
    }

    // velocity is in "units (pixels) / second", so scale by dt to get velocity
    // to apply for this update

    // attempt xy-axis movement
    vec2s
        dt_vel = VEC2(dt * obj->vel.x, dt * obj->vel.y),
        from = obj->pos,
        to = glms_vec2_add(obj->pos, dt_vel);

    int path_trace_flags = PATH_TRACE_NONE;
    if (obj->type_index == OT_PLAYER) {
        path_trace_flags |= PATH_TRACE_FORCE_NEAR_PORTALS;
    }

    path_trace(
        level,
        &from,
        &to,
        obj->type->radius,
        (path_trace_resolve_f) resolve_normal,
        obj,
        path_trace_flags);

    if (!level_find_point_sector(level, to, obj->sector)) {
        WARN("object %d attempting move out of sector", obj->index);
    } else if (!glms_vec2_eqv_eps(from, to)) {
        object_move(level, obj, to);
    }

move_z:
    if (fabsf(obj->vel_z) < 0.001f) {
        obj->vel_z = 0.0f;
        return;
    } else if (!obj->sector) {
        return;
    }

    // attempt z-axis movement
    const f32 znew = obj->z + (dt * obj->vel_z);
    const bool zhit =
        znew < obj->sector->floor.z
        || (znew + obj->type->height) > obj->sector->ceil.z;

    if (zhit) {
        obj->vel_z = 0.0f;

        if ((obj->type->flags & OTF_PROJECTILE)) {
            object_destroy(level, obj);
        }
    }

    obj->z = clamp(znew, obj->sector->floor.z, obj->sector->ceil.z);
}

void object_move(level_t *level, object_t *object, vec2s pos) {
    if (glms_vec2_eqv_eps(pos, object->pos)) {
        return;
    }

    object->pos.x = max(pos.x, 0);
    object->pos.y = max(pos.y, 0);

    // update sector (move if sector is no longer correct)
    {
        sector_t *new_sect =
            level_find_point_sector(level, object->pos, object->sector);

        if (!new_sect) {
            WARN(
                "object %d out of sector",
                lptr_to_index(level, LPTR_FROM(object)));

            // TODO
            /* if (state->mode != GAMEMODE_EDITOR) { */
            /*     object_destroy(level, object); */
            /* } */
        }

        if (object->sector != new_sect) {
            if (object->sector) {
                dlist_remove(sector_list, &object->sector->objects, object);
            }

            if (new_sect) {
                dlist_prepend(sector_list, &new_sect->objects, object);
            }

            object->sector = new_sect;
        }
    }

    // update block
    {
        block_t *newblock =
            level_get_block(level, level_pos_to_block(object->pos));

        if (!newblock) {
            WARN(
                "object %d out of block (%f, %f)",
                lptr_to_index(level, LPTR_FROM(object)),
                object->pos.x, object->pos.y);
            // !!! breaks because blockmap is not calculated at start
            /* TODO: object->state = OS_DESTROY; */
        }

        if (object->block != newblock) {
            if (object->block) {
                dlist_remove(block_list, &object->block->objects, object);
            }

            if (newblock) {
                dlist_prepend(block_list, &newblock->objects, object);
            }

            object->block = newblock;
        }
    }
}

void object_update(level_t *level, object_t *obj, f32 dt) {
    // TODO: OTF_EDITACTIVE

    if (obj->type->update_fn) {
        obj->type->update_fn(level, obj);
    }

    if (!(obj->type->flags & OTF_PROJECTILE) && obj->sector) {
        // TODO: formalize gravity
        obj->vel_z -= 1.0f * 16.0f * dt;
    }

    update_move(level, obj, dt);
}

// generic controls
static void update_control(level_t *level, object_t *o) {
    if (!state->allow_input) {
        return;
    }

    /* if (input_get(state->input, "e") & INPUT_PRESS) { */
    /*     pt_interact_resolve_data_t resdata = { level, o }; */
    /*     v2 */
    /*         from = o->pos, */
    /*         to = { */
    /*             from.x + 32.0f * state->render.camera.anglecos, */
    /*             from.y + 32.0f * state->render.camera.anglesin, */
    /*         }; */
    /*     l_path_trace( */
    /*         &state->level, */
    /*         &from, */
    /*         &to, */
    /*         o->type->radius, */
    /*         (f_path_trace_resolve) pt_interact_resolve, */
    /*         &resdata, */
    /*         LPT_ADD_DECALS | LPT_ADD_OBJECTS); */
    /* } */

    const f32
        rotspeed = 8.0f * state->time.dt,
        movespeed = (80.0f - lerp(0.0f, 20.0f, state->aim_factor / 0.2f)) * state->time.dt;

    if (input_get(state->input, "right") & INPUT_DOWN) {
        o->angle += rotspeed;
    }

    if (input_get(state->input, "left") & INPUT_DOWN) {
        o->angle -= rotspeed;
    }

    if (state->mouse_grab) {
        o->angle += 5.0f * (state->input->cursor.motion.x / TARGET_WIDTH);

        state->cam.yaw = o->angle;
        state->cam.pitch +=
            3.0f * (state->input->cursor.motion.y / TARGET_HEIGHT);

        // TODO: standardize in one spot
        state->cam.pitch = clamp(state->cam.pitch, -PI_2 + 0.05f, PI_2 - 0.05f);
        state->cam.yaw = wrap_angle(state->cam.yaw);
    }

    const f32 move_angle = -o->angle;

    vec2s movement = VEC2(0);

    if (input_get(state->input, "up|w") & INPUT_DOWN) {
        movement.x += cos(move_angle);
        movement.y += sin(move_angle);
    }

    if (input_get(state->input, "down|s") & INPUT_DOWN) {
        movement.x -= cos(move_angle);
        movement.y -= sin(move_angle);
    }

    if (input_get(state->input, "a") & INPUT_DOWN) {
        movement.x -= cos(move_angle - PI_2);
        movement.y -= sin(move_angle - PI_2);
    }

    if (input_get(state->input, "d") & INPUT_DOWN) {
        movement.x += cos(move_angle - PI_2);
        movement.y += sin(move_angle - PI_2);
    }

    if (glms_vec2_norm(movement) > 0.001f) {
        movement = glms_vec2_normalize(movement);
        movement = glms_vec2_scale(movement, movespeed);
        o->vel = glms_vec2_add(o->vel, movement);
    }

    // TODO: eye height?
    state->cam.pos = VEC3(o->pos.x, o->pos.y, o->z + 1.35f);
    state->cam.sector = o->sector;
}

// TODO: remove
void act_player(level_t*, actor_t *a, object_t *obj) {
    if (state->mode != GAMEMODE_GAME) { return; }

    /* object_t *o = data->object; */
    /* object_player_t *ex = o->ex; */

    /* if (input_get(state->input, "mouse_left") & INPUT_DOWN) { */
    /*     ex->chargeticks++; */
    /* } else { */
    /*     ex->chargeticks = 0; */    // TODO: bad bad bad! store last hit on object, but in a good way
    if (obj->type_index == OT_BOY) {
        ((object_boy_t*)obj->ex)->hit_tick = state->time.tick;
    }
    /* } */
}

typedef struct {
    object_t *origin;
    bool result;
} sightline_data_t;

static int resolve_sightline(
    level_t *level,
    const path_hit_t *hit,
    vec2s *from,
    vec2s *to,
    sightline_data_t *data) {
    bool stopped = true;

    side_t *portal = hit->wall.side->portal;

    if (portal && portal->sector) {
        side_segment_t segs[4];
        side_get_segments(hit->wall.side, segs);

        // TODO: use a more refined "eye height"
        if (segs[SIDE_SEGMENT_MIDDLE].present
            && data->origin->z + data->origin->type->height
                >= segs[SIDE_SEGMENT_MIDDLE].z0
            && data->origin->z + data->origin->type->height
                <= segs[SIDE_SEGMENT_MIDDLE].z1) {
            stopped = false;
        }
    }

    int res = PATH_TRACE_CONTINUE;
    f32 portal_angle = 0.0f;
    if (path_trace_resolve_portal(
            level, hit, from, to, &res, &portal_angle,
            PATH_TRACE_RESOLVE_PORTAL_NONE)) {
        return res;
    }

    // do not process further collisions
    if (!stopped) {
        return PATH_TRACE_CONTINUE;
    }

    data->result = false;
    return PATH_TRACE_STOP;
}

void act_boy(level_t *level, actor_t *a, object_t *obj) {
    object_boy_t *ex = obj->ex;

    if (ex->health <= 0) {
        object_destroy(level, obj);
        return;
    }

    object_t *target = NULL;
    level_dynlist_each(level->objects, it) {
        object_t *o = *it.el;
        if (o->type_index == OT_PLAYER) {
            target = o;
            break;
        }
    }

    ASSERT(target);

    sightline_data_t sl_data = { .origin = obj, .result = true };
    vec2s from = obj->pos, to = target->pos;
    path_trace(
        level,
        &from,
        &to,
        0.0f,
        (path_trace_resolve_f) resolve_sightline,
        &sl_data,
        PATH_TRACE_NONE);

    if (!sl_data.result && ex->ai_state == AI_STATE_SLEEP) {
        // keep sleeping until we see the target
        return;
    }

    if (sl_data.result) {
        ex->see_tick = state->time.tick;
    }

    // idle if we're too far away from target or it's been a while since we've
    // seen then
    if (glms_vec2_norm(glms_vec2_sub(obj->pos, target->pos)) > 10.0f
        || (state->time.tick - ex->see_tick) > 200) {
        ex->ai_state = AI_STATE_IDLE;
    } else if (ex->ai_state != AI_STATE_WALK) {
        ex->ai_state = AI_STATE_WALK;
        ex->dir_tick = 0;
    }

    if (ex->hit_tick > ex->dir_tick && !sl_data.result) {
        // did we just hit a wall? go in random direction within wall normal
        ex->dir_tick = state->time.tick;

        // TODO: wall move direction should be anywhere in 90 degrees of wall
        // normal
        /* if (ex->lasthit.wall) { */
        /*     const v2 */
        /*         n = l_side_normal(ex->lasthit.side), */
        /*         d = normalize( */
        /*             ((v2) { */
        /*                 rand_f64(&rand, -1.0f, +1.0f), */
        /*                 rand_f64(&rand, -1.0f, +1.0f), */
        /*              })); */
        /*     const f32 s = sign(dot(n, d)); */
        /*     ex->walkdir = (v2) { s * d.x, s * d.y }; */
        /* } else { */
        // walk a random direction
        ex->dir = glms_vec2_normalize(rand_v2(&state->rand, VEC2(-1), VEC2(+1)));
        /* } */
    } else if (ex->ai_state == AI_STATE_IDLE) {
        // random walk direction
        if ((state->time.tick - ex->dir_tick) > 200) {
            ex->dir_tick = state->time.tick;
            ex->dir = glms_vec2_normalize(rand_v2(&state->rand, VEC2(-1), VEC2(+1)));
        }
    } else if (ex->ai_state == AI_STATE_WALK) {
        // path towards player if:
        // - walkdirtick is old (> 2s)
        // - just started walking (walkdirtick == 0)
        // - we hit a wall, but we can see the player
        if (ex->dir_tick == 0
            || (state->time.tick - ex->dir_tick) > 240
            || ((ex->hit_tick > ex->dir_tick) && sl_data.result)) {
            LOG("DOING! %d", ex->hit_tick);
            ex->dir_tick = state->time.tick;
            ex->dir = glms_vec2_normalize(glms_vec2_sub(target->pos, obj->pos));
        }
    }

    obj->vel = glms_vec2_add(obj->vel, glms_vec2_scale(ex->dir, 0.5f));

    if (sl_data.result && ex->cooldown == 0) {
        // TODO: check for melee

        /* const v2 pos_to_target = { */
        /*     target->pos.x - o->pos.x, */
        /*     target->pos.y - o->pos.y */
        /* }; */

        /* // TODO: melee const f32 dist = length(pos_to_target); */

        /* const v2 dir = normalize(pos_to_target); */
        /* object_t *proj = l_new_object(&state->level); */
        /* l_object_set_type(data->level, proj, OT_PROJ2); */
        /* ((object_projectile_t*) proj->ex)->source = o; */
        /* proj->vel.x = 384.0f * dir.x; */
        /* proj->vel.y = 384.0f * dir.y; */
        /* proj->z = o->z + (o->type->height * 0.35f); */
        /* proj->pos = (v2) { 0, 0 }; */
        /* l_object_move( */
        /*     &state->level, */
        /*     proj, */
        /*     (v2) { */
        /*         o->pos.x + (8.0f * dir.x), */
        /*         o->pos.y + (8.0f * dir.y), */
        /*     }); */
        ex->cooldown = 60 + rand_n(&state->rand, -10, 10);
    } else {
        ex->cooldown = max(ex->cooldown - 1, 0);
    }
}

static void update_player(level_t *level, object_t *o) {
    if (state->mode != GAMEMODE_GAME) { return; }
    update_control(level, o);

    /* if (input_get(state->input, "e") & INPUT_PRESS) { */
    /*     pt_interact_resolve_data_t resdata = { level, o }; */
    /*     v2 */
    /*         from = o->pos, */
    /*         to = { */
    /*             from.x + 32.0f * state->render.camera.anglecos, */
    /*             from.y + 32.0f * state->render.camera.anglesin, */
    /*         }; */
    /*     l_path_trace( */
    /*         &state->level, */
    /*         &from, */
    /*         &to, */
    /*         o->type->radius, */
    /*         (f_path_trace_resolve) pt_interact_resolve, */
    /*         &resdata, */
    /*         LPT_ADD_DECALS | LPT_ADD_OBJECTS); */
    /* } */

    /* if (input_get(state->input, "mouse_left") & INPUT_RELEASE) { */
    /*     s_play("STEST0"); */

    /*     // TODO: remove */
    /*     state->render.camera.yaw -= 0.01f; */

    /*     object_t *proj = l_new_object(level); */
    /*     l_object_set_type( */
    /*         level, proj, */
    /*         OT_PROJ1); */
    /*     ((object_projectile_t*) proj->ex)->source = o; */
    /*     proj->vel.x = 1024.0f * fastcos(o->angle); */
    /*     proj->vel.y = 1024.0f * fastsin(o->angle); */
    /*     proj->z = o->z + (EYE_Z * 0.5f); */
    /*     l_object_move( */
    /*         level, */
    /*         proj, */
    /*         (v2) { */
    /*             o->pos.x + (24.0f * fastcos(o->angle)), */
    /*             o->pos.y + (24.0f * fastsin(o->angle)), */
    /*         }); */
    /*     ((object_player_t*) o->ex)->chargeticks = 0; */
    /* } */
}

object_type_t OBJECT_TYPES[OT_COUNT] = {
    [OT_PLACEHOLDER] = {
        .sprite = AS_RESOURCE(TEXTURE_NOTEX),
        .flags = OTF_NONE,
        .height = 1.0f,
        .radius = 1.0f,
    },
    [OT_SPAWN] = {
        .sprite = AS_RESOURCE("XSPAWN"),
        .flags = OTF_NONE,
        .height = 1.0f,
        .radius = 1.0f,
    },
    [OT_EDCAM] = {
        .sprite = AS_RESOURCE(TEXTURE_NOTEX),
        .flags = OTF_EDITACTIVE | OTF_INVISIBLE | OTF_NO_VERSION_BUMP,
        .height = 1.65f,
        .radius = 1.0f,
        /* .actfn = (f_actor) act_edcam, */
        /* .updatefn = (f_object_update) update_edcam, */
    },
    [OT_PROJ1] = {
        .sprite = AS_RESOURCE("OPROJ1"),
        .flags = OTF_PROJECTILE,
        .height = 1.0f,
        .radius = 8.0f,
        /* .actfn = (f_actor) act_projectile, */
        /* .exsize = sizeof(object_projectile_t), */
    },
    [OT_PROJ2] = {
        .sprite = AS_RESOURCE("OPROJ2"),
        .flags = OTF_PROJECTILE,
        .height = 1.0f,
        .radius = 8.0f,
        /* .actfn = (f_actor) act_projectile, */
        /* .exsize = sizeof(object_projectile_t), */
    },
    [OT_BOY] = {
        .sprite = AS_RESOURCE("OBOY0"),
        .height = 1.35f,
        .radius = 0.5f,
        .flags = OTF_NONE,
        .act_fn = (actor_f) act_boy,
        .ex_size = sizeof(object_boy_t),
        .ex_init = &((object_boy_t) {
            .health = 1
        })
    },
    [OT_PLAYER] = {
        .sprite = AS_RESOURCE(TEXTURE_NOTEX),
        .height = 1.65f,
        .radius = 0.25f,
        .flags = OTF_INVISIBLE,
        .act_fn = (actor_f) act_player,
        .update_fn = (object_update_f) update_player,
        /* .exsize = sizeof(object_player_t), */
        /* .exinit = &((object_player_t) { */
        /*     .health = 100 */
        /* }) */
    },
};
