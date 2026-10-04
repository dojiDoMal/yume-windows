/**
 * @file scene_compiler.cpp
 * @brief Ferramenta que compila uma cena de JSON para o formato binário .scnb.
 *
 * Faz o caminho inverso do SceneLoader: lê a descrição textual da cena (JSON),
 * preenche as structs POD de scene_format.hpp (objetos, materiais, luzes,
 * câmeras, grupos de LOD, etc.) e grava o resultado como um arquivo binário
 * compacto que o engine carrega em tempo de execução. As funções @c compile*
 * convertem cada tipo de componente do JSON para seu ComponentData.
 *
 * @see scene_format.hpp, SceneLoader
 */
#include "color.hpp"
#include "scene/scene_format.hpp"
#include "math/vector3.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "assets/stb_image.h"

// TODO: Cores estão sem validação de faixa/tamanho pra cores. Se o JSON  
// tiver menos de 4 elementos ou valores fora de 0–255, operator[] do 
// std::array não checa  limites e valores acima de 255 passam de 1.0. 
// Se quiser robustez, dá pra usar .at() ou um clamp.

using json = nlohmann::json;

void compileLodGroup(ComponentData& compData, const json& comp) {
    compData.type = ComponentType::LOD_GROUP;

    auto& levels = comp["levels"];
    compData.lodGroup.levelCount = (uint8_t)std::min(levels.size(), (size_t)MAX_LOD_LEVELS);

    for (size_t i = 0; i < compData.lodGroup.levelCount; i++) {
        auto& lvl = levels[i];
        std::string path = lvl["mesh"]["path"];
        std::snprintf(compData.lodGroup.levels[i].mesh.path,
                      sizeof(compData.lodGroup.levels[i].mesh.path), "%s", path.c_str());
        compData.lodGroup.levels[i].mesh.shadeSmooth = lvl["mesh"].value("shadeSmooth", true);
        compData.lodGroup.levels[i].screenSpaceThreshold = lvl["screenSpaceThreshold"];
    }

    std::string vertPath = comp["material"]["vertexShaderPath"];
    std::string fragPath = comp["material"]["fragmentShaderPath"];
    std::array<int, 4> color = comp["material"]["color"];
    std::snprintf(compData.lodGroup.material.vertexShaderPath,
                  sizeof(compData.lodGroup.material.vertexShaderPath), "%s", vertPath.c_str());
    std::snprintf(compData.lodGroup.material.fragmentShaderPath,
                  sizeof(compData.lodGroup.material.fragmentShaderPath), "%s", fragPath.c_str());
    compData.lodGroup.material.color = {color[0]/255.0f, color[1]/255.0f, color[2]/255.0f, color[3]/255.0f};
}

void compileMeshRenderer(ComponentData& compData, const json& comp) {
    compData.type = ComponentType::MESH_RENDERER;

    std::string objPath = comp["mesh"]["path"];
    std::string vertPath = comp["material"]["vertexShaderPath"];
    std::string fragPath = comp["material"]["fragmentShaderPath"];
    std::array<int, 4> color = comp["material"]["color"];

    compData.meshRenderer.mesh.shadeSmooth = comp["mesh"].value("shadeSmooth", true);

    std::snprintf(compData.meshRenderer.mesh.path, sizeof(compData.meshRenderer.mesh.path), "%s",
                  objPath.c_str());
    std::snprintf(compData.meshRenderer.material.vertexShaderPath,
                  sizeof(compData.meshRenderer.material.vertexShaderPath), "%s", vertPath.c_str());
    std::snprintf(compData.meshRenderer.material.fragmentShaderPath,
                  sizeof(compData.meshRenderer.material.fragmentShaderPath), "%s",
                  fragPath.c_str());

    compData.meshRenderer.material.color = {color[0]/255.0f, color[1]/255.0f, color[2]/255.0f, color[3]/255.0f};
}

void compileSpriteRenderer(ComponentData& compData, const json& comp) {
    compData.type = ComponentType::SPRITE_RENDERER;

    std::string texPath = comp["texture"]["path"];
    float scaleFactor = comp["texture"].value("scaleFactor", 1.0f);
    std::string filter = comp["texture"].value("filterType", "NEAREST");

    std::string vertPath = comp["material"]["vertexShaderPath"];
    std::string fragPath = comp["material"]["fragmentShaderPath"];
    std::array<int, 4> color = comp["material"]["color"];

    int width, height, channels;
    if (!stbi_info(texPath.c_str(), &width, &height, &channels)) {
        std::cerr << "Failed to read texture info: " << texPath << std::endl;
        width = height = 1;
    }

    std::snprintf(compData.spriteRenderer.texture.path,
                  sizeof(compData.spriteRenderer.texture.path), "%s", texPath.c_str());

    compData.spriteRenderer.texture.width = static_cast<float>(width);
    compData.spriteRenderer.texture.height = static_cast<float>(height);
    compData.spriteRenderer.texture.scaleFactor = scaleFactor;
    compData.spriteRenderer.texture.filterType = (filter == "LINEAR") ? 1 : 0;

    std::snprintf(compData.spriteRenderer.material.vertexShaderPath,
                  sizeof(compData.spriteRenderer.material.vertexShaderPath), "%s",
                  vertPath.c_str());
    std::snprintf(compData.spriteRenderer.material.fragmentShaderPath,
                  sizeof(compData.spriteRenderer.material.fragmentShaderPath), "%s",
                  fragPath.c_str());

    compData.spriteRenderer.material.color = {color[0]/255.0f, color[1]/255.0f, color[2]/255.0f, color[3]/255.0f};
}

void compileCamera(ComponentData& compData, const json& comp) {
    compData.type = ComponentType::CAMERA;

    for (int i = 0; i < 4; i++)
        compData.camera.background_color[i] = comp["background_color"][i].get<float>()/255.0f;

    compData.camera.fov = comp["fov"];

    for (int i = 0; i < 2; i++)
        compData.camera.view_rect[i] = comp["view_rect"][i];

    compData.camera.orthographic = comp.value("orthographic", false);
    compData.camera.orthoSize = comp.value("orthoSize", 5.0f);

    if (comp.contains("skybox")) {
        compData.camera.hasSkybox = true;
        auto& skybox = comp["skybox"];

        std::string vertPath = skybox["material"]["vertexShaderPath"];
        std::string fragPath = skybox["material"]["fragmentShaderPath"];

        std::snprintf(compData.camera.skybox.material.vertexShaderPath,
                      sizeof(compData.camera.skybox.material.vertexShaderPath), "%s",
                      vertPath.c_str());
        std::snprintf(compData.camera.skybox.material.fragmentShaderPath,
                      sizeof(compData.camera.skybox.material.fragmentShaderPath), "%s",
                      fragPath.c_str());

        for (int i = 0; i < 6; i++) {
            std::string texPath = skybox["cubeMapTextures"][i];
            std::snprintf(compData.camera.skybox.cubeMapTextures[i],
                          sizeof(compData.camera.skybox.cubeMapTextures[i]), "%s", texPath.c_str());
        }
    } else {
        compData.camera.hasSkybox = false;
    }
}

void compileTextRenderer(ComponentData& compData, const json& comp) {
    compData.type = ComponentType::TEXT_RENDERER;

    std::string fontType = comp.value("fontType", "MSDF");
    compData.textRenderer.fontType = FontType::MSDF;

    std::string atlasPath = comp["font"]["atlasJsonPath"];
    std::string texPath = comp["font"]["texturePath"];
    std::string vertPath = comp["material"]["vertexShaderPath"];
    std::string fragPath = comp["material"]["fragmentShaderPath"];

    std::snprintf(compData.textRenderer.font.atlasJsonPath,
                  sizeof(compData.textRenderer.font.atlasJsonPath), "%s", atlasPath.c_str());
    std::snprintf(compData.textRenderer.font.texturePath,
                  sizeof(compData.textRenderer.font.texturePath), "%s", texPath.c_str());
    std::snprintf(compData.textRenderer.material.vertexShaderPath,
                  sizeof(compData.textRenderer.material.vertexShaderPath), "%s", vertPath.c_str());
    std::snprintf(compData.textRenderer.material.fragmentShaderPath,
                  sizeof(compData.textRenderer.material.fragmentShaderPath), "%s",
                  fragPath.c_str());
}

void compileLight(ComponentData& compData, const json& comp) {
    compData.type = ComponentType::LIGHT;

    std::string lightType = comp["lightType"];
    if (lightType == "DIRECTIONAL")
        compData.light.lightType = 0;
    else if (lightType == "POINT")
        compData.light.lightType = 1;
    else if (lightType == "SPOT")
        compData.light.lightType = 2;
    else
        compData.light.lightType = 0;

    compData.light.direction.x = comp["direction"][0];
    compData.light.direction.y = comp["direction"][1];
    compData.light.direction.z = comp["direction"][2];

    compData.light.intensity = comp["intensity"];

    for (int i = 0; i < 4; i++)
        compData.light.color[i] = comp["color"][i].get<float>()/255.0f;
}

void compileWorldObjects(CompiledScene& scene, const json& j) {
    scene.worldObjectCount = 0;
    scene.worldObjects.clear();
    if (!j.contains("worldObjects"))
        return;

    auto& worldObjects = j["worldObjects"];
    scene.worldObjectCount = std::min(worldObjects.size(), (size_t)MAX_WORLD_OBJECTS);
    // Zero-initialize each element so unused component slots / padding are
    // deterministic (matches the previous value-initialized fixed array).
    scene.worldObjects.assign(scene.worldObjectCount, WorldObjectData{});

    for (size_t i = 0; i < worldObjects.size() && i < MAX_WORLD_OBJECTS; i++) {
        auto& wo = worldObjects[i];
        auto& woData = scene.worldObjects[i];

        // Transform (sempre presente)
        if (wo.contains("transform")) {
            auto& transform = wo["transform"];
            woData.position.x = transform["position"][0];
            woData.position.y = transform["position"][1];
            woData.position.z = transform["position"][2];

            woData.rotation.x = transform["rotation"][0];
            woData.rotation.y = transform["rotation"][1];
            woData.rotation.z = transform["rotation"][2];

            woData.scale.x = transform["scale"][0];
            woData.scale.y = transform["scale"][1];
            woData.scale.z = transform["scale"][2];
        } else {
            // Valores padrão
            woData.position = {0.0f, 0.0f, 0.0f};
            woData.rotation = {0.0f, 0.0f, 0.0f};
            woData.scale = {1.0f, 1.0f, 1.0f};
        }

        // Componentes
        woData.componentCount = 0;
        if (wo.contains("components")) {
            auto& components = wo["components"];
            woData.componentCount = components.size();

            for (size_t j = 0; j < components.size() && j < 8; j++) {
                auto& comp = components[j];
                std::string type = comp["type"];

                if (type == "MESH_RENDERER") {
                    compileMeshRenderer(woData.components[j], comp);
                } else if (type == "SPRITE_RENDERER") {
                    compileSpriteRenderer(woData.components[j], comp);
                } else if (type == "CAMERA") {
                    compileCamera(woData.components[j], comp);
                } else if (type == "LIGHT") {
                    compileLight(woData.components[j], comp);
                } else if (type == "TEXT_RENDERER") {
                    compileTextRenderer(woData.components[j], comp);
                } else if (type == "LOD_GROUP") {
                    compileLodGroup(woData.components[j], comp);
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input.scn> <output.scnb>" << std::endl;
        return 1;
    }

    std::ifstream input(argv[1]);
    if (!input.is_open()) {
        std::cerr << "Failed to open input file: " << argv[1] << std::endl;
        return 1;
    }

    json j = json::parse(input);

    auto scene = std::make_unique<CompiledScene>(); // <- heap
    compileWorldObjects(*scene, j);

    std::ofstream output(argv[2], std::ios::binary);
    if (!output.is_open()) {
        std::cerr << "Failed to open output file: " << argv[2] << std::endl;
        return 1;
    }

    // Write: [SceneHeader][worldObjectCount x WorldObjectData]. Only the objects
    // actually present are written, so the file scales with the object count.
    SceneHeader header{SCENE_MAGIC, scene->worldObjectCount};
    output.write(reinterpret_cast<const char*>(&header), sizeof(header));

    if (scene->worldObjectCount > 0) {
        output.write(reinterpret_cast<const char*>(scene->worldObjects.data()),
                     static_cast<std::streamsize>(scene->worldObjectCount) *
                         sizeof(WorldObjectData));
    }

    std::cout << "Scene compiled successfully: " << scene->worldObjectCount << " world objects"
              << std::endl;

    return 0;
}
