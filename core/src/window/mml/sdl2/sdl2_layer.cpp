#define CLASS_NAME "SDL2Layer"
#include "log_macros.hpp"

#ifdef __SWITCH__
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "sdl2_layer.hpp"
#include <string>

bool SDL2Layer::init() {
    // The engine owns the SDL lifecycle. SDL_MAIN_HANDLED is defined so SDL
    // does NOT hijack main() into SDL_main and SDL2main is not linked. That
    // means the video subsystem must be initialized before creating any window.
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG_ERROR(std::string("SDL_Init failed: ") + SDL_GetError());
        return false;
    }

    return true;
}

// For GL/EGL backends SDL freezes the pixel format at
// SDL_CreateWindow time, so these SDL_GL_SetAttribute
// calls must happen first. No-op for D3D12/Vulkan.
void SDL2Layer::configureContext(const GraphicsAPI& graphicsAPI) {
    switch (graphicsAPI) {
    case GraphicsAPI::OPENGL:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        break;
    case GraphicsAPI::WEBGL:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        break;
    case GraphicsAPI::DIRECTX12:
    case GraphicsAPI::VULKAN:
    default:
        return;
    }
}

void* SDL2Layer::createMainDisplay(unsigned int flags, const WindowDesc& opts) {
    SDL_Window* window =
        SDL_CreateWindow(opts.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         opts.width, opts.height, SDL_WINDOW_SHOWN | flags);

    if (!window) {
        LOG_ERROR(std::string("Failed to create SDL window: ") + SDL_GetError());
        return nullptr;
    }

    return window;
}

void SDL2Layer::destroyDisplay(void* display) {
    if (display) {
        SDL_DestroyWindow(reinterpret_cast<SDL_Window*>(display));
    }
}

void SDL2Layer::end() { SDL_Quit(); }