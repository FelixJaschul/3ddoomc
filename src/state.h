#pragma once

#include "sdl2.h"
#include "imgui.h"
#include "gfx/gfx.h"
#include <cglm/cglm.h>
#include "objects/triangle.h"

typedef struct ImGuiContext ImGuiContext;

typedef struct {
	SDL_Window* window;
	SDL_GLContext gl_ctx;
	ImGuiContext* imgui_ctx;

	int WIDTH, HEIGHT;

	bool quit;
	float color[3];

	// sokol-gfx resources
	gfx_resources_t gfx;
	void *sg_state;  // sokol-gfx state
	void *sgp_state; // sokol-gp state
	void *simgui_state; // sokol-imgui state

	// Object state
	triangle_t triangle;
	bool show_debug_menu;
	bool show_object_properties;

	// Matrices for rendering
	mat4 model_matrix;
	mat4 view_matrix;
	mat4 projection_matrix;

} state_t;

extern state_t* state;
