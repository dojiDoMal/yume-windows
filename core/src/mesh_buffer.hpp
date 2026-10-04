#ifndef MESH_BUFFER_HPP
#define MESH_BUFFER_HPP

#include <vector>

/**
 * @brief Buffer de malha na GPU, independente de backend.
 *
 * Abstrai os buffers nativos (VBO/VAO no OpenGL, buffers no Vulkan/DirectX, etc.)
 * que guardam vértices e normais prontos para desenho. Cada backend fornece sua
 * implementação; instancie via MeshBufferFactory. É usado internamente por Mesh.
 *
 * @see MeshBufferFactory, Mesh
 */
class MeshBuffer {
  public:
    virtual ~MeshBuffer() = default;

    /**
     * @brief Cria os buffers na GPU a partir dos dados da malha.
     * @param vertices Posições dos vértices (intercaladas conforme o layout).
     * @param normals  Normais dos vértices.
     * @return @c true se os buffers foram criados com sucesso.
     */
    virtual bool createBuffers(const std::vector<float>& vertices,
                               const std::vector<float>& normals) = 0;

    /** @brief Ativa o buffer para as próximas chamadas de desenho. */
    virtual void bind() = 0;
    /** @brief Desativa o buffer. */
    virtual void unbind() = 0;
    /** @brief Libera os recursos de GPU associados. */
    virtual void destroy() = 0;
    /** @brief Retorna o handle nativo do buffer (uso específico de backend). */
    virtual void* getHandle() const = 0;
};

#endif // MESHBUFFER_HPP
