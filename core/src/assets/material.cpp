#define CLASS_NAME "Material"
#include "log_macros.hpp"

#include "assets/material.hpp"
#include "color.hpp"
#include "components/light.hpp"

Material::Material() {}

bool Material::init() {
    // O fragment shader é opcional: no PICA200 (3DS) ele não existe (a etapa de
    // fragmento é feita por TexEnv no backend), então o scene_loader não cria um
    // fragmentShader nessa plataforma. As demais plataformas sempre o fornecem.
    if (!vertexShader || !shaderProgram) {
        return false;
    }

    if (!vertexShader->load()) {
        return false;
    }
    if (fragmentShader && !fragmentShader->load()) {
        return false;
    }

    if (!shaderProgram->attachShader(*vertexShader)) {
        return false;
    }
    if (fragmentShader && !shaderProgram->attachShader(*fragmentShader)) {
        return false;
    }

    if (!shaderProgram->link()) {
        return false;
    }

    setBaseColor(baseColor);
    return true;
}

void Material::use() {
    if (shaderProgram) {
        shaderProgram->use();
    }
}

void Material::setBaseColor(const ColorRGBA color) {
    baseColor = color;
    if (shaderProgram) {
        shaderProgram->setUniformBuffer("MaterialData", &baseColor, sizeof(baseColor));
    }
}

void Material::applyLight(const Light& light) {
    if (!shaderProgram) {
        LOG_WARN("Can not apply light on material with null shaderProgram");
        return;
    }

    if (light.getType() == LightType::DIRECTIONAL) {
        struct LightDataBuffer {
            Vector3 direction;
            float padding;
            ColorRGBA color;
            float intensity;
        } lightData = {light.getDirection(), 0.0f, light.getColor(), light.getIntensity()};

        shaderProgram->setUniformBuffer("LightData", &lightData, sizeof(lightData));
    }
}