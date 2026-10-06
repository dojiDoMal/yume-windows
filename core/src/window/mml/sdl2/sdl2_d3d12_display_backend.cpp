#include "sdl2_d3d12_display_backend.hpp"
#define CLASS_NAME "SDL2D3D12DisplayBackend"
#include "log_macros.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

void* SDL2D3D12DisplayBackend::getNativeWindowHandle(void* window) {
    if (!window) {
        LOG_ERROR("getNativeWindowHandle: window is null");
        return nullptr;
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(reinterpret_cast<SDL_Window*>(window), &wmInfo)) {
        LOG_ERROR("Failed to get window info!");
        return nullptr;
    }
    return reinterpret_cast<void*>(wmInfo.info.win.window);
}
