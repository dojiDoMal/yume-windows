#define CLASS_NAME "Renderer"
#include "../log_macros.hpp"

#include "../components/lod_group.hpp"
#include "../world_object.hpp"
#include "renderer.hpp"
#include "renderer_factory.hpp"
#include <cmath>

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

    backend->bindCamera(camera);
    backend->clear(camera);

    std::vector<Light*> lights;
    for (auto* obj : scene.getLightObjects()) {
        if (Light* light = obj->getComponent<Light>()) {
            lights.push_back(light);
        }
    }

    // Atualizar LOD
    WorldObject* camObj = camera->getOwner();
    if (camObj) {
        glm::vec3 camPos(camObj->getTransform().getPosition().x,
                         camObj->getTransform().getPosition().y,
                         camObj->getTransform().getPosition().z);

        for (auto& obj : scene.getObjectManager()->getObjects()) {
            auto* lodGroup = obj->getComponent<LodGroup>();
            if (!lodGroup)
                continue;

            float radius = 1.0f;
            auto* mesh = obj->getMesh();
            if (mesh && !mesh->getVertices().empty()) {
                const auto& verts = mesh->getVertices();
                for (size_t i = 0; i + 2 < verts.size(); i += 3) {
                    float d = std::sqrt(verts[i] * verts[i] + verts[i + 1] * verts[i + 1] +
                                        verts[i + 2] * verts[i + 2]);
                    if (d > radius)
                        radius = d;
                }
            }

            glm::vec3 objPos(obj->getTransform().getPosition().x,
                             obj->getTransform().getPosition().y,
                             obj->getTransform().getPosition().z);

            bool visible =
                lodGroup->update(objPos, radius, camPos, camera->getFov(), camera->getHeight());
            obj->setMesh(visible ? lodGroup->getActiveMeshShared() : nullptr);
        }
    }

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
