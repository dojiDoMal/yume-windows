#include "mesh.hpp"
#include <GL/glew.h>

bool Mesh::configure() {
    bool result = meshBuffer->createBuffers(vertices, normals);

    return result;
}

void Mesh::setVertices(const std::vector<float>& v) { vertices = v; }

const std::vector<float>& Mesh::getVertices() const { return vertices; }

void Mesh::setNormals(const std::vector<float>& n) { normals = n; }

const std::vector<float>& Mesh::getNormals() const { return normals; }

void Mesh::bind() {
    if (meshBuffer)
        meshBuffer->bind();
}

void Mesh::unbind() {
    if (meshBuffer)
        meshBuffer->unbind();
}

void* Mesh::getHandle() const { return meshBuffer ? meshBuffer->getHandle() : nullptr; }

void* Mesh::getMeshHandle() const { return meshBuffer ? meshBuffer->getHandle() : nullptr; }

void* Mesh::getMeshBufferHandle() const { return meshBuffer ? meshBuffer->getHandle() : nullptr; }

MeshBuffer* Mesh::getMeshBuffer() const { return meshBuffer.get(); }

void Mesh::setMeshBuffer(std::unique_ptr<MeshBuffer> buffer) { meshBuffer = std::move(buffer); }
