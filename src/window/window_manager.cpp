#include "renderer/renderer_backend.hpp"
#define CLASS_NAME "WindowManager"
#include "log_macros.hpp"
#include "window_manager.hpp"

WindowManager::~WindowManager() {
    if (renderer) {
        delete renderer;
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

void WindowManager::render(Scene& scene) { renderer->render(scene); }

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

    // Propagate presentation config to the backend before any GPU init, so each
    // backend picks the right swapchain/framebuffer format and present mode.
    if (auto* backend = renderer->getRendererBackend()) {
        backend->setSrgbEnabled(rendererConfig.srgb);
        backend->setVsyncEnabled(rendererConfig.vsync);

        // Let the backend set its GL/EGL context attributes (profile, version,
        // color/depth sizes) BEFORE the window is created. For GL/EGL backends
        // SDL freezes the pixel format at SDL_CreateWindow time, so these
        // SDL_GL_SetAttribute calls must happen first. No-op for D3D12/Vulkan.
        backend->initWindowContext();
    }

    unsigned int flags = SDL_WINDOW_SHOWN | desc.extraFlags |
                         renderer->getRendererBackend()->getRequiredWindowFlags();

    window = SDL_CreateWindow(desc.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              desc.width, desc.height, flags);
    if (!window) {
        LOG_ERROR(std::string("Failed to create window: %s\n") + SDL_GetError());
        SDL_Quit();
        return false;
    }

    if (!renderer->initWindow(window)) {
        LOG_ERROR("Failed to initialize window!");
        return false;
    }

    return window != nullptr;
}
