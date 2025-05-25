#include "editor/ui.h"
#include "imgui.h"
#include <cglm/cglm.h>
#include <stdio.h> // For snprintf

void draw_ui_debug(state_t* state) {
	ImGuiIO* io = igGetIO(); // Get IO state once per frame

	// Debug menu (Status window)
	if (state->show_debug_menu) {
		// Set position for the Status window on the right
		ImVec2 status_window_pos;
		// Estimate a width for the status window or adjust based on content
		float status_window_width = 300.0f; // Estimated width
		float padding = 10.0f;
		status_window_pos.x = (float)state->WIDTH - status_window_width - padding;
		status_window_pos.y = padding;
		igSetNextWindowPos(status_window_pos, ImGuiCond_Always, (ImVec2){0, 0});

		igBegin("Status", &state->show_debug_menu, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);

		// General Status Info
		igText("Application Status");
		igSeparator();
		igText("FPS: %.2f", io->Framerate);
		igText("Window Size: %d x %d", state->WIDTH, state->HEIGHT);
		// Add other general info here later if needed

		igEnd();
	}

	// Object properties popup
	if (state->show_object_properties) {
		igSetNextWindowSize((ImVec2){300, 400}, ImGuiCond_Appearing); // Set size for the popup
		igBegin("Object Properties", &state->show_object_properties, ImGuiWindowFlags_None);

		// Color picker
		igColorEdit3("Color", state->triangle.color, ImGuiColorEditFlags_None);

		// Position controls for each vertex
		for (int i = 0; i < 3; i++) {
			char label[32];
			snprintf(label, sizeof(label), "Vertex %d", i + 1);
			if (igTreeNode_Str(label)) {
				if (igDragFloat3("Position", state->triangle.vertices[i], 0.01f, -1.0f, 1.0f, "%.2f", ImGuiSliderFlags_None)) {
					// Update vertex buffer after changing position
					sg_update_buffer(state->gfx.vbuf, &(sg_range){.ptr = state->triangle.vertices, .size = sizeof(state->triangle.vertices)}); // Update the whole buffer
				}
				igTreePop();
			}
		}

		// Rotation control
		float rotation_deg = glm_deg(state->triangle.rotation);
		if (igSliderFloat("Rotation", &rotation_deg, 0.0f, 360.0f, "%.1f deg", ImGuiSliderFlags_None)) {
			state->triangle.rotation = glm_rad(rotation_deg);
		}

		// Scale control
		igSliderFloat("Scale", &state->triangle.scale, 0.1f, 2.0f, "%.2f", ImGuiSliderFlags_None);

		igEnd();
	}
} 