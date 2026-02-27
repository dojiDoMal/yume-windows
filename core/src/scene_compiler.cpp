#include "color.hpp"
#include "scene_format.hpp"
#include "vector3.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using json = nlohmann::json;

void compileCamera(CompiledScene& scene, const json& cam) {
    for (int i = 0; i < 4; i++)
        scene.camera.background_color[i] = cam["background_color"][i];
    scene.camera.fov = cam["fov"];
    for (int i = 0; i < 2; i++)
        scene.camera.view_rect[i] = cam["view_rect"][i];
    for (int i = 0; i < 3; i++)
        scene.camera.position[i] = cam["position"][i];
    
    scene.camera.orthographic = cam.value("orthographic", false);
    scene.camera.orthoSize = cam.value("orthoSize", 5.0f);

    if (cam.contains("skybox")) {
        scene.camera.hasSkybox = true;
        auto& skybox = cam["skybox"];

        std::string vertPath = skybox["material"]["vertexShaderPath"];
        std::string fragPath = skybox["material"]["fragmentShaderPath"];

        std::snprintf(scene.camera.skybox.material.vertexShaderPath,
                      sizeof(scene.camera.skybox.material.vertexShaderPath), "%s",
                      vertPath.c_str());
        std::snprintf(scene.camera.skybox.material.fragmentShaderPath,
                      sizeof(scene.camera.skybox.material.fragmentShaderPath), "%s",
                      fragPath.c_str());

        for (int i = 0; i < 6; i++) {
            std::string texPath = skybox["cubeMapTextures"][i];
            std::snprintf(scene.camera.skybox.cubeMapTextures[i],
                          sizeof(scene.camera.skybox.cubeMapTextures[i]), "%s", texPath.c_str());
        }
    } else {
        scene.camera.hasSkybox = false;
    }
}

void compileLights(CompiledScene& scene, const json& j) {
    scene.lightCount = 0;
    if (!j.contains("lights"))
        return;

    auto& lights = j["lights"];
    scene.lightCount = lights.size();
    for (size_t i = 0; i < lights.size() && i < 32; i++) {
        std::string type = lights[i]["type"];
        if (type == "DIRECTIONAL")
            scene.lights[i].type = 0;
        else if (type == "POINT")
            scene.lights[i].type = 1;
        else if (type == "SPOT")
            scene.lights[i].type = 2;
        else
            scene.lights[i].type = 0;

        Vector3 direction;
        direction.x = lights[i]["direction"][0];
        direction.y = lights[i]["direction"][1];
        direction.z = lights[i]["direction"][2];
        scene.lights[i].direction = direction;
    }
}

void compileMeshRenderer(ComponentData& compData, const json& comp) {
    compData.type = ComponentType::MESH_RENDERER;

    std::string objPath = comp["mesh"]["path"];
    std::string vertPath = comp["material"]["vertexShaderPath"];
    std::string fragPath = comp["material"]["fragmentShaderPath"];
    std::array<float, 4> color = comp["material"]["color"];

    compData.meshRenderer.mesh.shadeSmooth = comp["mesh"].value("shadeSmooth", true);

    std::snprintf(compData.meshRenderer.mesh.path, sizeof(compData.meshRenderer.mesh.path), "%s",
                  objPath.c_str());
    std::snprintf(compData.meshRenderer.material.vertexShaderPath,
                  sizeof(compData.meshRenderer.material.vertexShaderPath), "%s", vertPath.c_str());
    std::snprintf(compData.meshRenderer.material.fragmentShaderPath,
                  sizeof(compData.meshRenderer.material.fragmentShaderPath), "%s",
                  fragPath.c_str());

    compData.meshRenderer.material.color = {color[0], color[1], color[2], color[3]};
}

void compileSpriteRenderer(ComponentData& compData, const json& comp) {
    compData.type = ComponentType::SPRITE_RENDERER;

    std::string texPath = comp["texture"]["path"];
    float scaleFactor = comp["texture"].value("scaleFactor", 1.0f);
    std::string filter = comp["texture"].value("filterType", "NEAREST");

    std::string vertPath = comp["material"]["vertexShaderPath"];
    std::string fragPath = comp["material"]["fragmentShaderPath"];
    std::array<float, 4> color = comp["material"]["color"];

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

    compData.spriteRenderer.material.color = {color[0], color[1], color[2], color[3]};
}

void compileTransform(ComponentData& compData, const json& comp) {
    compData.type = ComponentType::TRANSFORM;

    compData.transform.position.x = comp["position"][0];
    compData.transform.position.y = comp["position"][1];
    compData.transform.position.z = comp["position"][2];

    compData.transform.rotation.x = comp["rotation"][0];
    compData.transform.rotation.y = comp["rotation"][1];
    compData.transform.rotation.z = comp["rotation"][2];

    compData.transform.scale.x = comp["scale"][0];
    compData.transform.scale.y = comp["scale"][1];
    compData.transform.scale.z = comp["scale"][2];
}

void compileGameObjects(CompiledScene& scene, const json& j) {
    scene.gameObjectCount = 0;
    if (!j.contains("gameObjects"))
        return;

    auto& gameObjects = j["gameObjects"];
    scene.gameObjectCount = gameObjects.size();
    for (size_t i = 0; i < gameObjects.size() && i < 32; i++) {
        auto& go = gameObjects[i];
        auto& goData = scene.gameObjects[i];

        goData.componentCount = 0;

        if (go.contains("components")) {
            auto& components = go["components"];
            goData.componentCount = components.size();

            for (size_t j = 0; j < components.size() && j < 8; j++) {
                auto& comp = components[j];
                std::string type = comp["type"];

                if (type == "MESH_RENDERER") {
                    compileMeshRenderer(goData.components[j], comp);
                } else if (type == "TRANSFORM") {
                    compileTransform(goData.components[j], comp);
                } else if (type == "SPRITE_RENDERER") {
                    compileSpriteRenderer(goData.components[j], comp);
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    std::ifstream input(argv[1]);
    json j = json::parse(input);

    CompiledScene scene;

    compileCamera(scene, j["camera"]);
    compileLights(scene, j);
    compileGameObjects(scene, j);

    std::ofstream output(argv[2], std::ios::binary);
    output.write(reinterpret_cast<char*>(&scene), sizeof(CompiledScene));

    return 0;
}