#ifndef WEB_GL_MESH_BUFFER_HPP
#define WEB_GL_MESH_BUFFER_HPP

#include "mesh_buffer.hpp"
#include <GLES3/gl3.h>

class WebGLMeshBuffer : public MeshBuffer {
private:
    GLuint VAO = 0;
    GLuint positionVBO = 0;
    GLuint normalVBO = 0;

public:
    ~WebGLMeshBuffer() override;
    
    bool createBuffers(const std::vector<float>& vertices, const std::vector<float>& normals) override;
    void bind() override;
    void unbind() override;
    void destroy() override;
    void* getHandle() const override;
};

#endif 
