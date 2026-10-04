#include "renderer/renderer_backend.hpp"
#define CLASS_NAME "WindowManager"
#include "log_macros.hpp"
#include "window/window_manager.hpp"
#include "window/mml/multimedia_layer_factory.hpp"

WindowManager::~WindowManager() {
    if (renderer) {
        delete renderer;
    }
    if (window) {
        // TODO: multimedia lyer vai ser responsavel por isso
        SDL_DestroyWindow(window);
    }
    if (multiMediaLayer) {
        delete multiMediaLayer;
    }
    // TODO: multimedia lyer vai ser responsavel por isso
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

    multiMediaLayer = MultimediaLayerFactory::create(graphicsApi);
    if (!multiMediaLayer) {
        LOG_ERROR("Failed to initialize multimedia laayer!");
        return false;
    }

    // Propagate presentation config to the backend before any GPU init, so each
    // backend picks the right swapchain/framebuffer format and present mode.
    if (auto* backend = renderer->getRendererBackend()) {
        backend->setSrgbEnabled(rendererConfig.srgb);
        backend->setVsyncEnabled(rendererConfig.vsync);

        // Set GL/EGL context attributes (profile, version, color/depth sizes) 
        // BEFORE the window is created. 
        multiMediaLayer->configureContext(graphicsApi);
    }

    // TODO: multimedia layer vai ser responsavel por isso
    unsigned int flags = SDL_WINDOW_SHOWN | desc.extraFlags |
                         renderer->getRendererBackend()->getRequiredWindowFlags();

    // TODO: multimedia layer vai ser responsavel por isso
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
