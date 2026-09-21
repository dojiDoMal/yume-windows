#ifndef MESH_HPP
#define MESH_HPP

#include "mesh_buffer.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class Mesh {
  private:
    std::vector<float> vertices;
    std::vector<float> normals;
    std::unique_ptr<MeshBuffer> meshBuffer;
    int uniqueVertexCount = 0;
    int triangleCount = 0;

    // Local-space bounding sphere, computed once from the vertices.
    // Used by frustum culling so we don't rescan vertices every frame.
    glm::vec3 boundingCenter{0.0f};
    float boundingRadius = 0.0f;
    bool boundsComputed = false;

    void computeBounds();

  public:
    Mesh() = default;

    void setVertices(const std::vector<float>& v);
    const std::vector<float>& getVertices() const;
    void setNormals(const std::vector<float>& n);
    const std::vector<float>& getNormals() const;

    bool configure();
    void bind();
    void unbind();

    // Local-space bounding sphere accessors (valid after configure()/setVertices()).
    const glm::vec3& getBoundingCenter() const { return boundingCenter; }
    float getBoundingRadius() const { return boundingRadius; }
    bool hasBounds() const { return boundsComputed; }

    void* getHandle() const;
    void* getMeshHandle() const;
    void* getMeshBufferHandle() const;

    MeshBuffer* getMeshBuffer() const;
    void setMeshBuffer(std::unique_ptr<MeshBuffer> buffer);

    int getUniqueVertexCount() const { return uniqueVertexCount; }
    void setUniqueVertexCount(int v) { uniqueVertexCount = v; }
    int getTriangleCount() const { return triangleCount; }
    void setTriangleCount(int t) { triangleCount = t; }
};

#endif // MESH_HPP
