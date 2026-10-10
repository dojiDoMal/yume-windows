#ifndef CITRO3D_SHADER_PROGRAM_HPP
#define CITRO3D_SHADER_PROGRAM_HPP

#include "assets/shader_program.hpp"
#include <3ds.h>
#include <citro3d.h>
#include <string>
#include <unordered_map>

/**
 * @brief Handle nativo de um shader compilado no backend Citro3D.
 *
 * O "compilador" do 3DS (Citro3DShaderCompiler) carrega um .shbin e o parseia
 * num DVLB_s*. Como DVLB_ParseFile não copia o bytecode, o buffer original
 * (`code`) precisa viver junto do DVLB — por isso ambos ficam neste handle, que
 * é o void* retornado por ShaderAsset::getHandle().
 *
 * @see Citro3DShaderCompiler, Citro3DShaderProgram
 */
struct Citro3DShaderHandle {
    DVLB_s* dvlb = nullptr; ///< DVLB parseado do .shbin (contém os DVLE).
    u32* code = nullptr;    ///< Bytecode bruto que o DVLB referencia (dono).
};

/**
 * @brief ShaderProgram do backend Citro3D (Nintendo 3DS / PICA200).
 *
 * No OpenGL um programa é glCreateProgram + attach + link; no PICA200 o
 * equivalente é um shaderProgram_s que recebe um DVLE (vertex shader) do DVLB e
 * é ativado com C3D_BindProgram. Não há "linkagem" de fragment shader: a etapa
 * de fragmento é configurada via TexEnv no backend, não por um shader de pixel.
 *
 * Uniforms também diferem: o PICA200 não tem Uniform Buffer Objects. Os valores
 * (matrizes, vetores) são escritos diretamente nos registradores de uniform do
 * vertex shader via C3D_FVUnifMtx4x4 / C3D_FVUnifSet, usando os locais obtidos
 * por shaderInstanceGetUniformLocation. setUniformBuffer() traduz os blocos da
 * engine (ModelViewProjection, etc.) para essas escritas de uniform.
 *
 * @see ShaderProgram, ShaderProgramFactory, Citro3DShaderCompiler
 */
class Citro3DShaderProgram : public ShaderProgram {
  private:
    shaderProgram_s program;   ///< Programa de shader do PICA200.
    bool initialized = false;  ///< @c true após shaderProgramInit + SetVsh.
    DVLB_s* vshDvlb = nullptr; ///< DVLB do vertex shader anexado (não-dono).

    // Locais dos uniforms do vertex shader, resolvidos por nome no link().
    // -1 quando o uniform não existe no shader (uniform opcional).
    int8_t uLocProjection = -1;
    int8_t uLocModelView = -1;
    int8_t uLocModel = -1;

  public:
    Citro3DShaderProgram() = default;
    ~Citro3DShaderProgram() override;

    bool attachShader(const ShaderAsset& shader) override;
    bool link() override;
    void use() override;
    void setUniformBuffer(const char* name, const void* data, size_t size) override;
    void* getHandle() const override { return const_cast<shaderProgram_s*>(&program); }
    bool isValid() const override { return initialized; }
};

#endif // CITRO3D_SHADER_PROGRAM_HPP
