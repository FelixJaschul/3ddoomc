#pragma once

#include <cglm/cglm.h>

// Forward declaration
typedef struct object object_t;

typedef struct triangle {
	float vertices[9];  // 3 vertices, each with x,y,z coordinates
	float rotation;
	float scale;
	vec3 color;        // RGB color
} triangle_t;

// Triangle-specific functions
void triangle_init(object_t *obj);
void triangle_update(object_t *obj);
void triangle_render(object_t *obj);
void triangle_cleanup(object_t *obj); 
