#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "color.hpp"
#include "components/light.hpp"
#include "assets/shader_asset.hpp"
#include "assets/shader_program.hpp"
#include <memory>

/**
 * @brief Define a aparência de uma superfície: shaders, cor e iluminação.
 *
 * Combina os shaders de vértice e fragmento num ShaderProgram linkado e guarda
 * parâmetros de aparência (cor base, luz). Mantém dois programas: um para
 * desenho instanciado (vários objetos de uma vez) e um "single" para desenho
 * individual — getShaderProgramSingle() cai no instanciado quando o single não
 * existe. Chame init() uma vez e use() antes de desenhar.
 *
 * @see ShaderProgram, ShaderAsset, Light
 */
class Material {
  private:
    std::unique_ptr<ShaderAsset> vertexShader;          ///< Shader de vértice.
    std::unique_ptr<ShaderAsset> fragmentShader;        ///< Shader de fragmento.
    std::unique_ptr<ShaderProgram> shaderProgram;       ///< Programa para desenho instanciado.
    std::unique_ptr<ShaderProgram> shaderProgramSingle; ///< Programa para desenho individual.
    ColorRGBA baseColor = COLOR::RED;                   ///< Cor base da superfície.
    bool instancingEnabled = true;                      ///< Se o material usa instancing.

  public:
    Material();

    /**
     * @brief Linka os shaders e prepara o material para uso.
     * @return @c true em caso de sucesso.
     */
    bool init();
    /** @brief Ativa o programa de shader deste material para desenho. */
    void use();
    /** @brief Define a cor base da superfície. */
    void setBaseColor(const ColorRGBA color);
    /** @brief Envia os parâmetros de uma luz para o shader. */
    void applyLight(const Light& light);

    /** @brief Define o shader de vértice (assume a posse). */
    void setVertexShader(std::unique_ptr<ShaderAsset> shader) { vertexShader = std::move(shader); }

    /** @brief Define o shader de fragmento (assume a posse). */
    void setFragmentShader(std::unique_ptr<ShaderAsset> shader) {
        fragmentShader = std::move(shader);
    }

    /** @brief Retorna o programa de shader (instanciado). */
    ShaderProgram* getShaderProgram() const { return shaderProgram.get(); }
    /** @brief Define o programa de shader instanciado (assume a posse). */
    void setShaderProgram(std::unique_ptr<ShaderProgram> program) {
        shaderProgram = std::move(program);
    }

    /** @brief Indica se o instancing está habilitado. */
    bool isInstancingEnabled() const { return instancingEnabled; }
    /** @brief Habilita ou desabilita o instancing. */
    void setInstancingEnabled(bool enabled) { instancingEnabled = enabled; }

    /** @brief Define o programa de shader para desenho individual (assume a posse). */
    void setShaderProgramSingle(std::unique_ptr<ShaderProgram> program) {
        shaderProgramSingle = std::move(program);
    }
    /**
     * @brief Retorna o programa para desenho individual.
     * @return O programa "single"; se não houver, cai no programa instanciado.
     */
    ShaderProgram* getShaderProgramSingle() const {
        return shaderProgramSingle ? shaderProgramSingle.get() : shaderProgram.get();
    }
};

#endif // MATERIAL_HPP
