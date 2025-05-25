#include "sdl2.h"
#include <glad/glad.h>
#include <stdio.h>
#include <stdbool.h>
#include <cglm/cglm.h>

#include "imgui.h"

#include "shaders/shader.h"
#include "state.h"

static bool init() {
	// allocate state on heap
	state = calloc(1, sizeof(state_t));

	if (!state) {
		fprintf(stderr, "Failed to allocate state\n"); goto failed;
	}

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError()); goto failed;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	
	SDL_GL_SetAttribute(
			SDL_GL_CONTEXT_PROFILE_MASK, 
			SDL_GL_CONTEXT_PROFILE_CORE);

	state->window = 
		SDL_CreateWindow(
				"Test", 
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				1250, 720,
				SDL_WINDOW_OPENGL | 
				SDL_WINDOW_RESIZABLE);

	if (!state->window) {
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError()); goto failed;
	}

	state->gl_ctx = SDL_GL_CreateContext(state->window);
	if (!state->gl_ctx) {
		fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError()); goto failed;
	}
	
	// load open gl
	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		fprintf(stderr, "Failed to initialize glad\n"); goto failed;
	}

	// Setup ImGui context
	state->imgui_ctx = igCreateContext(NULL);
	ImGuiIO* io = igGetIO();
	
	// Set initial display size
	int width, height;
	SDL_GetWindowSize(state->window, &width, &height);
	io->DisplaySize.x = (float)width;
	io->DisplaySize.y = (float)height;

	// Initialize ImGui style
	ImGuiStyle* style = igGetStyle();
	igStyleColorsDark(style);

	// Initialize font atlas
	unsigned char* pixels;
	int font_width, font_height;
	ImFontAtlas* fonts = io->Fonts;
	ImFontAtlas_GetTexDataAsRGBA32(fonts, &pixels, &font_width, &font_height, NULL);

	// Create OpenGL texture for font atlas
	GLuint font_texture;
	glGenTextures(1, &font_texture);
	glBindTexture(GL_TEXTURE_2D, font_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, font_width, font_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	ImFontAtlas_SetTexID(fonts, (void*)(intptr_t)font_texture);

	// state view
	state->color[0] = 1.0f;
	state->color[1] = 0.0f;
	state->color[2] = 0.0f;

	// prepare triangle vertex data 
	float vertices[] = {
		0.0f,  0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f
	};

	glGenVertexArrays(1, &state->vao);
	glGenBuffers(1, &state->vbo);

	glBindVertexArray(state->vao);
	glBindBuffer(GL_ARRAY_BUFFER, state->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// load shader
	state->shader_program = shader_load_program(
			"src/shaders/triangle.vert", 
			"src/shaders/triangle.frag");

	if (!state->shader_program) {
		fprintf(stderr, "Failed to load shader program\n"); goto failed;
 	}

	state->quit = false;

	return true;

failed:
	return false;
}

static void deinit() {
	if (!state) return;

	glDeleteBuffers(1, &state->vbo);
	glDeleteVertexArrays(1, &state->vao);
	glDeleteProgram(state->shader_program);

	igDestroyContext(state->imgui_ctx);

	SDL_GL_DeleteContext(state->gl_ctx);
	SDL_DestroyWindow(state->window);
	SDL_Quit();

	free(state);
	state = NULL;
}

int main() {
	if (!init()) {
		return -1;
	}

	while (!state->quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
		if (!imgui_impl_process_sdl_event(&event)) {
			switch (event.type) {
			case SDL_QUIT: 
				state->quit = true;
				break;

			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					state->quit = true;
				}
				break;

			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					int width, height;
					SDL_GetWindowSize(state->window, &width, &height);
					glViewport(0, 0, width, height);
				}
				break;
			}
		}
		}

		igNewFrame();

		// imGui window
		{
			igSetNextWindowPos((ImVec2){800 - 220, 10}, ImGuiCond_Always, (ImVec2){0, 0});
			igSetNextWindowSize((ImVec2){210, 110}, ImGuiCond_Always);

			igBegin("Triangle Color", 
				NULL, ImGuiWindowFlags_NoResize | 
				ImGuiWindowFlags_NoCollapse | 
				ImGuiWindowFlags_NoMove);

			igColorEdit3("Color", state->color, ImGuiColorEditFlags_None);

			igEnd();
		}

		igRender();

		glViewport(0, 0, 800, 600);
		glClearColor(0.2f, 0.25f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// render triangle
		glUseProgram(state->shader_program);

		// set uniform for color
		GLint color_loc = glGetUniformLocation(state->shader_program, "uColor");
		glUniform3fv(color_loc, 1, state->color);

		glBindVertexArray(state->vao);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindVertexArray(0);

		SDL_GL_SwapWindow(state->window);
    }

    deinit();

    return 0;
}
