#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "color.hpp"
#include "light.hpp"
#include "shader_asset.hpp"
#include "shader_program.hpp"
#include <memory>

class Material {
  private:
    std::unique_ptr<ShaderAsset> vertexShader;
    std::unique_ptr<ShaderAsset> fragmentShader;
    std::unique_ptr<ShaderProgram> shaderProgram;
    ColorRGBA baseColor = COLOR::GREEN;

  public:
    Material();

    bool init();
    void use();
    void setBaseColor(const ColorRGBA color);
    void applyLight(const Light light);

    void setVertexShader(std::unique_ptr<ShaderAsset> shader) { vertexShader = std::move(shader); }

    void setFragmentShader(std::unique_ptr<ShaderAsset> shader) {
        fragmentShader = std::move(shader);
    }

    ShaderProgram* getShaderProgram() const { return shaderProgram.get(); }
    void setShaderProgram(std::unique_ptr<ShaderProgram> program) {
        shaderProgram = std::move(program);
    }
};

#endif // MATERIAL_HPP