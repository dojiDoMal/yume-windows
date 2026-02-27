#define CLASS_NAME "Renderer"
#include "../log_macros.hpp"

#include "../game_object.hpp"
#include "../material.hpp"
#include "../log_macros.hpp"
#include "renderer_factory.hpp"
#include "renderer.hpp"
#include <cstdint>
#include <cstdio>

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

    if (!scene.getCamera()) {
        LOG_WARN("Scene doesn't have a main camera to render!");
        return;
    }

    // Order is important here

    backend->bindCamera(scene.getCamera());

    backend->clear(scene.getCamera());

    backend->renderGameObjects(const_cast<std::vector<GameObject*>*>(scene.getGameObjects()),
                               const_cast<std::vector<Light>*>(scene.getLights()));
}

void Renderer::present(SDL_Window* window) {
    if (backend) {
        backend->present(window);
    }
}

//deprecated
void Renderer::render(const std::vector<GameObject*>* objects) {

    if (backend && backend->getCamera()) {
        auto skybox = backend->getCamera()->getSkybox();
        if (skybox) {
            printf("Skybox exists\n");
            if (skybox->getMaterial()) {
                printf("Skybox material exists\n");
                skybox->getMaterial()->use();
                auto program = skybox->getMaterial()->getShaderProgram();
                if (program) {
                    printf("Skybox program exists\n");
                    unsigned int shaderProgram = static_cast<unsigned int>(
                        reinterpret_cast<uintptr_t>(program->getHandle()));
                    printf("Calling renderSkybox with shader: %d, texture: %d\n", shaderProgram,
                           skybox->getTextureID());
                    backend->renderSkybox(*skybox->getMesh(), shaderProgram,
                                          skybox->getTextureID());
                } else {
                    printf("Skybox program is null\n");
                }
            } else {
                printf("Skybox material is null\n");
            }
        } else {
            // printf("Skybox is null\n");
        }
    } else {
        printf("Backend or camera is null\n");
    }
}