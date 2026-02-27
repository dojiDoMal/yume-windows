#include "renderer/renderer_backend.hpp"
#define CLASS_NAME "WindowManager"
#include "window_manager.hpp"
#include "log_macros.hpp"


WindowManager::~WindowManager() {
    if (renderer) {
        delete renderer;
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

void WindowManager::render(Scene& scene) {
    renderer->render(scene);
}

void WindowManager::present() {
    if (renderer) {
        renderer->present(window);
    }
}

bool WindowManager::init(const WindowDesc& desc) {

    renderer = new Renderer();

    if (!renderer->initBackend(graphicsApi)) {
        LOG_ERROR("Failed to initialize renderer backend!");
        return false;
    }

    unsigned int flags = SDL_WINDOW_SHOWN | desc.extraFlags |
                         renderer->getRendererBackend()->getRequiredWindowFlags();

    window = SDL_CreateWindow(desc.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              desc.width, desc.height, flags);

    if (!window) {
        SDL_Quit();
        return false;
    }

    if (!renderer->initWindow(window)) {
        LOG_ERROR("Failed to initialize window!");
        return false;
    }

    return window != nullptr;
}
