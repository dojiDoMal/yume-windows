#include "renderer/renderer_backend.hpp"
#define CLASS_NAME "DisplayManager"
#include "log_macros.hpp"
#include "window/display_manager.hpp"

DisplayManager::~DisplayManager() {
    // The DisplayBackend is owned by the MultimediaLayer, not here. Destroy the
    // window we created through it, but do not delete the backend or tear down
    // the platform subsystem — that is the MultimediaLayer's job.
    if (displayBackend) {
        displayBackend->destroyDisplay(window);
    }

    if (renderer) {
        delete renderer;
    }
}

void DisplayManager::render(Scene& scene) { renderer->render(scene); }

void DisplayManager::present() {
    if (renderer) {
        // The window is an opaque handle end-to-end now: the renderer no longer
        // knows it is an SDL_Window. Presentation that needs the window (GL swap)
        // goes back through the DisplayBackend inside the renderer backend.
        renderer->present(window);
    }
}

bool DisplayManager::init(DisplayBackend& backend, const WindowDesc& desc) {
    displayBackend = &backend;

    renderer = new Renderer();
    if (!renderer->initBackend(graphicsApi)) {
        LOG_ERROR("Failed to initialize renderer backend!");
        return false;
    }

    // Propagate presentation config to the backend before any GPU init, so each
    // backend picks the right swapchain/framebuffer format and present mode.
    if (auto* rb = renderer->getRendererBackend()) {
        rb->setSrgbEnabled(rendererConfig.srgb);
        rb->setVsyncEnabled(rendererConfig.vsync);

        // Set GL/EGL context attributes (profile, version, color/depth sizes)
        // BEFORE the window is created.
        displayBackend->configureContext(graphicsApi);
    }

    window = displayBackend->createMainDisplay(
        desc.extraFlags | displayBackend->getApiWindowFlags(), desc);

    if (!window) {
        LOG_ERROR("Failed to create display!");
        return false;
    }

    if (!renderer->initWindow(window, *displayBackend)) {
        LOG_ERROR("Failed to initialize window!");
        return false;
    }

    return window != nullptr;
}
