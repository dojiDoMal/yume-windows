#ifndef CITRO3D_SHADER_COMPILER_HPP
#define CITRO3D_SHADER_COMPILER_HPP

#include "assets/shader_compiler.hpp"

/**
 * @brief "Compilador" de shaders do backend Citro3D (Nintendo 3DS).
 *
 * Ao contrário do OpenGL/Vulkan, o PICA200 não compila shaders em tempo de
 * execução. Os shaders são escritos em assembly PICA200 e montados em
 * build-time pela ferramenta `picasso` (devkitPro), gerando um binário .shbin.
 * Em runtime só resta carregar esse binário e parseá-lo em um DVLB_s*
 * (DVLB_ParseFile), que é o handle devolvido por compile().
 *
 * Por isso este "compilador" na prática apenas lê o .shbin do disco e extrai o
 * DVLB. O estágio (ShaderType) é informativo aqui: o DVLB pode conter tanto o
 * vertex quanto o geometry shader; o ShaderProgram escolhe o DVLE adequado.
 *
 * @todo O pipeline de build ainda não gera .shbin. É preciso adicionar ao CMake
 *       um passo picasso (*.pica -> *.shbin), análogo ao dxc/spirv-cross das
 *       outras plataformas, e definir a extensão de saída para o 3DS.
 * @todo Alternativa comum no 3DS: embutir o .shbin no binário via `bin2s`
 *       (símbolos *_shbin / *_shbin_size) e usar DVLB_ParseFile sobre eles em
 *       vez de ler de romfs. Decidir qual caminho adotar junto com o CMake.
 *
 * @see ShaderCompiler, ShaderCompilerFactory, Citro3DShaderProgram
 */
class Citro3DShaderCompiler : public ShaderCompiler {
  public:
    /**
     * @brief Carrega um .shbin do caminho informado e o parseia em um DVLB.
     * @param source    Caminho do arquivo .shbin montado pelo picasso.
     * @param type      Estágio do pipeline (informativo; ver nota da classe).
     * @param outHandle Saída: recebe o DVLB_s* resultante.
     * @return @c true se o arquivo foi lido e parseado com sucesso.
     */
    bool compile(const std::string& source, ShaderType type, void** outHandle) override;

    /** @brief Libera o DVLB_s* (DVLB_Free) e a memória do bytecode associada. */
    void destroy(void* handle) override;

    /** @brief Indica se o handle aponta para um DVLB válido. */
    bool isValid(void* handle) override;
};

#endif // CITRO3D_SHADER_COMPILER_HPP
