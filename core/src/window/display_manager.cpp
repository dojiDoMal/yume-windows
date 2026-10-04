#include "renderer/renderer_backend.hpp"
#define CLASS_NAME "DisplayManager"
#include "log_macros.hpp"
#include "window/display_manager.hpp"
#include "window/mml/multimedia_layer_factory.hpp"

DisplayManager::~DisplayManager() {
    if (renderer) {
        delete renderer;
    }

    if (multiMediaLayer) {
        multiMediaLayer->destroyDisplay(window);
        multiMediaLayer->end();
    }

    if (multiMediaLayer) {
        delete multiMediaLayer;
    }
}

void DisplayManager::render(Scene& scene) { renderer->render(scene); }

void DisplayManager::present() {
    if (renderer) {
        // The MML hands back an opaque display handle (void*). The current
        // Renderer/RendererBackend interface is still typed as SDL_Window*, so
        // bridge the two here. TODO: make the renderer accept an opaque handle
        // (void*) so it no longer depends on SDL — see note in display_manager.hpp.
        renderer->present(reinterpret_cast<SDL_Window*>(window));
    }
}

bool DisplayManager::init(const WindowDesc& desc) {

    renderer = new Renderer();
    if (!renderer->initBackend(graphicsApi)) {
        LOG_ERROR("Failed to initialize renderer backend!");
        return false;
    }

    multiMediaLayer = MultimediaLayerFactory::create(graphicsApi);
    if (!multiMediaLayer) {
        LOG_ERROR("Failed to create multimedia layer!");
        return false;
    }
    if (!multiMediaLayer->init()) {
        LOG_ERROR("Failed to initialize multimedia layer!");
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

    window = multiMediaLayer->createMainDisplay(
        desc.extraFlags | renderer->getRendererBackend()->getRequiredWindowFlags(), desc);

    if (!window) {
        LOG_ERROR("Failed to create display!");
        return false;
    }

    if (!renderer->initWindow(reinterpret_cast<SDL_Window*>(window))) {
        LOG_ERROR("Failed to initialize window!");
        return false;
    }

    return window != nullptr;
}
