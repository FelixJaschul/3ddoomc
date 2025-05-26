#include "ui.h"
#include "imgui.h"
#include "../state.h"
#include "../objects/object.h"
#include "../objects/triangle.h"
#include "../objects/cube.h"
#include <cglm/cglm.h>
#include <stdio.h>

void draw_ui_debug(state_t *state) {
	if (!state->show_debug_menu) return;

	igBegin("Debug Menu", &state->show_debug_menu, ImGuiWindowFlags_None);

	if (igTreeNode_Str("Editor")) {
		if (state->editor) {
			igText("Level: %s", state->editor->levelpath);
		}
		igTreePop();
	}

	if (igTreeNode_Str("Camera")) {
		igText("Position:");
		igText("X: %.3f Y: %.3f Z: %.3f", 
			state->cam.pos[0], 
			state->cam.pos[1], 
			state->cam.pos[2]);
		igTreePop();
	}

	if (igTreeNode_Str("Objects")) {
		if (igTreeNode_Str("Triangle")) {
			// Color
			igColorEdit3("Color", state->objects.triangle.color, ImGuiColorEditFlags_None);

			// Vertices
			if (igTreeNode_Str("Vertices")) {
				for (int i = 0; i < 3; i++) {
					char label[32];
					snprintf(label, sizeof(label), "Vertex %d", i);
					if (igDragFloat3(label, &state->objects.triangle.vertices[i * 3], 0.01f, -1.0f, 1.0f, "%.2f", ImGuiSliderFlags_None)) {
						sg_update_buffer(state->gfx.triangle_vbuf, &(sg_range){
							.ptr = state->objects.triangle.vertices,
							.size = sizeof(state->objects.triangle.vertices)
						});
					}
				}
				igTreePop();
			}

			// Rotation
			float rotation_deg = glm_deg(state->objects.triangle.rotation);
			if (igSliderFloat("Rotation", &rotation_deg, 0.0f, 360.0f, "%.1f", ImGuiSliderFlags_None)) {
				state->objects.triangle.rotation = glm_rad(rotation_deg);
			}

			// Scale
			igSliderFloat("Scale", &state->objects.triangle.scale, 0.1f, 2.0f, "%.2f", ImGuiSliderFlags_None);

			igTreePop();
		}

		if (igTreeNode_Str("Cube")) {
			// Color
			igColorEdit3("Color", state->objects.cube.color, ImGuiColorEditFlags_None);

			// Rotation
			if (igDragFloat3("Rotation", state->objects.cube.rotation, 0.1f, 0.0f, 360.0f, "%.1f", ImGuiSliderFlags_None)) {
				// Update is handled in cube_update
			}

			// Scale
			igSliderFloat("Scale", &state->objects.cube.scale, 0.1f, 2.0f, "%.2f", ImGuiSliderFlags_None);

			igTreePop();
		}

		igTreePop();
	}

	igEnd();
} 
