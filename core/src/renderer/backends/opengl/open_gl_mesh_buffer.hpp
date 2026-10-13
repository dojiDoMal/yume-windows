#ifndef OPEN_GL_MESH_BUFFER_HPP
#define OPEN_GL_MESH_BUFFER_HPP

#include "assets/mesh_buffer.hpp"
#ifdef __SWITCH__
#include <glad/glad.h>
#else
#include <GL/glew.h>
#endif

class OpenGLMeshBuffer : public MeshBuffer {
  private:
    GLuint VAO = 0;
    GLuint positionVBO = 0;
    GLuint normalVBO = 0;
    GLuint texCoordVBO = 0;
    GLuint tangentVBO = 0;

  public:
    ~OpenGLMeshBuffer() override;

    bool createBuffers(const std::vector<float>& vertices, const std::vector<float>& normals,
                       const std::vector<float>& texCoords = {},
                       const std::vector<float>& tangents = {}) override;
    void bind() override;
    void unbind() override;
    void destroy() override;
    void* getHandle() const override;
};

#endif // OPENGLMESHBUFFER_HPP
