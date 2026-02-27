#include "open_gl_mesh_buffer.hpp"

OpenGLMeshBuffer::~OpenGLMeshBuffer() { 
    destroy(); 
}

void* OpenGLMeshBuffer::getHandle() const { 
    return reinterpret_cast<void*>(VAO); 
}

bool OpenGLMeshBuffer::createBuffers(const std::vector<float>& vertices, const std::vector<float>& normals) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &positionVBO);
    glGenBuffers(1, &normalVBO);

    glBindVertexArray(VAO);
    
    // Position buffer
    glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal buffer
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return true;
}

void OpenGLMeshBuffer::bind() {
    glBindVertexArray(VAO);
}

void OpenGLMeshBuffer::unbind() {
    glBindVertexArray(0);
}

void OpenGLMeshBuffer::destroy() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &positionVBO);
        glDeleteBuffers(1, &normalVBO);
        VAO = positionVBO = normalVBO = 0;
    }
}
