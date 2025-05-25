#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#include "../lib/cimgui/imgui/backends/imgui_impl_sdlrenderer.cpp"
#include "../lib/cimgui/imgui/backends/imgui_impl_sdl.cpp"

#define CIMGUI_API extern "C" __attribute__((__visibility__("default")))

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
struct SDL_Window;
struct SDL_Renderer;
typedef union SDL_Event SDL_Event;
typedef struct ImDrawData ImDrawData;

CIMGUI_API bool ImguiBackendSDL_Init(SDL_Window* window,SDL_Renderer* renderer) {
    return ImGui_ImplSDL2_InitForSDLRenderer(window, renderer)
        && ImGui_ImplSDLRenderer_Init(renderer);
}
CIMGUI_API void ImguiBackendSDL_Shutdown(void) {
    ImGui_ImplSDL2_Shutdown();
    ImGui_ImplSDLRenderer_Shutdown();
}
CIMGUI_API void ImguiBackendSDL_NewFrame(void) {
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
}
CIMGUI_API bool ImguiBackendSDL_ProcessEvent(const SDL_Event* event) {
    return ImGui_ImplSDL2_ProcessEvent(event);
}
CIMGUI_API void ImguiBackendSDL_RenderDrawData(struct ImDrawData *draw_data) {
    ImGui_ImplSDLRenderer_RenderDrawData(draw_data);
}
