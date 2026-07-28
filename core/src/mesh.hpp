#ifndef MESH_HPP
#define MESH_HPP

#include "mesh_buffer.hpp"
#include <memory>
#include <vector>

class Mesh {
  private:
    std::vector<float> vertices;
    std::vector<float> normals;
    std::unique_ptr<MeshBuffer> meshBuffer;
    int uniqueVertexCount = 0;
    int triangleCount = 0;

  public:
    Mesh() = default;

    void setVertices(const std::vector<float>& v);
    const std::vector<float>& getVertices() const;
    void setNormals(const std::vector<float>& n);
    const std::vector<float>& getNormals() const;

    bool configure();
    void bind();
    void unbind();

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
