#include "sdl2_display_backend.hpp"
#define CLASS_NAME "SDL2DisplayBackend"
#include "log_macros.hpp"
#include <SDL2/SDL.h>

// For GL/EGL backends SDL freezes the pixel format at
// SDL_CreateWindow time, so these SDL_GL_SetAttribute
// calls must happen first. No-op for D3D12/Vulkan.
void SDL2DisplayBackend::configureContext(const GraphicsAPI& graphicsAPI) {
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

void* SDL2DisplayBackend::createMainDisplay(unsigned int flags, const WindowDesc& opts) {
    SDL_Window* window =
        SDL_CreateWindow(opts.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         opts.width, opts.height, SDL_WINDOW_SHOWN | flags);

    if (!window) {
        LOG_ERROR(std::string("Failed to create SDL window: ") + SDL_GetError());
        return nullptr;
    }

    return window;
}

void SDL2DisplayBackend::destroyDisplay(void* display) {
    if (display) {
        SDL_DestroyWindow(reinterpret_cast<SDL_Window*>(display));
    }
}
