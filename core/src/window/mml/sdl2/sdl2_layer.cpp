#ifdef __SWITCH__
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "sdl2_layer.hpp"

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
        case GraphicsAPI::WEBGL:
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        case GraphicsAPI::DIRECTX12:
        case GraphicsAPI::VULKAN:
        default:
            return 
    }
}