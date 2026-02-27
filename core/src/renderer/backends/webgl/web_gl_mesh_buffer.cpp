#include "web_gl_mesh_buffer.hpp"
#include <cstdio>

WebGLMeshBuffer::~WebGLMeshBuffer() { 
    destroy(); 
}

void* WebGLMeshBuffer::getHandle() const { 
    return reinterpret_cast<void*>(VAO); 
}

bool WebGLMeshBuffer::createBuffers(const std::vector<float>& vertices, const std::vector<float>& normals) {
    printf("Creating buffers with %d vertices, %d normals\n", (int)vertices.size(), (int)normals.size());
    
    glGenVertexArrays(1, &VAO);
    printf("Generated VAO: %d\n", VAO);
    
    if (VAO == 0) {
        printf("ERROR: Failed to generate VAO\n");
        return false;
    }
    
    glGenBuffers(1, &positionVBO);
    glGenBuffers(1, &normalVBO);

    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    
    printf("Buffers created successfully\n");
    return true;
}


void WebGLMeshBuffer::bind() {
    glBindVertexArray(VAO);
}

void WebGLMeshBuffer::unbind() {
    glBindVertexArray(0);
}

void WebGLMeshBuffer::destroy() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &positionVBO);
        glDeleteBuffers(1, &normalVBO);
        VAO = positionVBO = normalVBO = 0;
    }
}
