#pragma once

#include <glad/glad.h>
#include <cglm/cglm.h>
#include "imgui.h"

typedef struct state {
	struct SDL_Window *window;
	struct SDL_GLContext *gl_ctx;
	ImGuiContext *imgui_ctx;

	struct {
		vec3 pos, dir, up, right;
	} cam;

	GLuint vao, vbo;
	GLuint shader_program;

	bool quit;

	float color[3];
} state_t;

extern state_t *state;
