#ifndef SHADER_PROGRAM_FACTORY_HPP
#define SHADER_PROGRAM_FACTORY_HPP

#include "graphics_api.hpp"
#include "assets/shader_program.hpp"
#include <memory>

/**
 * @brief Fábrica que cria o ShaderProgram adequado ao backend gráfico.
 *
 * Equivalente à ShaderCompilerFactory, mas para programas linkados: devolve a
 * implementação de ShaderProgram correspondente à API escolhida.
 *
 * @see ShaderProgram, GraphicsAPI
 */
class ShaderProgramFactory {
  public:
    /**
     * @brief Cria um programa de shader para a API informada.
     * @param api     Backend gráfico desejado.
     * @param context Estado opaco do backend, quando exigido pela API
     *                (ex.: Vulkan, DirectX 12). @c nullptr caso contrário.
     * @return Um ShaderProgram pronto para uso, ou @c nullptr se a API não for
     *         suportada na plataforma atual.
     */
    static std::unique_ptr<ShaderProgram> create(GraphicsAPI api, void* context = nullptr);
};

#endif // SHADERPROGRAMFACTORY_HPP
