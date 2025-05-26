#pragma once

#include <cglm/cglm.h>

// Forward declaration
typedef struct object object_t;

typedef struct cube {
    float vertices[108];  // 36 vertices (6 faces * 2 triangles * 3 vertices) * 3 coordinates
    float rotation[3];    // Rotation around each axis
    float scale;         // Uniform scale
    vec3 color;          // RGB color
} cube_t;

// Cube-specific functions
void cube_init(object_t *obj);
void cube_update(object_t *obj);
void cube_render(object_t *obj);
void cube_cleanup(object_t *obj); 
