#pragma once

#include <cglm/cglm.h>
#include <stdbool.h>

typedef struct {

	vec3 vertices[3];
	vec3 color;
	float rotation;
	float scale;
	bool selected;

} triangle_t;

void init_triangle(triangle_t* triangle); 
