#define CLASS_NAME "Renderer"
#include "../log_macros.hpp"

#include "../components/lod_group.hpp"
#include "../world_object.hpp"
#include "frustum.hpp"
#include "renderer.hpp"
#include "renderer_factory.hpp"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

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
        Vector3 camPos = camObj->getTransform().getPosition();

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

            Vector3 objPos = obj->getTransform().getPosition();

            bool visible =
                lodGroup->update(objPos, radius, camPos, camera->getFov(), camera->getHeight());
            obj->setMesh(visible ? lodGroup->getActiveMeshShared() : nullptr);
        }
    }

    // Build the camera frustum from a view-projection matrix reconstructed
    // from the camera transform + parameters (mirrors the backend's
    // bindCamera). Used for CPU frustum culling below.
    Frustum frustum;
    bool frustumValid = false;
    if (backend->isFrustumCullingEnabled() && camObj) {
        const auto camPos = camObj->getTransform().getPosition();
        const auto camRot = camObj->getTransform().getRotation();

        float yawRad = glm::radians(camRot.y);
        float pitchRad = glm::radians(camRot.x);
        glm::vec3 forward;
        forward.x = std::cos(pitchRad) * std::sin(yawRad);
        forward.y = std::sin(pitchRad);
        forward.z = std::cos(pitchRad) * std::cos(yawRad);
        forward = glm::normalize(forward);
        forward = -forward; // OpenGL forward is -Z

        glm::vec3 camPosVec(camPos.x, camPos.y, camPos.z);
        glm::mat4 view = glm::lookAt(camPosVec, camPosVec + forward, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 projection;
        if (camera->isOrthographic()) {
            float orthoSize = camera->getOrthoSize();
            float aspect = camera->getAspectRatio();
            projection = glm::ortho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize,
                                    camera->getNearDistance(), camera->getFarDistance());
        } else {
            projection = glm::perspective(glm::radians(camera->getFov()), camera->getAspectRatio(),
                                          camera->getNearDistance(), camera->getFarDistance());
        }

        frustum.fromViewProjection(projection * view);
        frustumValid = true;
    }

    int frustumCulled = 0;
    std::vector<WorldObject*> renderableObjects;
    for (auto& obj : scene.getObjectManager()->getObjects()) {
        if (!(obj->hasMesh() || obj->hasSprite()))
            continue;

        // Frustum cull mesh objects that have a valid bounding sphere.
        // Sprites and mesh-less objects are always kept.
        if (frustumValid && obj->hasMesh()) {
            auto* mesh = obj->getMesh();
            if (mesh && mesh->hasBounds()) {
                glm::mat4 model = obj->getTransform().getModelMatrix();
                const Vector3& bc = mesh->getBoundingCenter();
                glm::vec3 worldCenter = glm::vec3(model * glm::vec4(bc.x, bc.y, bc.z, 1.0f));

                // Scale the radius by the largest axis scale so the sphere
                // still encloses the mesh after non-uniform scaling.
                const auto scl = obj->getTransform().getScale();
                float maxScale = std::max({std::abs(scl.x), std::abs(scl.y), std::abs(scl.z)});
                float worldRadius = mesh->getBoundingRadius() * maxScale;

                if (!frustum.intersectsSphere({worldCenter.x, worldCenter.y, worldCenter.z},
                                              worldRadius)) {
                    frustumCulled++;
                    continue; // outside the frustum -> skip
                }
            }
        }

        renderableObjects.push_back(obj.get());
    }

    backend->setFrustumCulledObjects(frustumCulled);
    backend->renderWorldObjects(renderableObjects, lights);
}

void Renderer::present(SDL_Window* window) {
    if (backend) {
        backend->present(window);
    }
}
