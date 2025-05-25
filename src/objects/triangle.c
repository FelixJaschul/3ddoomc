#include "objects/triangle.h"
#include <cglm/cglm.h>

void init_triangle(triangle_t* triangle) {
	triangle->vertices[0][0] = 0.0f;
	triangle->vertices[0][1] = 0.5f;
	triangle->vertices[0][2] = 0.0f;

	triangle->vertices[1][0] = -0.5f;
	triangle->vertices[1][1] = -0.5f;
	triangle->vertices[1][2] = 0.0f;

	triangle->vertices[2][0] = 0.5f;
	triangle->vertices[2][1] = -0.5f;
	triangle->vertices[2][2] = 0.0f;

	triangle->color[0] = 1.0f;
	triangle->color[1] = 0.0f;
	triangle->color[2] = 0.0f;

	triangle->rotation = 0.0f;
	triangle->scale = 1.0f;
	triangle->selected = false;
} 
