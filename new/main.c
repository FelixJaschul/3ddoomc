#include <stdio.h>
#include <stdbool.h>
#include <cglm/cglm.h>

#include "imgui.h"
#include "gfx/gfx.h"

#ifdef EMSCRIPTEN
#   define SOKOL_GLES3
#else
#   define SOKOL_GLCORE33
#endif

#include <sokol_gfx.h>
#include <sokol_gp.h>
#include <util/sokol_gfx_imgui.h>
#include <util/sokol_gl.h>

#define SOKOL_IMGUI_NO_SOKOL_APP
#include <util/sokol_imgui.h>

#include "sdl2.h"
#include "shaders/shader.h"
#include "state.h"
#include "config.h"
#include "objects/object.h"
#include "objects/triangle.h"
#include "objects/cube.h"
#include "editor/ui.h"

#define GL_SILENCE_DEPRECATION
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>

// Forward declarations
static bool init(void);
static void deinit(void);

static void slog_func(const char* tag, uint32_t log_level, uint32_t log_item, const char* message, uint32_t line_nr, const char* filename, void* user_data) {
	fprintf(stderr, "[%s] %s\n", tag, message);
}

static bool init(void) {
	state = calloc(1, sizeof(state_t));

	if (!state) {
		fprintf(stderr, "Failed to allocate state\n"); 
		return false;
	}

	// Initialize editor
	state->editor = calloc(1, sizeof(editor_t));
	if (!state->editor) {
		fprintf(stderr, "Failed to allocate editor\n");
		return false;
	}
	strncpy(state->editor->levelpath, "untitled", sizeof(state->editor->levelpath) - 1);
	state->editor->is_active = true;
	state->editor->show_grid = true;
	state->editor->show_axes = true;
	state->editor->show_bounds = true;

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError()); 
		return false;
	}

	// openGL 3.3
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	
	SDL_GL_SetAttribute(
			SDL_GL_CONTEXT_PROFILE_MASK,
			SDL_GL_CONTEXT_PROFILE_CORE);

	state->window =
		SDL_CreateWindow(
				"3D Editor",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				1250, 720,
				SDL_WINDOW_OPENGL |
				SDL_WINDOW_RESIZABLE);

	if (!state->window) {
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError()); 
		return false;
	}

	state->gl_ctx = SDL_GL_CreateContext(state->window);
	if (!state->gl_ctx) {
		fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError()); 
		return false;
	}

	if (SDL_GL_MakeCurrent(state->window, state->gl_ctx) != 0) {
		fprintf(stderr, "SDL_GL_MakeCurrent failed: %s\n", SDL_GetError()); 
		return false;
	}

	// create and set sokol-gfx state
	state->sg_state = sg_create_state();
	sg_set_state(state->sg_state);

	// init sokol-gfx
	sg_desc desc = {
		.context = {
			.color_format = SG_PIXELFORMAT_RGBA8,
			.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
			.sample_count = 1,
		},
		.logger.func = slog_func,
	};
	sg_setup(&desc);

	// create and set sokol-gp state
	state->sgp_state = sgp_create_state();
	sgp_set_state(state->sgp_state);

	// init sokol-gp
	sgp_desc sgp_desc = {
		.max_vertices = 64 * 1024,
		.max_commands = 16 * 1024,
	};
	sgp_setup(&sgp_desc);

	// init ImGui
	state->imgui_ctx = igCreateContext(NULL);
	ImGuiIO* io = igGetIO();
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	state->WIDTH = DEFAULT_WIDTH;
	state->HEIGHT = DEFAULT_HEIGHT;

	// set initial display size
	SDL_GetWindowSize(state->window, &state->WIDTH, &state->HEIGHT);
	io->DisplaySize.x = (float)state->WIDTH;
	io->DisplaySize.y = (float)state->HEIGHT;

	ImGuiStyle* style = igGetStyle();
	igStyleColorsDark(style);

	// create and set sokol-imgui state
	state->simgui_state = simgui_create_state();
	simgui_set_state(state->simgui_state);

	// initialize sokol-imgui
	simgui_desc_t simgui_desc = {
		.max_vertices = 32 * 1024,
	};
	simgui_setup(&simgui_desc);

	// Initialize object system
	register_object_types();

	// Create triangle object
	object_t triangle_obj;
	object_init(&triangle_obj, OT_TRIANGLE);
	state->objects.triangle = triangle_obj.data.triangle;

	// Create cube object
	object_t cube_obj;
	object_init(&cube_obj, OT_CUBE);
	state->objects.cube = cube_obj.data.cube;

	// sokol-gfx resources
	sg_buffer_desc triangle_vbuf_desc = {
		.size = sizeof(float) * 9,  // 3 vertices * 3 coordinates
		.usage = SG_USAGE_DYNAMIC
	};
	state->gfx.triangle_vbuf = sg_make_buffer(&triangle_vbuf_desc);

	sg_buffer_desc cube_vbuf_desc = {
		.size = sizeof(float) * 108,  // 36 vertices * 3 coordinates
		.usage = SG_USAGE_DYNAMIC
	};
	state->gfx.cube_vbuf = sg_make_buffer(&cube_vbuf_desc);

	sg_shader_desc shd_desc = {
		.vs = {
			.source = shader_load_file("src/shaders/triangle.vert"),
			.uniform_blocks = { // declare uniform blocks
				[0] = { // uniform block for color, model, view, projection
					.size = sizeof(vec4) + sizeof(mat4) * 3, // size for 1 vec4 (padded vec3) + 3 mat4s
					.layout = SG_UNIFORMLAYOUT_STD140,
					.uniforms = {
						[0] = { .name = "color", .type = SG_UNIFORMTYPE_FLOAT3 }, // vec3 aligns to vec4 in std140
						[1] = { .name = "model", .type = SG_UNIFORMTYPE_MAT4 },
						[2] = { .name = "view", .type = SG_UNIFORMTYPE_MAT4 },
						[3] = { .name = "projection", .type = SG_UNIFORMTYPE_MAT4 }
					}
				}
			}
		},
		.fs = {
			.source = shader_load_file("src/shaders/triangle.frag")
		}
	};
	state->gfx.shd = sg_make_shader(&shd_desc);

	sg_pipeline_desc pip_desc = {
		.shader = state->gfx.shd,
		.layout = {
			.attrs = {
				[0] = { .format = SG_VERTEXFORMAT_FLOAT3 }
			}
		}
	};
	state->gfx.pip = sg_make_pipeline(&pip_desc);

	state->show_debug_menu = true;
	state->show_object_properties = false;

	// init model Matrix
	glm_mat4_identity(state->model_matrix);

	state->quit = false;

	return true;
}

static void deinit(void) {
	if (!state) return;

	// cleanup
	sg_destroy_buffer(state->gfx.triangle_vbuf);
	sg_destroy_buffer(state->gfx.cube_vbuf);
	sg_destroy_shader(state->gfx.shd);
	sg_destroy_pipeline(state->gfx.pip);

	// cleanup
	simgui_shutdown();
	sgp_shutdown();
	sg_shutdown();

	// more cleanup
	SDL_GL_DeleteContext(state->gl_ctx);
	SDL_DestroyWindow(state->window);
	SDL_Quit();

	// free all
	free(state);
	state = NULL;
}

int main(void) {
	if (!init()) {
		return -1;
	}

	while (!state->quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			// imgui does
			imgui_impl_process_sdl_event(&event);

			switch (event.type) {
			case SDL_QUIT:
				state->quit = true;
				break;

			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					state->quit = true;
				}
				break;

			case SDL_KEYUP:
				break;

			case SDL_MOUSEMOTION:
				break;

			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button >= 0 && event.button.button < 3) {
					igGetIO()->MouseDown[event.button.button] = true;
				}
				
				if (igGetIO()->WantCaptureMouse) {
					// done by imgui
				} else {
					if (event.button.button == SDL_BUTTON_RIGHT) {
						state->show_object_properties = !state->show_object_properties;
					}
				}
				break;

			case SDL_MOUSEBUTTONUP:
				if (event.button.button >= 0 && event.button.button < 3) {
					igGetIO()->MouseDown[event.button.button] = false;
				}
				break;

			case SDL_MOUSEWHEEL:
				break;

			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					int width, height;
					SDL_GetWindowSize(state->window, &width, &height);
					state->WIDTH = width;
					state->HEIGHT = height;
				}
				break;

			default: break;
			}
		}

		simgui_new_frame(&(simgui_frame_desc_t){
			.width = state->WIDTH,
			.height = state->HEIGHT,
			.delta_time = 1.0f / 60.0f,
			.dpi_scale = 1.0f
		});

		draw_ui_debug(state);

		// render frame
		sg_pass_action pass_action = {
			.colors[0] = { .action = SG_ACTION_CLEAR, .value = { 0.2f, 0.25f, 0.3f, 1.0f } }
		};

		sg_begin_default_pass(&pass_action, state->WIDTH, state->HEIGHT);
		
		// Update camera
		vec3 camera_pos = {0.0f, 0.0f, 2.0f};
		vec3 camera_target = {0.0f, 0.0f, 0.0f};
		vec3 camera_up = {0.0f, 1.0f, 0.0f};
		glm_lookat(camera_pos, camera_target, camera_up, state->view_matrix);

		float aspect_ratio = (float)state->WIDTH / (float)state->HEIGHT;
		glm_perspective(glm_rad(45.0f), aspect_ratio, 0.1f, 100.0f, state->projection_matrix);

		// Apply pipeline
		sg_apply_pipeline(state->gfx.pip);

		// Update and render triangle
		object_t triangle_obj;
		triangle_obj.data.triangle = state->objects.triangle;
		triangle_obj.type = OT_TRIANGLE;
		triangle_obj.type_info = get_object_type(OT_TRIANGLE);
		triangle_obj.init = triangle_init;
		triangle_obj.update = triangle_update;
		triangle_obj.render = triangle_render;
		triangle_obj.cleanup = triangle_cleanup;
		
		// Copy state from state->objects.triangle
		glm_vec3_copy(state->objects.triangle.color, triangle_obj.color);
		// Position triangle slightly to the left
		vec3 triangle_pos = {-0.5f, 0.0f, 0.0f};
		object_set_position(&triangle_obj, triangle_pos);
		
		object_update(&triangle_obj);
		object_render(&triangle_obj);
		
		// Update and render cube
		object_t cube_obj;
		cube_obj.data.cube = state->objects.cube;
		cube_obj.type = OT_CUBE;
		cube_obj.type_info = get_object_type(OT_CUBE);
		cube_obj.init = cube_init;
		cube_obj.update = cube_update;
		cube_obj.render = cube_render;
		cube_obj.cleanup = cube_cleanup;
		
		// Copy state from state->objects.cube
		glm_vec3_copy(state->objects.cube.color, cube_obj.color);
		// Position cube slightly to the right
		vec3 cube_pos = {0.5f, 0.0f, 0.0f};
		object_set_position(&cube_obj, cube_pos);
		
		object_update(&cube_obj);
		object_render(&cube_obj);
		
		// Update state with any changes
		state->objects.triangle = triangle_obj.data.triangle;
		state->objects.cube = cube_obj.data.cube;

		simgui_render();
		sg_end_pass();
		sg_commit();

		SDL_GL_SwapWindow(state->window);
	}

	deinit();

	return 0;
}
