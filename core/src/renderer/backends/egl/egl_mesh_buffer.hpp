#ifndef EGL_MESH_BUFFER_HPP
#define EGL_MESH_BUFFER_HPP

#include "mesh_buffer.hpp"
#include <glad/glad.h>

class EGLMeshBuffer : public MeshBuffer {
  private:
    GLuint VAO = 0;
    GLuint positionVBO = 0;
    GLuint normalVBO = 0;

  public:
    ~EGLMeshBuffer() override;

    bool createBuffers(const std::vector<float>& vertices,
                       const std::vector<float>& normals) override;
    void bind() override;
    void unbind() override;
    void destroy() override;
    void* getHandle() const override;
};

#endif // EGL_MESH_BUFFER_HPP
