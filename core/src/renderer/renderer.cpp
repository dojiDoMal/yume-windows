#define CLASS_NAME "Renderer"
#include "log_macros.hpp"

#include "components/lod_group.hpp"
#include "math/math.hpp"
#include "scene/world_object.hpp"
#include "renderer/frustum.hpp"
#include "renderer/renderer.hpp"
#include "renderer/renderer_factory.hpp"
#include <algorithm>
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

        float yawRad = Yume::Math::radians(camRot.y);
        float pitchRad = Yume::Math::radians(camRot.x);
        Vector3 forward;
        forward.x = std::cos(pitchRad) * std::sin(yawRad);
        forward.y = std::sin(pitchRad);
        forward.z = std::cos(pitchRad) * std::cos(yawRad);
        forward = Yume::Math::normalize(forward);
        forward = forward * -1.0f; // OpenGL forward is -Z

        Vector3 camPosVec{camPos.x, camPos.y, camPos.z};
        Matrix4 view = Yume::Math::lookAt(camPosVec, camPosVec + forward, {0.0f, 1.0f, 0.0f});

        Matrix4 projection;
        if (camera->isOrthographic()) {
            float orthoSize = camera->getOrthoSize();
            float aspect = camera->getAspectRatio();
            projection =
                Yume::Math::ortho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize,
                                  camera->getNearDistance(), camera->getFarDistance());
        } else {
            projection = Yume::Math::perspective(
                Yume::Math::radians(camera->getFov()), camera->getAspectRatio(),
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
                Matrix4 model = obj->getTransform().getModelMatrix();
                const Vector3& bc = mesh->getBoundingCenter();
                Vector4 worldCenter = model * Vector4{bc.x, bc.y, bc.z, 1.0f};

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
