#define CLASS_NAME "SceneLoader"
#include "log_macros.hpp"
#include "platform_paths.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "assets/tinyobjloader/tiny_obj_loader.h"

#include "assets/material.hpp"
#include "assets/shader_asset.hpp"
#include "assets/stb_image.h"
#include "components/box_collider.hpp"
#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/lod_group.hpp"
#include "components/mesh_renderer.hpp"
#include "components/plane_collider.hpp"
#include "components/rigidbody.hpp"
#include "components/script_component.hpp"
#include "components/sphere_collider.hpp"
#include "components/sprite_renderer.hpp"
#include "components/text_renderer_component.hpp"
#include "renderer/renderer_backend.hpp"
#include "scene/scene_format.hpp"
#include "scene/scene_loader.hpp"
#include "scene/skybox.hpp"
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>

SceneLoader::SceneLoader() : rendererBackend(nullptr) {}

void SceneLoader::setRendererBackend(RendererBackend& backend) { rendererBackend = &backend; }

std::unique_ptr<Material> SceneLoader::createMaterial(const MaterialData& matData) {
    auto shaderExt = rendererBackend->getShaderExtension();

    // Resolve against the platform asset root (romfs:/ on consoles, no-op
    // elsewhere) before appending the backend's shader extension.
    auto vertexShader = std::make_unique<ShaderAsset>(
        Yume::resolveAssetPath(matData.vertexShaderPath + shaderExt), ShaderType::VERTEX);
    vertexShader->setShaderCompiler(rendererBackend->createShaderCompiler());

    auto material = std::make_unique<Material>();
    material->setShaderProgram(rendererBackend->createShaderProgram());
    material->setVertexShader(std::move(vertexShader));

#ifndef __3DS__
    // O PICA200 (3DS) não tem fragment shader programável: a etapa de fragmento
    // é configurada por TexEnv no backend, não por um shader. Então só criamos/
    // anexamos o fragment nas plataformas que de fato o usam — no 3DS não há
    // ".shbin" de fragment para carregar.
    auto fragmentShader = std::make_unique<ShaderAsset>(
        Yume::resolveAssetPath(matData.fragmentShaderPath + shaderExt), ShaderType::FRAGMENT);
    fragmentShader->setShaderCompiler(rendererBackend->createShaderCompiler());
    material->setFragmentShader(std::move(fragmentShader));
#endif

    material->setBaseColor(matData.color);

    if (!material->init())
        return nullptr;

    // Texturas PBR (opcionais). Caminho vazio = sem textura. Albedo é carregado
    // como sRGB; os demais mapas (normal/roughness/metallic) são dados lineares.
    // Filtro LINEAR (1) para suavizar; a geração de mipmaps é feita no backend.
    auto loadMap = [&](const char* relPath, bool srgb) -> unsigned int {
        if (relPath[0] == '\0')
            return 0;
        return rendererBackend->loadTexture(Yume::resolveAssetPath(relPath), 1, srgb);
    };

    material->setAlbedoMap(loadMap(matData.albedoMap, true));
    material->setNormalMap(loadMap(matData.normalMap, false));
    material->setRoughnessMap(loadMap(matData.roughnessMap, false));
    material->setMetallicMap(loadMap(matData.metallicMap, false));

    return material;
}

void SceneLoader::loadLodGroupComponent(WorldObject* obj, const ComponentData& comp) {
    auto& data = comp.lodGroup;

    auto& matData = data.material;
    auto& c = matData.color;
    std::string matKey = std::string(matData.vertexShaderPath) + "|" + matData.fragmentShaderPath +
                         "|" + std::to_string(c.r) + std::to_string(c.g) + std::to_string(c.b) +
                         std::to_string(c.a) + "|" + matData.albedoMap + "|" + matData.normalMap +
                         "|" + matData.roughnessMap + "|" + matData.metallicMap;

    auto matIt = materialCache.find(matKey);
    if (matIt == materialCache.end()) {
        auto material = createMaterial(matData);
        if (!material) {
            LOG_ERROR("Material init failed for LOD_GROUP");
            return;
        }
        materialCache[matKey] = std::move(material);
        matIt = materialCache.find(matKey);
    }

    auto lodGroup = std::make_unique<LodGroup>();

    for (uint8_t i = 0; i < data.levelCount; i++) {
        auto& lvl = data.levels[i];
        auto mesh = loadObjMesh(lvl.mesh.path, lvl.mesh.shadeSmooth);
        if (!mesh) {
            LOG_ERROR("Failed to load LOD mesh: " + std::string(lvl.mesh.path));
            return;
        }
        lodGroup->addLevel(mesh, lvl.screenSpaceThreshold);
    }

    // Seta o mesh do LOD0 como mesh inicial do objeto
    if (data.levelCount > 0)
        obj->setMesh(loadObjMesh(data.levels[0].mesh.path, data.levels[0].mesh.shadeSmooth));

    auto meshRenderer = std::make_unique<MeshRenderer>();
    meshRenderer->setMaterial(matIt->second);
    obj->addComponent(std::move(meshRenderer));
    obj->addComponent(std::move(lodGroup));
}

std::unique_ptr<CompiledScene> SceneLoader::loadCompiledScene(const std::string& filepath) {
    if (!validateSceneFile(filepath))
        return nullptr;

    std::ifstream file(filepath, std::ios::binary);

    // Read the fixed header first, then the variable objects block.
    SceneHeader header{};
    if (!file.read(reinterpret_cast<char*>(&header), sizeof(header))) {
        LOG_ERROR("Failed to read scene header: " + filepath);
        return nullptr;
    }

    if (header.magic != SCENE_MAGIC) {
        LOG_ERROR("Bad scene magic (stale or corrupt .scnb?): " + filepath);
        return nullptr;
    }

    if (header.worldObjectCount > MAX_WORLD_OBJECTS) {
        LOG_ERROR("Scene worldObjectCount (" + std::to_string(header.worldObjectCount) +
                  ") exceeds MAX_WORLD_OBJECTS (" + std::to_string(MAX_WORLD_OBJECTS) +
                  "): " + filepath);
        return nullptr;
    }

    auto scene = std::make_unique<CompiledScene>();
    scene->worldObjectCount = header.worldObjectCount;
    scene->worldObjects.resize(header.worldObjectCount);

    if (header.worldObjectCount > 0) {
        const std::streamsize bytes =
            static_cast<std::streamsize>(header.worldObjectCount) * sizeof(WorldObjectData);
        if (!file.read(reinterpret_cast<char*>(scene->worldObjects.data()), bytes)) {
            LOG_ERROR("Failed to read scene objects (truncated file?): " + filepath);
            return nullptr;
        }
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

    // chave única por combinação de shaders + cor + texturas PBR. As texturas
    // ENTRAM na chave: sem elas, materiais com os mesmos shaders/cor mas mapas
    // diferentes (chão/árvore/toco) colidiriam e todos reusariam o primeiro.
    auto& c = materialData.color;
    std::string matKey = std::string(materialData.vertexShaderPath) + "|" +
                         materialData.fragmentShaderPath + "|" + std::to_string(c.r) +
                         std::to_string(c.g) + std::to_string(c.b) + std::to_string(c.a) + "|" +
                         materialData.albedoMap + "|" + materialData.normalMap + "|" +
                         materialData.roughnessMap + "|" + materialData.metallicMap;

    auto matIt = materialCache.find(matKey);
    if (matIt == materialCache.end()) {
        auto material = createMaterial(materialData);
        if (!material) {
            LOG_ERROR("Material init failed for mesh: " + std::string(meshData.path));
            return;
        }

        materialCache[matKey] = std::move(material);
        matIt = materialCache.find(matKey);
    }

    auto meshRenderer = std::make_unique<MeshRenderer>();
    meshRenderer->setMaterial(matIt->second); // shared_ptr

    obj->setMesh(mesh);
    obj->addComponent(std::move(meshRenderer));
}

// Carrega os recortes de um sprite sheet (.ssm) em um Sprite. O .ssm lista
// "groups", cada um com "sprites" definidos por points.start/points.end em
// PIXELS, com origem no canto inferior-esquerdo (estilo OpenGL). Convertemos
// cada retângulo para UVs normalizados [0,1] usando as dimensões da textura.
// Resiliente: um arquivo ausente/ inválido só emite LOG_WARN e deixa o sprite
// sem frames (desenha a textura cheia).
static void loadSsmIntoSprite(Sprite& sprite, const std::string& ssmPath, float texW, float texH) {
    if (texW <= 0.0f || texH <= 0.0f) {
        LOG_WARN("SSM ignorado: dimensões de textura inválidas (" + ssmPath + ")");
        return;
    }
    std::ifstream file(ssmPath);
    if (!file.is_open()) {
        LOG_WARN("Não foi possível abrir o sprite sheet (.ssm): " + ssmPath);
        return;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const std::exception& e) {
        LOG_WARN("SSM inválido (" + ssmPath + "): " + e.what());
        return;
    }

    if (!j.contains("groups") || !j["groups"].is_array()) {
        LOG_WARN("SSM sem 'groups' (" + ssmPath + ")");
        return;
    }

    int groupIndex = 0;
    for (const auto& g : j["groups"]) {
        std::string name = g.value("name", std::string("group") + std::to_string(groupIndex));
        groupIndex++;
        if (!g.contains("sprites") || !g["sprites"].is_array())
            continue;

        std::vector<Sprite::Frame> frames;
        for (const auto& s : g["sprites"]) {
            if (!s.contains("points"))
                continue;
            const auto& p = s["points"];
            if (!p.contains("start") || !p.contains("end"))
                continue;

            float sx = p["start"][0].get<float>();
            float sy = p["start"][1].get<float>();
            float ex = p["end"][0].get<float>();
            float ey = p["end"][1].get<float>();

            Sprite::Frame f;
            // UVs normalizados. O .ssm já usa origem bottom-left (como o OpenGL),
            // então mapeamos y direto sem inverter.
            f.u0 = sx / texW;
            f.u1 = ex / texW;
            f.v0 = sy / texH;
            f.v1 = ey / texH;
            f.pixelWidth = ex - sx;
            f.pixelHeight = ey - sy;
            if (f.pixelWidth <= 0.0f || f.pixelHeight <= 0.0f) {
                LOG_WARN("Frame com tamanho inválido em " + ssmPath + "; ignorado");
                continue;
            }
            frames.push_back(f);
        }

        if (!frames.empty())
            sprite.addGroup(name, std::move(frames));
    }
}

void SceneLoader::loadSpriteRendererComponent(WorldObject* obj, const ComponentData& comp) {
    auto& textureData = comp.spriteRenderer.texture;
    auto& materialData = comp.spriteRenderer.material;

    float width = textureData.width * textureData.scaleFactor;
    float height = textureData.height * textureData.scaleFactor;
    auto sprite = std::make_unique<Sprite>(width, height);
    sprite->setScaleFactor(textureData.scaleFactor);

    unsigned int texID = rendererBackend->loadTexture(Yume::resolveAssetPath(textureData.path),
                                                      textureData.filterType);
    sprite->setTexture(texID);

    // Sprite sheet opcional: carrega os recortes e seleciona o primeiro frame
    // do primeiro grupo, dimensionando o quad ao tamanho desse frame.
    if (textureData.ssmPath[0] != '\0') {
        loadSsmIntoSprite(*sprite, Yume::resolveAssetPath(textureData.ssmPath), textureData.width,
                          textureData.height);
        const std::string& first = sprite->firstGroup();
        if (!first.empty())
            sprite->setFrame(first, 0);
    }

    auto material = createMaterial(materialData);
    if (!material) {
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

        auto skyboxMaterial = createMaterial(camData.skybox.material);
        if (!skyboxMaterial) {
            LOG_ERROR("Material init failed for skybox");
            return;
        }

        std::vector<std::string> faces;
        for (const auto& row : camData.skybox.cubeMapTextures) {
            faces.push_back(Yume::resolveAssetPath(row));
        }

        unsigned int cubemapID = rendererBackend->createCubemapTexture(faces);
        skybox->setTextureID(cubemapID);
        skybox->setMaterial(std::move(skyboxMaterial));
        // A malha do cubo do skybox precisa de um MeshBuffer de GPU antes de
        // configure() (feito dentro de init()); diferente das malhas de .obj,
        // o Skybox não passa pelo loadObjMesh, então criamos o buffer aqui.
        if (Mesh* skyMesh = skybox->getMesh())
            skyMesh->setMeshBuffer(rendererBackend->createMeshBuffer());
        skybox->init();
        camera->setSkybox(std::move(skybox));
    }

    obj->addComponent(std::move(camera));
}

void SceneLoader::loadTextRendererComponent(WorldObject* obj, const ComponentData& comp) {
    auto& data = comp.textRenderer;

    auto it = fontAtlasCache.find(data.font.atlasJsonPath);
    if (it == fontAtlasCache.end()) {
        auto atlas = std::make_shared<FontAtlas>();
        if (!atlas->load(Yume::resolveAssetPath(data.font.atlasJsonPath))) {
            LOG_ERROR("Failed to load font atlas: " + std::string(data.font.atlasJsonPath));
            return;
        }
        fontAtlasCache[data.font.atlasJsonPath] = atlas;
        it = fontAtlasCache.find(data.font.atlasJsonPath);
    }

    unsigned int texID =
        rendererBackend->loadTexture(Yume::resolveAssetPath(data.font.texturePath), 1);

    auto shaderExt = rendererBackend->getShaderExtension();
    auto textRenderer = std::make_unique<TextRenderer>();
    textRenderer->init(
        *rendererBackend, *it->second, texID,
        Yume::resolveAssetPath(std::string(data.material.vertexShaderPath) + shaderExt),
        Yume::resolveAssetPath(std::string(data.material.fragmentShaderPath) + shaderExt));

    auto component = std::make_unique<TextRendererComponent>();
    component->setFontAtlas(std::make_unique<FontAtlas>(*it->second));
    component->setTextRenderer(std::move(textRenderer));
    obj->addComponent(std::move(component));
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

void SceneLoader::loadRigidbodyComponent(WorldObject* obj, const ComponentData& comp) {
    auto& data = comp.rigidbody;

    auto body = std::make_unique<Rigidbody>();
    body->setMass(data.mass);
    body->setGravityScale(data.gravityScale);
    body->setRestitution(data.restitution);
    body->setFriction(data.friction);
    body->setUseGravity(data.useGravity);
    body->setVelocity(data.velocity);
    body->setAngularVelocity(data.angularVelocity);

    obj->addComponent(std::move(body));
}

void SceneLoader::loadBoxColliderComponent(WorldObject* obj, const ComponentData& comp) {
    auto& data = comp.boxCollider;

    auto collider = std::make_unique<BoxCollider>();
    collider->setHalfExtents(data.halfExtents);
    collider->setCenter(data.center);

    obj->addComponent(std::move(collider));
}

void SceneLoader::loadPlaneColliderComponent(WorldObject* obj, const ComponentData& comp) {
    auto& data = comp.planeCollider;

    auto collider = std::make_unique<PlaneCollider>();
    collider->setNormal(data.normal);
    collider->setOffset(data.offset);

    obj->addComponent(std::move(collider));
}

void SceneLoader::loadSphereColliderComponent(WorldObject* obj, const ComponentData& comp) {
    auto& data = comp.sphereCollider;

    auto collider = std::make_unique<SphereCollider>();
    collider->setRadius(data.radius);
    collider->setCenter(data.center);

    obj->addComponent(std::move(collider));
}

std::shared_ptr<Mesh> SceneLoader::loadObjMesh(const std::string& relativePath, bool shadeSmooth) {
    // Resolve once at the entry point so the cache key and the path handed to
    // tinyobj both carry the platform asset root (romfs:/ on consoles).
    const std::string filepath = Yume::resolveAssetPath(relativePath);

    auto it = meshCache.find(filepath);
    if (it != meshCache.end())
        return it->second;

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
    std::vector<float> texCoords; // UV (2 floats/vértice)
    std::vector<float> tangents;  // tangente (3 floats/vértice), para normal map

    // Processamos sempre por triângulo (grupos de 3 índices). Isso serve aos
    // dois modos de sombreamento e permite calcular a tangente por face a
    // partir das posições e dos UVs — a tangente é compartilhada pelos 3
    // vértices do triângulo (suficiente para normal mapping desindexado).
    const bool useObjNormals = shadeSmooth && !attrib.normals.empty();
    const bool hasUVs = !attrib.texcoords.empty();

    for (const auto& shape : shapes) {
        for (size_t f = 0; f + 2 < shape.mesh.indices.size(); f += 3) {
            const auto& i0 = shape.mesh.indices[f + 0];
            const auto& i1 = shape.mesh.indices[f + 1];
            const auto& i2 = shape.mesh.indices[f + 2];

            float v0[3] = {attrib.vertices[3 * i0.vertex_index + 0],
                           attrib.vertices[3 * i0.vertex_index + 1],
                           attrib.vertices[3 * i0.vertex_index + 2]};
            float v1[3] = {attrib.vertices[3 * i1.vertex_index + 0],
                           attrib.vertices[3 * i1.vertex_index + 1],
                           attrib.vertices[3 * i1.vertex_index + 2]};
            float v2[3] = {attrib.vertices[3 * i2.vertex_index + 0],
                           attrib.vertices[3 * i2.vertex_index + 1],
                           attrib.vertices[3 * i2.vertex_index + 2]};

            // Normal da face (fallback para flat shading / quando não há UVs).
            float edge1[3] = {v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]};
            float edge2[3] = {v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]};
            float faceN[3] = {edge1[1] * edge2[2] - edge1[2] * edge2[1],
                              edge1[2] * edge2[0] - edge1[0] * edge2[2],
                              edge1[0] * edge2[1] - edge1[1] * edge2[0]};
            float nlen = std::sqrt(faceN[0] * faceN[0] + faceN[1] * faceN[1] + faceN[2] * faceN[2]);
            if (nlen > 0) {
                faceN[0] /= nlen;
                faceN[1] /= nlen;
                faceN[2] /= nlen;
            }

            // UVs do triângulo (0 quando a malha não tem coordenadas).
            auto uvOf = [&](const tinyobj::index_t& idx, float out[2]) {
                if (hasUVs && idx.texcoord_index >= 0) {
                    out[0] = attrib.texcoords[2 * idx.texcoord_index + 0];
                    out[1] = attrib.texcoords[2 * idx.texcoord_index + 1];
                } else {
                    out[0] = 0.0f;
                    out[1] = 0.0f;
                }
            };
            float uv0[2], uv1[2], uv2[2];
            uvOf(i0, uv0);
            uvOf(i1, uv1);
            uvOf(i2, uv2);

            // Tangente da face a partir das posições e deltas de UV.
            // T = (deltaUV2.y * edge1 - deltaUV1.y * edge2) * r
            float du1 = uv1[0] - uv0[0], dv1 = uv1[1] - uv0[1];
            float du2 = uv2[0] - uv0[0], dv2 = uv2[1] - uv0[1];
            float denom = du1 * dv2 - du2 * dv1;
            float tangent[3] = {1.0f, 0.0f, 0.0f};
            if (hasUVs && std::fabs(denom) > 1e-8f) {
                float r = 1.0f / denom;
                tangent[0] = r * (dv2 * edge1[0] - dv1 * edge2[0]);
                tangent[1] = r * (dv2 * edge1[1] - dv1 * edge2[1]);
                tangent[2] = r * (dv2 * edge1[2] - dv1 * edge2[2]);
                float tlen = std::sqrt(tangent[0] * tangent[0] + tangent[1] * tangent[1] +
                                       tangent[2] * tangent[2]);
                if (tlen > 0) {
                    tangent[0] /= tlen;
                    tangent[1] /= tlen;
                    tangent[2] /= tlen;
                }
            }

            const tinyobj::index_t* tri[3] = {&i0, &i1, &i2};
            const float* uvs[3] = {uv0, uv1, uv2};
            for (int k = 0; k < 3; k++) {
                const auto& idx = *tri[k];
                vertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
                vertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
                vertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);

                if (useObjNormals && idx.normal_index >= 0) {
                    normals.push_back(attrib.normals[3 * idx.normal_index + 0]);
                    normals.push_back(attrib.normals[3 * idx.normal_index + 1]);
                    normals.push_back(attrib.normals[3 * idx.normal_index + 2]);
                } else {
                    normals.push_back(faceN[0]);
                    normals.push_back(faceN[1]);
                    normals.push_back(faceN[2]);
                }

                texCoords.push_back(uvs[k][0]);
                texCoords.push_back(uvs[k][1]);

                tangents.push_back(tangent[0]);
                tangents.push_back(tangent[1]);
                tangents.push_back(tangent[2]);
            }
        }
    }

    auto mesh = std::make_shared<Mesh>();
    mesh->setVertices(vertices);
    mesh->setNormals(normals);
    // Só anexa UV/tangente quando a malha de fato tem UVs; sem isso o VAO não
    // cria esses atributos e o shader recebe os defaults.
    if (hasUVs) {
        mesh->setTexCoords(texCoords);
        mesh->setTangents(tangents);
    }
    mesh->setMeshBuffer(rendererBackend->createMeshBuffer());
    mesh->configure();

    mesh->setUniqueVertexCount((int)vertices.size() / 3);
    int totalTris = 0;
    for (const auto& shape : shapes)
        totalTris += (int)shape.mesh.indices.size() / 3;
    mesh->setTriangleCount(totalTris);

    meshCache[filepath] = mesh;
    return mesh;
}

void SceneLoader::loadScriptComponent(WorldObject* obj, const ComponentData& comp) {
    std::string path = comp.script.scriptPath;
    if (path.empty()) {
        LOG_ERROR("SCRIPT component with empty path; skipping");
        return;
    }
    obj->addComponent(std::make_unique<ScriptComponent>(Yume::resolveAssetPath(path)));
}

void SceneLoader::loadWorldObjects(WorldObjectManager* manager, const CompiledScene* scene) {
    LOG_INFO("Loading " + std::to_string(scene->worldObjectCount) + " world objects");

    // Guarda os objetos criados na ordem do arquivo para resolver o parentesco
    // (por índice) numa segunda passada — o pai pode vir depois do filho.
    std::vector<WorldObject*> created;
    created.reserve(scene->worldObjectCount);

    for (uint32_t i = 0; i < scene->worldObjectCount; i++) {
        auto& woData = scene->worldObjects[i];
        auto* obj = manager->createObject();
        created.push_back(obj);

        // Carregar transform (local; o parentesco é resolvido depois)
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
            case ComponentType::TEXT_RENDERER:
                LOG_INFO("  - Loading TEXT_RENDERER component");
                loadTextRendererComponent(obj, comp);
                break;
            case ComponentType::LOD_GROUP:
                LOG_INFO("  - Loading LOD_GROUP component");
                loadLodGroupComponent(obj, comp);
                break;
            case ComponentType::SCRIPT:
                LOG_INFO("  - Loading SCRIPT component");
                loadScriptComponent(obj, comp);
                break;
            case ComponentType::RIGIDBODY:
                LOG_INFO("  - Loading RIGIDBODY component");
                loadRigidbodyComponent(obj, comp);
                break;
            case ComponentType::BOX_COLLIDER:
                LOG_INFO("  - Loading BOX_COLLIDER component");
                loadBoxColliderComponent(obj, comp);
                break;
            case ComponentType::PLANE_COLLIDER:
                LOG_INFO("  - Loading PLANE_COLLIDER component");
                loadPlaneColliderComponent(obj, comp);
                break;
            case ComponentType::SPHERE_COLLIDER:
                LOG_INFO("  - Loading SPHERE_COLLIDER component");
                loadSphereColliderComponent(obj, comp);
                break;
            default:
                break;
            }
        }
    }

    // Segunda passada: resolve parentIndex -> ponteiro, agora que todos os
    // objetos existem. Índice inválido (fora de faixa ou apontando pra si
    // mesmo) é ignorado com aviso, deixando o objeto como raiz.
    for (uint32_t i = 0; i < scene->worldObjectCount; i++) {
        int32_t p = scene->worldObjects[i].parentIndex;
        if (p < 0)
            continue; // raiz
        if (static_cast<uint32_t>(p) >= scene->worldObjectCount || static_cast<uint32_t>(p) == i) {
            LOG_WARN("WorldObject #" + std::to_string(i) + " tem parentIndex inválido (" +
                     std::to_string(p) + "); tratado como raiz");
            continue;
        }
        created[i]->setParent(created[static_cast<uint32_t>(p)]);
        LOG_INFO("WorldObject #" + std::to_string(i) + " parent -> #" + std::to_string(p));
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
