#ifndef MESH_BUFFER_FACTORY_HPP
#define MESH_BUFFER_FACTORY_HPP

#include "graphics_api.hpp"
#include "assets/mesh_buffer.hpp"
#include <memory>

/**
 * @brief Fábrica que cria o MeshBuffer adequado ao backend gráfico.
 *
 * Mesmo padrão das fábricas de shader: devolve a implementação de MeshBuffer
 * correspondente à API escolhida, isolando o resto do engine dos detalhes de
 * cada backend.
 *
 * @see MeshBuffer, GraphicsAPI
 */
class MeshBufferFactory {
  public:
    /**
     * @brief Cria um buffer de malha para a API informada.
     * @param api     Backend gráfico desejado.
     * @param context Estado opaco do backend, quando exigido (ex.: Vulkan,
     *                DirectX 12). @c nullptr caso contrário.
     * @return Um MeshBuffer pronto para uso, ou @c nullptr se a API não for
     *         suportada na plataforma atual.
     */
    static std::unique_ptr<MeshBuffer> create(GraphicsAPI api, void* context = nullptr);
};

#endif // MESHBUFFERFACTORY_HPP
