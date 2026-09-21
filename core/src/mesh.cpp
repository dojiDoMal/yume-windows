#include "mesh.hpp"
#include <GL/glew.h>
#include <cmath>

bool Mesh::configure() {
    if (!boundsComputed)
        computeBounds();

    bool result = meshBuffer->createBuffers(vertices, normals);

    return result;
}

void Mesh::computeBounds() {
    boundingCenter = glm::vec3(0.0f);
    boundingRadius = 0.0f;
    boundsComputed = true;

    if (vertices.size() < 3)
        return;

    // Pass 1: axis-aligned bounding box, use its center as the sphere center.
    glm::vec3 minP(vertices[0], vertices[1], vertices[2]);
    glm::vec3 maxP = minP;
    for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
        glm::vec3 p(vertices[i], vertices[i + 1], vertices[i + 2]);
        minP = glm::min(minP, p);
        maxP = glm::max(maxP, p);
    }
    boundingCenter = (minP + maxP) * 0.5f;

    // Pass 2: radius = max distance from center to any vertex.
    float maxDistSq = 0.0f;
    for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
        glm::vec3 p(vertices[i], vertices[i + 1], vertices[i + 2]);
        float d2 = glm::dot(p - boundingCenter, p - boundingCenter);
        if (d2 > maxDistSq)
            maxDistSq = d2;
    }
    boundingRadius = std::sqrt(maxDistSq);
}

void Mesh::setVertices(const std::vector<float>& v) {
    vertices = v;
    boundsComputed = false;
    computeBounds();
}

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
