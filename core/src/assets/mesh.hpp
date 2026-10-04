#ifndef MESH_HPP
#define MESH_HPP

#include "assets/mesh_buffer.hpp"
#include "math/vector3.hpp"
#include <memory>
#include <vector>

/**
 * @brief Dados de geometria de uma malha e seu buffer de GPU associado.
 *
 * Guarda os vértices e normais em memória e, após configure(), um MeshBuffer
 * com os dados enviados à GPU. Também mantém contagens (vértices/triângulos) e
 * uma esfera envolvente (bounding sphere) em espaço local, calculada uma vez a
 * partir dos vértices e usada pelo frustum culling para evitar reescanear a
 * geometria a cada frame.
 *
 * @see MeshBuffer, MeshBufferFactory
 */
class Mesh {
  private:
    std::vector<float> vertices;              ///< Posições dos vértices.
    std::vector<float> normals;               ///< Normais dos vértices.
    std::unique_ptr<MeshBuffer> meshBuffer;   ///< Buffer correspondente na GPU.
    int uniqueVertexCount = 0;                ///< Número de vértices únicos.
    int triangleCount = 0;                    ///< Número de triângulos.

    Vector3 boundingCenter{0.0f, 0.0f, 0.0f}; ///< Centro da esfera envolvente (espaço local).
    float boundingRadius = 0.0f;              ///< Raio da esfera envolvente.
    bool boundsComputed = false;              ///< @c true após o cálculo da esfera.

    /** @brief Calcula a esfera envolvente a partir dos vértices. */
    void computeBounds();

  public:
    Mesh() = default;

    /** @brief Define os vértices da malha. */
    void setVertices(const std::vector<float>& v);
    /** @brief Retorna os vértices da malha. */
    const std::vector<float>& getVertices() const;
    /** @brief Define as normais da malha. */
    void setNormals(const std::vector<float>& n);
    /** @brief Retorna as normais da malha. */
    const std::vector<float>& getNormals() const;

    /**
     * @brief Envia a geometria para a GPU e calcula a esfera envolvente.
     * @return @c true em caso de sucesso.
     */
    bool configure();
    /** @brief Ativa o buffer da malha para desenho. */
    void bind();
    /** @brief Desativa o buffer da malha. */
    void unbind();

    /** @brief Centro da esfera envolvente (válido após configure()/setVertices()). */
    const Vector3& getBoundingCenter() const { return boundingCenter; }
    /** @brief Raio da esfera envolvente. */
    float getBoundingRadius() const { return boundingRadius; }
    /** @brief Indica se a esfera envolvente já foi calculada. */
    bool hasBounds() const { return boundsComputed; }

    /** @brief Handle nativo da malha (atalho para o do buffer). */
    void* getHandle() const;
    /** @brief Handle nativo da malha. */
    void* getMeshHandle() const;
    /** @brief Handle nativo do buffer de GPU. */
    void* getMeshBufferHandle() const;

    /** @brief Retorna o MeshBuffer associado. */
    MeshBuffer* getMeshBuffer() const;
    /** @brief Define o MeshBuffer associado (assume a posse). */
    void setMeshBuffer(std::unique_ptr<MeshBuffer> buffer);

    /** @brief Número de vértices únicos. */
    int getUniqueVertexCount() const { return uniqueVertexCount; }
    /** @brief Define o número de vértices únicos. */
    void setUniqueVertexCount(int v) { uniqueVertexCount = v; }
    /** @brief Número de triângulos. */
    int getTriangleCount() const { return triangleCount; }
    /** @brief Define o número de triângulos. */
    void setTriangleCount(int t) { triangleCount = t; }
};

#endif // MESH_HPP
