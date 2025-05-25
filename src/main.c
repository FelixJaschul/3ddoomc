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
#include "objects/triangle.h"
#include "editor/ui.h"

#define GL_SILENCE_DEPRECATION
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>

static void slog_func(const char* tag, uint32_t log_level, uint32_t log_item, const char* message, uint32_t line_nr, const char* filename, void* user_data) {
	fprintf(stderr, "[%s] %s\n", tag, message);
}

static bool init() {
	state = calloc(1, sizeof(state_t));

	if (!state) {
		fprintf(stderr, "Failed to allocate state\n"); goto failed;
	}

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError()); goto failed;
	}

	// OpenGL 3.3
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	
	SDL_GL_SetAttribute(
			SDL_GL_CONTEXT_PROFILE_MASK,
			SDL_GL_CONTEXT_PROFILE_CORE);

	state->window =
		SDL_CreateWindow(
				"TEST",
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

	// Make sure we have a valid OpenGL context
	if (SDL_GL_MakeCurrent(state->window, state->gl_ctx) != 0) {
		fprintf(stderr, "SDL_GL_MakeCurrent failed: %s\n", SDL_GetError()); goto failed;
	}

	// Create and set sokol-gfx state
	state->sg_state = sg_create_state();
	sg_set_state(state->sg_state);

	// Initialize sokol-gfx
	sg_desc desc = {
		.context = {
			.color_format = SG_PIXELFORMAT_RGBA8,
			.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
			.sample_count = 1,
		},
		.logger.func = slog_func,
	};
	sg_setup(&desc);

	// Create and set sokol-gp state
	state->sgp_state = sgp_create_state();
	sgp_set_state(state->sgp_state);

	// Initialize sokol-gp
	sgp_desc sgp_desc = {
		.max_vertices = 64 * 1024,
		.max_commands = 16 * 1024,
	};
	sgp_setup(&sgp_desc);

	// Initialize ImGui
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

	// Create and set sokol-imgui state
	state->simgui_state = simgui_create_state();
	simgui_set_state(state->simgui_state);

	// Initialize sokol-imgui
	simgui_desc_t simgui_desc = {
		.max_vertices = 32 * 1024,
	};
	simgui_setup(&simgui_desc);

	// state view
	state->color[0] = 1.0f;
	state->color[1] = 0.0f;
	state->color[2] = 0.0f;

	// Create sokol-gfx resources
	sg_buffer_desc vbuf_desc = {
		.size = sizeof(float) * 3 * 3, // Size of 3 vec3 vertices
		.usage = SG_USAGE_DYNAMIC // Usage is dynamic
	};
	state->gfx.vbuf = sg_make_buffer(&vbuf_desc);

	sg_shader_desc shd_desc = {
		.vs = {
			.source = shader_load_file("src/shaders/triangle.vert"),
			.uniform_blocks = { // Declare uniform blocks
				[0] = { // Uniform block for color, model, view, projection
					.size = sizeof(vec4) + sizeof(mat4) * 3, // Size for 1 vec4 (padded vec3) + 3 mat4s
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

	// Initialize triangle using the new function
	init_triangle(&state->triangle);
	
	state->show_debug_menu = true;
	state->show_object_properties = false;

	// Initialize model matrix to identity
	glm_mat4_identity(state->model_matrix);

	state->quit = false;

	return true;

failed:
	return false;
}

static void deinit() {
	if (!state) return;

	// Destroy sokol-gfx resources first
	sg_destroy_buffer(state->gfx.vbuf);
	sg_destroy_shader(state->gfx.shd);
	sg_destroy_pipeline(state->gfx.pip);

	// Shutdown sokol systems in reverse order of initialization
	// Note: simgui_shutdown() will destroy the ImGui context
	simgui_shutdown();
	sgp_shutdown();
	sg_shutdown();

	// Clean up SDL
	SDL_GL_DeleteContext(state->gl_ctx);
	SDL_DestroyWindow(state->window);
	SDL_Quit();

	// Free state last
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
			// Pass all events to ImGui first
			imgui_impl_process_sdl_event(&event);

			switch (event.type) {
			case SDL_QUIT:
				state->quit = true;
				break;

			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					state->quit = true;
				}
				break; // ImGui event handling is done by imgui_impl_process_sdl_event

			case SDL_KEYUP:
				break; // ImGui event handling is done by imgui_impl_process_sdl_event

			case SDL_MOUSEMOTION:
				break; // ImGui event handling is done by imgui_impl_process_sdl_event

			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button >= 0 && event.button.button < 3) {
					igGetIO()->MouseDown[event.button.button] = true;
				}
				
				if (igGetIO()->WantCaptureMouse) {
					// ImGui is using the mouse, don't process for game objects
				} else {
					// Simplified mouse handling - no 3D picking for now
					// ImVec2 mouse_click_pos = { (float)event.button.x, (float)event.button.y };
					// bool hit_triangle = false;
					// ... removed 3D to 2D projection and hit test ...

					// For now, right-click anywhere outside ImGui toggles object properties popup for the triangle
					if (event.button.button == SDL_BUTTON_RIGHT) {
						state->show_object_properties = !state->show_object_properties; // Toggle popup
					}
				} // Middle mouse button is handled by imgui_impl_process_sdl_event
				break;

			case SDL_MOUSEBUTTONUP:
				if (event.button.button >= 0 && event.button.button < 3) {
					igGetIO()->MouseDown[event.button.button] = false;
				}
				break; // ImGui event handling is done by imgui_impl_process_sdl_event

			case SDL_MOUSEWHEEL:
				break; // ImGui event handling is done by imgui_impl_process_sdl_event

			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					int width, height;
					SDL_GetWindowSize(state->window, &width, &height);
					state->WIDTH = width;
					state->HEIGHT = height;
				}
				break; // ImGui event handling is done by imgui_impl_process_sdl_event for WindowEvent_Size_Changed

			default: break;
			}
		}

		simgui_new_frame(&(simgui_frame_desc_t){
			.width = state->WIDTH,
			.height = state->HEIGHT,
			.delta_time = 1.0f / 60.0f,
			.dpi_scale = 1.0f
		});

		// Draw all debug UI windows
		draw_ui_debug(state);

		// Render frame
		sg_pass_action pass_action = {
			.colors[0] = { .action = SG_ACTION_CLEAR, .value = { 0.2f, 0.25f, 0.3f, 1.0f } }
		};

		sg_begin_default_pass(&pass_action, state->WIDTH, state->HEIGHT);
		
		// Calculate Model, View, Projection matrices
		mat4 identity = GLM_MAT4_IDENTITY_INIT;

		// Model matrix (translate, rotate, scale the triangle)
		glm_mat4_copy(identity, state->model_matrix);
		// For simplicity, let's assume triangle's origin is at (0,0,0) in world space
		// If triangle_t had a position, we would translate here: glm_translate(state->model_matrix, state->triangle.position);

		// Apply rotation from state->triangle.rotation
		glm_rotate(state->model_matrix, state->triangle.rotation, (vec3){0.0f, 0.0f, 1.0f}); // Rotate around Z axis

		// Apply scale from state->triangle.scale
		glm_scale(state->model_matrix, (vec3){state->triangle.scale, state->triangle.scale, state->triangle.scale});

		// View matrix (camera)
		// For a simple setup, let's position the camera looking towards the origin
		vec3 camera_pos = {0.0f, 0.0f, 2.0f}; // Camera position
		vec3 camera_target = {0.0f, 0.0f, 0.0f}; // Point camera is looking at
		vec3 camera_up = {0.0f, 1.0f, 0.0f}; // Up direction
		glm_lookat(camera_pos, camera_target, camera_up, state->view_matrix);

		// Projection matrix
		float aspect_ratio = (float)state->WIDTH / (float)state->HEIGHT;
		glm_perspective(glm_rad(45.0f), aspect_ratio, 0.1f, 100.0f, state->projection_matrix); // 45 degree FOV, aspect ratio, near/far plane

		// Update vertex buffer with current triangle data
		sg_update_buffer(state->gfx.vbuf, &(sg_range){.ptr = state->triangle.vertices, .size = sizeof(state->triangle.vertices)});

		// Draw triangle
		sg_apply_pipeline(state->gfx.pip);
		sg_apply_bindings(&(sg_bindings){
			.vertex_buffers[0] = state->gfx.vbuf
		});
		// Apply uniforms
		// Apply all uniforms in one block at slot 0
		struct { // C struct matching the uniform block layout
			vec3 color;
			float _pad1; // Padding to align vec3 to vec4
			mat4 model;
			mat4 view;
			mat4 projection;
		} vs_uniforms;

		glm_vec3_copy(state->triangle.color, vs_uniforms.color);
		glm_mat4_copy(state->model_matrix, vs_uniforms.model);
		glm_mat4_copy(state->view_matrix, vs_uniforms.view);
		glm_mat4_copy(state->projection_matrix, vs_uniforms.projection);

		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &(sg_range){ // Apply the entire struct as the uniform block
			.ptr = &vs_uniforms,
			.size = sizeof(vs_uniforms)
		});

		sg_draw(0, 3, 1);

		simgui_render();
		sg_end_pass();
		sg_commit();

		SDL_GL_SwapWindow(state->window);
	}

	deinit();

	return 0;
}
