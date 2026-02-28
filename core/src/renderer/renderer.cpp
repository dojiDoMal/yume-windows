#define CLASS_NAME "Renderer"
#include "../log_macros.hpp"

#include "../world_object.hpp"
#include "renderer.hpp"
#include "renderer_factory.hpp"


Renderer::~Renderer() {
    if (backend) {
        delete backend;
    }
}

void Renderer::setRendererBackend(RendererBackend* backend) { this->backend = backend; }

RendererBackend* Renderer::getRendererBackend() { return backend; }

bool Renderer::initBackend(const GraphicsAPI& graphicsApi) {
    backend = RendererFactory::create(graphicsApi);
    if (!backend) {
        LOG_ERROR("Unsupported graphics API!");
        return false;
    }
    return true;
}

bool Renderer::initWindow(SDL_Window* win) {
    if (backend) {
        return backend->init(win);
    }
    return false;
}

void Renderer::render(const Scene& scene) {
    if (!backend) {
        LOG_ERROR("Can not render without a renderer backend!");
        return;
    }

    Camera* camera = scene.getCamera();
    if (!camera) {
        LOG_WARN("Scene doesn't have a main camera to render!");
        return;
    }

    // Bind camera
    backend->bindCamera(camera);

    // Clear screen
    backend->clear(camera);

    // Collect lights
    std::vector<Light*> lights;
    for (auto* obj : scene.getLightObjects()) {
        if (Light* light = obj->getComponent<Light>()) {
            lights.push_back(light);
        }
    }

    // Render objects
    std::vector<WorldObject*> renderableObjects;
    for (auto& obj : scene.getObjectManager()->getObjects()) {
        if (obj->hasMesh() || obj->hasSprite()) {
            renderableObjects.push_back(obj.get());
        }
    }

    backend->renderWorldObjects(renderableObjects, lights);
}

void Renderer::present(SDL_Window* window) {
    if (backend) {
        backend->present(window);
    }
}
