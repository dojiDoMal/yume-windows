#ifndef SHADER_COMPILER_FACTORY_HPP
#define SHADER_COMPILER_FACTORY_HPP

#include "graphics_api.hpp"
#include "assets/shader_compiler.hpp"
#include <memory>

/**
 * @brief Fábrica que cria o ShaderCompiler adequado ao backend gráfico.
 *
 * Isola o resto do engine das implementações específicas de cada API: basta
 * informar o GraphicsAPI desejado e a fábrica devolve a implementação correta
 * para a plataforma atual (algumas opções só existem em certas plataformas,
 * controladas por macros de compilação).
 *
 * @see ShaderCompiler, GraphicsAPI
 */
class ShaderCompilerFactory {
  public:
    /**
     * @brief Cria um compilador de shaders para a API informada.
     *
     * @param api     Backend gráfico desejado.
     * @param context Ponteiro opaco para o estado do backend, exigido por
     *                algumas APIs (ex.: VulkanRendererBackend para Vulkan,
     *                D3D12RendererBackend para DirectX 12). Deixe @c nullptr
     *                para backends que não precisam dele, como OpenGL.
     * @return Um ShaderCompiler pronto para uso, ou @c nullptr se a API não
     *         for suportada na plataforma atual.
     *
     * @code
     * auto compiler = ShaderCompilerFactory::create(GraphicsAPI::VULKAN, backend);
     * @endcode
     */
    static std::unique_ptr<ShaderCompiler> create(GraphicsAPI api, void* context = nullptr);
};

#endif // SHADERCOMPILERFACTORY_HPP
