#ifndef CITRO3D_MESH_BUFFER_HPP
#define CITRO3D_MESH_BUFFER_HPP

#include "assets/mesh_buffer.hpp"
#include <citro3d.h>

/**
 * @brief MeshBuffer do backend Citro3D (Nintendo 3DS / GPU PICA200).
 *
 * Diferente do OpenGL (VAO + VBOs), o PICA200 lê os vértices de um bloco de
 * memória linear (alocado com linearAlloc) descrito por um C3D_BufInfo. As
 * posições e as normais chegam da engine em dois std::vector<float> separados
 * (3 floats cada), então aqui elas são intercaladas num único buffer com o
 * layout [px,py,pz, nx,ny,nz] por vértice. O C3D_AttrInfo (quais atributos o
 * vertex shader recebe) é responsabilidade do backend/shader; este buffer só
 * guarda os dados e sabe quantos vértices tem.
 *
 * @note O layout de vértice (atributo 0 = posição vec3, atributo 1 = normal
 *       vec3) precisa casar com o C3D_AttrInfo configurado no backend e com os
 *       inputs (v0, v1) do vertex shader .shbin.
 *
 * @see MeshBuffer, MeshBufferFactory, Citro3DRendererBackend
 */
class Citro3DMeshBuffer : public MeshBuffer {
  private:
    void* vboData = nullptr; ///< Buffer linear com os vértices intercalados.
    int vertexCount = 0;     ///< Número de vértices (vertices.size() / 3).
    bool hasNormals = false; ///< @c true se o buffer inclui normais.

    static constexpr int kFloatsPerVertex = 6; ///< pos(3) + normal(3) intercalados.

  public:
    ~Citro3DMeshBuffer() override;

    bool createBuffers(const std::vector<float>& vertices,
                       const std::vector<float>& normals) override;
    void bind() override;
    void unbind() override;
    void destroy() override;
    void* getHandle() const override;

    /** @brief Ponteiro para o buffer linear de vértices (para o C3D_BufInfo). */
    void* getVertexData() const { return vboData; }
    /** @brief Número de vértices do buffer. */
    int getVertexCount() const { return vertexCount; }
    /** @brief Stride, em bytes, de cada vértice no buffer linear. */
    int getStride() const { return kFloatsPerVertex * sizeof(float); }
};

#endif // CITRO3D_MESH_BUFFER_HPP
