#include "editor/ui.h"
#include "imgui.h"
#include <cglm/cglm.h>
#include <stdio.h>

void draw_ui_debug(state_t* state) {
	ImGuiIO* io = igGetIO();

	if (state->show_debug_menu) {

		ImVec2 status_window_pos;

		float status_window_width = 300.0f;
		float padding = 10.0f;
		status_window_pos.x = (float)state->WIDTH - status_window_width - padding;
		status_window_pos.y = padding;
		igSetNextWindowPos(status_window_pos, ImGuiCond_Always, (ImVec2){0, 0});

		igBegin("STATUS", &state->show_debug_menu, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

		igText("FILE");
		igText("%s", state->editor->levelpath);

		igSeparator();

		igText("FPS: %.2f", io->Framerate);

		igText("Window Size: %d x %d", state->WIDTH, state->HEIGHT);

		igText("CURSOR: (%.0f, %.0f)", io->MousePos.x, io->MousePos.y);

		igText("CAMERA POS");
		igText("X: %.3f Y: %.3f Z: %.3f", state->cam.pos.x, state->cam.pos.y, state->cam.pos.z);

		igEnd();
	}

	if (state->show_object_properties) {
		igSetNextWindowSize((ImVec2){300, 400}, ImGuiCond_Appearing);
		igBegin("Object Properties", &state->show_object_properties, ImGuiWindowFlags_None);

		igColorEdit3("Color", state->triangle.color, ImGuiColorEditFlags_None);

		for (int i = 0; i < 3; i++) {
			char label[32];
			snprintf(label, sizeof(label), "Vertex %d", i + 1);
			if (igTreeNode_Str(label)) {
				if (igDragFloat3("Position", state->triangle.vertices[i], 0.01f, -1.0f, 1.0f, "%.2f", ImGuiSliderFlags_None)) {
					sg_update_buffer(state->gfx.vbuf, &(sg_range){.ptr = state->triangle.vertices, .size = sizeof(state->triangle.vertices)}); // Update the whole buffer
				}
				igTreePop();
			}
		}

		float rotation_deg = glm_deg(state->triangle.rotation);
		if (igSliderFloat("Rotation", &rotation_deg, 0.0f, 360.0f, "%.1f deg", ImGuiSliderFlags_None)) {
			state->triangle.rotation = glm_rad(rotation_deg);
		}

		igSliderFloat("Scale", &state->triangle.scale, 0.1f, 2.0f, "%.2f", ImGuiSliderFlags_None);

		igEnd();
	}
} 
