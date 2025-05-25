#pragma once

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

typedef union SDL_Event SDL_Event;
bool imgui_impl_process_sdl_event(const SDL_Event *event);

bool ImGui_ImplOpenGL3_Init();
void ImGui_ImplOpenGL3_Shutdown();
void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);
