#pragma once

#ifndef CIMGUI_API
#define CIMGUI_API extern "C" __attribute__((__visibility__("default")))
#endif

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
struct SDL_Window;
struct SDL_Renderer;
typedef union SDL_Event SDL_Event;
typedef struct ImDrawData ImDrawData;

CIMGUI_API bool ImguiBackendSDL_Init(SDL_Window*,SDL_Renderer*);
CIMGUI_API void ImguiBackendSDL_Shutdown(void);
CIMGUI_API void ImguiBackendSDL_NewFrame(void);
CIMGUI_API bool ImguiBackendSDL_ProcessEvent(const SDL_Event*);
CIMGUI_API void ImguiBackendSDL_RenderDrawData(struct ImDrawData *draw_data);
