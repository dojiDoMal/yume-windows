#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "color.hpp"
#include "components/light.hpp"
#include "shader_asset.hpp"
#include "shader_program.hpp"
#include <memory>

class Material {
  private:
    std::unique_ptr<ShaderAsset> vertexShader;
    std::unique_ptr<ShaderAsset> fragmentShader;
    std::unique_ptr<ShaderProgram> shaderProgram;
    std::unique_ptr<ShaderProgram> shaderProgramSingle;
    ColorRGBA baseColor = COLOR::RED;
    bool instancingEnabled = true;

  public:
    Material();

    bool init();
    void use();
    void setBaseColor(const ColorRGBA color);
    void applyLight(const Light& light);

    void setVertexShader(std::unique_ptr<ShaderAsset> shader) { vertexShader = std::move(shader); }

    void setFragmentShader(std::unique_ptr<ShaderAsset> shader) {
        fragmentShader = std::move(shader);
    }

    ShaderProgram* getShaderProgram() const { return shaderProgram.get(); }
    void setShaderProgram(std::unique_ptr<ShaderProgram> program) {
        shaderProgram = std::move(program);
    }

    bool isInstancingEnabled() const { return instancingEnabled; }
    void setInstancingEnabled(bool enabled) { instancingEnabled = enabled; }

    void setShaderProgramSingle(std::unique_ptr<ShaderProgram> program) {
        shaderProgramSingle = std::move(program);
    }
    ShaderProgram* getShaderProgramSingle() const {
        return shaderProgramSingle ? shaderProgramSingle.get() : shaderProgram.get();
    }
};

#endif // MATERIAL_HPP