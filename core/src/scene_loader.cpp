#define CLASS_NAME "SceneLoader"
#include "log_macros.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"

#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/mesh_renderer.hpp"
#include "components/sprite_renderer.hpp"
#include "material.hpp"
#include "renderer/renderer_backend.hpp"
#include "scene_format.hpp"
#include "scene_loader.hpp"
#include "shader_asset.hpp"
#include "skybox.hpp"
#include "stb_image.h"
#include <fstream>

SceneLoader::SceneLoader() : rendererBackend(nullptr) {}

void SceneLoader::setRendererBackend(RendererBackend& backend) { rendererBackend = &backend; }

CompiledScene* SceneLoader::loadCompiledScene(const std::string& filepath) {
    if (!validateSceneFile(filepath))
        return nullptr;

    std::ifstream file(filepath, std::ios::binary);
    auto scene = new CompiledScene();
    if (!file.read(reinterpret_cast<char*>(scene), sizeof(CompiledScene))) {
        LOG_ERROR("Failed to read scene file: " + filepath);
        delete scene;
        return nullptr;
    }

    LOG_INFO("Loaded scene with " + std::to_string(scene->worldObjectCount) + " world objects");
    return scene;
}

void SceneLoader::loadMeshRendererComponent(WorldObject* obj, const ComponentData& comp) {
    auto& meshData = comp.meshRenderer.mesh;
    auto& materialData = comp.meshRenderer.material;

    auto mesh = loadObjMesh(meshData.path, meshData.shadeSmooth);
    if (!mesh) {
        LOG_ERROR("Failed to load mesh: " + std::string(meshData.path));
        return;
    }
    mesh->setMeshBuffer(rendererBackend->createMeshBuffer());
    mesh->configure();

    auto shaderExt = rendererBackend->getShaderExtension();
    auto vertexShader = std::make_unique<ShaderAsset>(materialData.vertexShaderPath + shaderExt,
                                                      ShaderType::VERTEX);
    vertexShader->setShaderCompiler(rendererBackend->createShaderCompiler());

    auto fragmentShader = std::make_unique<ShaderAsset>(materialData.fragmentShaderPath + shaderExt,
                                                        ShaderType::FRAGMENT);
    fragmentShader->setShaderCompiler(rendererBackend->createShaderCompiler());

    auto material = std::make_unique<Material>();
    material->setShaderProgram(rendererBackend->createShaderProgram());
    material->setVertexShader(std::move(vertexShader));
    material->setFragmentShader(std::move(fragmentShader));
    material->setBaseColor(materialData.color);

    if (!material->init()) {
        LOG_ERROR("Material init failed for mesh: " + std::string(meshData.path));
        return;
    }

    auto meshRenderer = std::make_unique<MeshRenderer>();
    meshRenderer->setMaterial(std::move(material));

    obj->setMesh(std::move(mesh));
    obj->addComponent(std::move(meshRenderer));
}

void SceneLoader::loadSpriteRendererComponent(WorldObject* obj, const ComponentData& comp) {
    auto& textureData = comp.spriteRenderer.texture;
    auto& materialData = comp.spriteRenderer.material;

    float width = textureData.width * textureData.scaleFactor;
    float height = textureData.height * textureData.scaleFactor;
    auto sprite = std::make_unique<Sprite>(width, height);

    unsigned int texID = rendererBackend->loadTexture(textureData.path, textureData.filterType);
    sprite->setTexture(texID);

    auto shaderExt = rendererBackend->getShaderExtension();
    auto vertexShader = std::make_unique<ShaderAsset>(materialData.vertexShaderPath + shaderExt,
                                                      ShaderType::VERTEX);
    vertexShader->setShaderCompiler(rendererBackend->createShaderCompiler());

    auto fragmentShader = std::make_unique<ShaderAsset>(materialData.fragmentShaderPath + shaderExt,
                                                        ShaderType::FRAGMENT);
    fragmentShader->setShaderCompiler(rendererBackend->createShaderCompiler());

    auto material = std::make_unique<Material>();
    material->setShaderProgram(rendererBackend->createShaderProgram());
    material->setVertexShader(std::move(vertexShader));
    material->setFragmentShader(std::move(fragmentShader));
    material->setBaseColor(materialData.color);

    if (!material->init()) {
        LOG_ERROR("Material init failed for sprite: " + std::string(textureData.path));
        return;
    }

    auto spriteRenderer = std::make_unique<SpriteRenderer>();
    spriteRenderer->setMaterial(std::move(material));

    obj->setSprite(std::move(sprite));
    obj->addComponent(std::move(spriteRenderer));
}

void SceneLoader::loadCameraComponent(WorldObject* obj, const ComponentData& comp) {
    auto& camData = comp.camera;

    auto camera = std::make_unique<Camera>();
    camera->setBackgroundColor(ColorRGBA{camData.background_color[0], camData.background_color[1],
                                         camData.background_color[2], camData.background_color[3]});
    camera->setFov(camData.fov);
    camera->setViewRect(camData.view_rect[0], camData.view_rect[1]);
    camera->setOrthographic(camData.orthographic);
    camera->setOrthoSize(camData.orthoSize);

    if (camData.hasSkybox) {
        auto skybox = std::make_unique<Skybox>();

        auto shaderExt = rendererBackend->getShaderExtension();
        auto skyboxVertexShaderPtr = std::make_unique<ShaderAsset>(
            camData.skybox.material.vertexShaderPath + shaderExt, ShaderType::VERTEX);
        skyboxVertexShaderPtr->setShaderCompiler(rendererBackend->createShaderCompiler());

        auto skyboxFragmentShaderPtr = std::make_unique<ShaderAsset>(
            camData.skybox.material.fragmentShaderPath + shaderExt, ShaderType::FRAGMENT);
        skyboxFragmentShaderPtr->setShaderCompiler(rendererBackend->createShaderCompiler());

        auto skyboxMaterial = std::make_unique<Material>();
        skyboxMaterial->setShaderProgram(rendererBackend->createShaderProgram());
        skyboxMaterial->setVertexShader(std::move(skyboxVertexShaderPtr));
        skyboxMaterial->setFragmentShader(std::move(skyboxFragmentShaderPtr));
        skyboxMaterial->init();

        std::vector<std::string> faces;
        for (const auto& row : camData.skybox.cubeMapTextures) {
            faces.push_back(row);
        }

        unsigned int cubemapID = rendererBackend->createCubemapTexture(faces);
        skybox->setTextureID(cubemapID);
        skybox->setMaterial(std::move(skyboxMaterial));
        skybox->init();
        camera->setSkybox(std::move(skybox));
    }

    obj->addComponent(std::move(camera));
}

void SceneLoader::loadLightComponent(WorldObject* obj, const ComponentData& comp) {
    auto& lightData = comp.light;

    auto light = std::make_unique<Light>();
    light->setType(static_cast<LightType>(lightData.lightType));
    light->setDirection(lightData.direction);
    light->setColor(
        ColorRGBA{lightData.color[0], lightData.color[1], lightData.color[2], lightData.color[3]});
    light->setIntensity(lightData.intensity);

    obj->addComponent(std::move(light));
}

std::unique_ptr<Mesh> SceneLoader::loadObjMesh(const std::string& filepath, bool shadeSmooth) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filepath.c_str())) {
        LOG_ERROR("Unable to load obj: " + filepath);
        return nullptr;
    }

    std::vector<float> vertices;
    std::vector<float> normals;

    if (shadeSmooth && !attrib.normals.empty()) {
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);

                if (index.normal_index >= 0) {
                    normals.push_back(attrib.normals[3 * index.normal_index + 0]);
                    normals.push_back(attrib.normals[3 * index.normal_index + 1]);
                    normals.push_back(attrib.normals[3 * index.normal_index + 2]);
                }
            }
        }
    } else {
        for (const auto& shape : shapes) {
            for (size_t f = 0; f < shape.mesh.indices.size(); f += 3) {
                auto& i0 = shape.mesh.indices[f + 0];
                auto& i1 = shape.mesh.indices[f + 1];
                auto& i2 = shape.mesh.indices[f + 2];

                float v0[3] = {attrib.vertices[3 * i0.vertex_index + 0],
                               attrib.vertices[3 * i0.vertex_index + 1],
                               attrib.vertices[3 * i0.vertex_index + 2]};
                float v1[3] = {attrib.vertices[3 * i1.vertex_index + 0],
                               attrib.vertices[3 * i1.vertex_index + 1],
                               attrib.vertices[3 * i1.vertex_index + 2]};
                float v2[3] = {attrib.vertices[3 * i2.vertex_index + 0],
                               attrib.vertices[3 * i2.vertex_index + 1],
                               attrib.vertices[3 * i2.vertex_index + 2]};

                float edge1[3] = {v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]};
                float edge2[3] = {v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]};
                float normal[3] = {edge1[1] * edge2[2] - edge1[2] * edge2[1],
                                   edge1[2] * edge2[0] - edge1[0] * edge2[2],
                                   edge1[0] * edge2[1] - edge1[1] * edge2[0]};

                float len =
                    sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
                if (len > 0) {
                    normal[0] /= len;
                    normal[1] /= len;
                    normal[2] /= len;
                }

                for (int i = 0; i < 3; i++) {
                    auto& idx = shape.mesh.indices[f + i];
                    vertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
                    vertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
                    vertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);
                    normals.push_back(normal[0]);
                    normals.push_back(normal[1]);
                    normals.push_back(normal[2]);
                }
            }
        }
    }

    auto mesh = std::make_unique<Mesh>();
    mesh->setVertices(vertices);
    mesh->setNormals(normals);
    return mesh;
}

void SceneLoader::loadWorldObjects(WorldObjectManager* manager, const CompiledScene* scene) {
    LOG_INFO("Loading " + std::to_string(scene->worldObjectCount) + " world objects");

    for (uint32_t i = 0; i < scene->worldObjectCount; i++) {
        auto& woData = scene->worldObjects[i];
        auto* obj = manager->createObject();

        // Carregar transform
        obj->getTransform().setPosition(woData.position);
        obj->getTransform().setRotation(woData.rotation);
        obj->getTransform().setScale(woData.scale);

        LOG_INFO("WorldObject #" + std::to_string(i) + " - Pos: (" +
                 std::to_string(woData.position.x) + ", " + std::to_string(woData.position.y) +
                 ", " + std::to_string(woData.position.z) + ")");

        // Carregar componentes
        for (uint8_t j = 0; j < woData.componentCount; j++) {
            auto& comp = woData.components[j];

            switch (comp.type) {
            case ComponentType::MESH_RENDERER:
                LOG_INFO("  - Loading MESH_RENDERER component");
                loadMeshRendererComponent(obj, comp);
                break;
            case ComponentType::SPRITE_RENDERER:
                LOG_INFO("  - Loading SPRITE_RENDERER component");
                loadSpriteRendererComponent(obj, comp);
                break;
            case ComponentType::CAMERA:
                LOG_INFO("  - Loading CAMERA component");
                loadCameraComponent(obj, comp);
                break;
            case ComponentType::LIGHT:
                LOG_INFO("  - Loading LIGHT component");
                loadLightComponent(obj, comp);
                break;
            default:
                break;
            }
        }
    }
}

bool SceneLoader::validateSceneFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.good()) {
        LOG_ERROR("Scene file does not exist: " + filepath);
        return false;
    }
    return true;
}
