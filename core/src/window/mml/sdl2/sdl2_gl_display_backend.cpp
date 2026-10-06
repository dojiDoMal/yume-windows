#include "sdl2_gl_display_backend.hpp"
#define CLASS_NAME "SDL2GLDisplayBackend"
#include "log_macros.hpp"
#ifdef __SWITCH__
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

unsigned int SDL2GLDisplayBackend::getApiWindowFlags() const { return SDL_WINDOW_OPENGL; }

void* SDL2GLDisplayBackend::createGLContext(void* window) {
    if (!window) {
        LOG_ERROR("Cannot create GL context: window is null");
        return nullptr;
    }
    // SDL_GL_CreateContext also makes the new context current on the calling
    // thread, which is exactly what the GL backend expects before loading
    // entry points.
    SDL_GLContext context = SDL_GL_CreateContext(reinterpret_cast<SDL_Window*>(window));
    if (!context) {
        LOG_ERROR(std::string("Failed to create OpenGL context: ") + SDL_GetError());
        return nullptr;
    }
    return context;
}

void SDL2GLDisplayBackend::destroyGLContext(void* context) {
    if (context) {
        SDL_GL_DeleteContext(static_cast<SDL_GLContext>(context));
    }
}

void SDL2GLDisplayBackend::setSwapInterval(bool vsync) {
    // 1 = cap to the monitor refresh rate, 0 = uncapped.
    SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}

void* SDL2GLDisplayBackend::getGLProcAddressLoader() {
    // Returned as a plain function pointer so the GL backend can hand it to
    // glad (GLADloadproc) without this header depending on glad.
    return reinterpret_cast<void*>(&SDL_GL_GetProcAddress);
}

void SDL2GLDisplayBackend::swapBuffers(void* window) {
    if (window) {
        SDL_GL_SwapWindow(reinterpret_cast<SDL_Window*>(window));
    }
}
