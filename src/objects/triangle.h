#pragma once

#include <cglm/cglm.h>
#include <stdbool.h>

// Define the triangle_t struct
typedef struct {
	vec3 vertices[3];  // Triangle vertices
	vec3 color;        // Triangle color
	float rotation;    // Rotation in radians
	float scale;       // Scale factor
	bool selected;     // Whether this object is selected
} triangle_t;

// Function to initialize a triangle
void init_triangle(triangle_t* triangle); 