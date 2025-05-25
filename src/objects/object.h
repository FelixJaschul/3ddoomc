#pragma once

#include "../defs.h"
#include <cglm/cglm.h>
#include "triangle.h"
#include "cube.h"

// Forward declarations for types that don't need full definitions
typedef struct plane plane_t;
typedef struct camera camera_t;
typedef struct light light_t;

// Base object structure that all objects inherit from
typedef struct object {
    object_type_index_t type;
    uint32_t flags;
    const object_type_t *type_info;

    // Transform
    vec3 position;
    vec3 rotation;
    vec3 scale;
    mat4 model_matrix;

    // Material properties
    vec3 color;
    float alpha;
    float metallic;
    float roughness;

    // Object-specific data
    union {
        triangle_t triangle;
        cube_t cube;
        // plane_t plane;
        // camera_t camera;
        // light_t light;
    } data;

    // Object methods
    void (*init)(struct object *obj);
    void (*update)(struct object *obj);
    void (*render)(struct object *obj);
    void (*cleanup)(struct object *obj);
} object_t;

// Object management functions
void object_init(object_t *obj, object_type_index_t type);
void object_update(object_t *obj);
void object_render(object_t *obj);
void object_cleanup(object_t *obj);

// Transform helpers
void object_set_position(object_t *obj, vec3 position);
void object_set_rotation(object_t *obj, vec3 rotation);
void object_set_scale(object_t *obj, vec3 scale);
void object_update_model_matrix(object_t *obj);

// Object type registration
void register_object_types(void);
const object_type_t *get_object_type(object_type_index_t type); 
