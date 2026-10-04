#ifndef SHADER_ASSET_HPP
#define SHADER_ASSET_HPP

#include "asset.hpp"
#include "shader_compiler.hpp"
#include <memory>

/**
 * @brief Asset que representa um único shader carregado de arquivo e compilado.
 *
 * Herda de Asset, portanto segue o ciclo de vida load()/unload(). Ao carregar,
 * lê o código-fonte do caminho informado e o compila usando o ShaderCompiler
 * configurado, guardando o handle nativo resultante.
 *
 * Defina o compilador com setShaderCompiler() antes de chamar load(); em geral
 * ele vem da ShaderCompilerFactory. Vários ShaderAsset podem depois ser
 * combinados em um ShaderProgram.
 *
 * @see Asset, ShaderCompiler, ShaderProgram
 */
class ShaderAsset : public Asset {
  private:
    ShaderType shaderType;                      ///< Estágio do pipeline deste shader.
    void* shaderHandle = nullptr;               ///< Handle nativo do shader compilado.
    bool isCompiled = false;                    ///< @c true após compilação bem-sucedida.
    std::unique_ptr<ShaderCompiler> compiler;   ///< Compilador específico do backend.

  public:
    /**
     * @brief Constrói o asset a partir de um caminho e de um estágio de shader.
     * @param path Caminho do arquivo de código-fonte do shader.
     * @param type Estágio do pipeline (vertex, fragment, etc.).
     */
    ShaderAsset(const std::string& path, ShaderType type);

    /** @brief Libera o shader nativo ao destruir o asset. */
    ~ShaderAsset() override { unload(); }

    /**
     * @brief Lê o código-fonte do disco e o compila via ShaderCompiler.
     * @return @c true se carregou e compilou com sucesso.
     */
    bool load() override;

    /** @brief Libera o shader nativo e zera o estado de compilação. */
    void unload() override;

    /** @brief Retorna o handle nativo do shader compilado (ou @c nullptr). */
    void* getHandle() const { return shaderHandle; }

    /** @brief Retorna o estágio de pipeline deste shader. */
    ShaderType getType() const { return shaderType; }

    /**
     * @brief Define o compilador usado em load(). Deve ser chamado antes dele.
     * @see ShaderCompilerFactory
     */
    void setShaderCompiler(std::unique_ptr<ShaderCompiler>);
};

#endif // SHADERASSET_HPP
