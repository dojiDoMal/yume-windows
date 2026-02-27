#ifndef OPEN_GL_MESH_BUFFER_HPP
#define OPEN_GL_MESH_BUFFER_HPP

#include "mesh_buffer.hpp"
#include <GL/glew.h>

class OpenGLMeshBuffer : public MeshBuffer {
private:
    GLuint VAO = 0;
    GLuint positionVBO = 0;
    GLuint normalVBO = 0;

public:
    ~OpenGLMeshBuffer() override;
    
    bool createBuffers(const std::vector<float>& vertices, const std::vector<float>& normals) override;
    void bind() override;
    void unbind() override;
    void destroy() override;
    void* getHandle() const override;
};

#endif // OPENGLMESHBUFFER_HPP
