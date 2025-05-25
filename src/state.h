#pragma once

#include <SDL.h>
#define SOKOL_IMGUI_NO_SOKOL_APP
#include <sokol_gfx.h>
#include <sokol_gp.h>
#include <util/sokol_gfx_imgui.h>
#include <util/sokol_gl.h>
#include <util/sokol_imgui.h>
#include <cglm/cglm.h>
#include "defs.h"
#include "editor/editor.h"
#include "objects/object.h"

// Forward declarations
typedef struct camera camera_t;
typedef struct light light_t;

typedef struct state {
	// Window and context
	SDL_Window *window;
	SDL_GLContext gl_ctx;
	int WIDTH;
	int HEIGHT;
	bool quit;

	// Sokol states
	void *sg_state;
	void *sgp_state;
	void *simgui_state;

	// ImGui context
	void *imgui_ctx;

	// Editor state
	editor_t *editor;

	// Camera
	struct {
		vec3 pos;
		vec3 target;
		vec3 up;
	} cam;

	// Graphics resources
	struct {
		sg_buffer triangle_vbuf;  // Vertex buffer for triangle
		sg_buffer cube_vbuf;      // Vertex buffer for cube
		sg_shader shd;
		sg_pipeline pip;
	} gfx;

	// Matrices
	mat4 model_matrix;
	mat4 view_matrix;
	mat4 projection_matrix;

	// UI state
	bool show_debug_menu;
	bool show_object_properties;

	// Objects
	struct {
		triangle_t triangle;
		cube_t cube;
	} objects;

} state_t;

// Global state
extern state_t *state;
