#include "open_gl_mesh_buffer.hpp"

OpenGLMeshBuffer::~OpenGLMeshBuffer() { destroy(); }

void* OpenGLMeshBuffer::getHandle() const { return reinterpret_cast<void*>(VAO); }

bool OpenGLMeshBuffer::createBuffers(const std::vector<float>& vertices,
                                     const std::vector<float>& normals,
                                     const std::vector<float>& texCoords,
                                     const std::vector<float>& tangents) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &positionVBO);
    glGenBuffers(1, &normalVBO);

    glBindVertexArray(VAO);

    // Posição -> location 0 (3 floats).
    glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // Normal -> location 1 (3 floats). Só habilitado quando há normais: o
    // skybox, por exemplo, tem malha só de posição (normals vazio). Habilitar o
    // atributo apontando para um buffer de 0 bytes faz o driver (ex.: AMD) ler
    // vértices fora dos limites e DESCARTAR o draw inteiro silenciosamente — era
    // por isso que o cubo do skybox não desenhava nada.
    if (!normals.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), normals.data(),
                     GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(1);
    }

    // UV -> location 2 (2 floats). Só criado quando a malha tem coordenadas de
    // textura; sem UVs o atributo fica desabilitado e o shader recebe (0,0).
    if (!texCoords.empty()) {
        glGenBuffers(1, &texCoordVBO);
        glBindBuffer(GL_ARRAY_BUFFER, texCoordVBO);
        glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(float), texCoords.data(),
                     GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(2);
    }

    // Tangente -> location 3 (3 floats). Usada para normal mapping; opcional.
    if (!tangents.empty()) {
        glGenBuffers(1, &tangentVBO);
        glBindBuffer(GL_ARRAY_BUFFER, tangentVBO);
        glBufferData(GL_ARRAY_BUFFER, tangents.size() * sizeof(float), tangents.data(),
                     GL_STATIC_DRAW);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(3);
    }

    glBindVertexArray(0);
    return true;
}

void OpenGLMeshBuffer::bind() { glBindVertexArray(VAO); }

void OpenGLMeshBuffer::unbind() { glBindVertexArray(0); }

void OpenGLMeshBuffer::destroy() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &positionVBO);
        glDeleteBuffers(1, &normalVBO);
        if (texCoordVBO != 0)
            glDeleteBuffers(1, &texCoordVBO);
        if (tangentVBO != 0)
            glDeleteBuffers(1, &tangentVBO);
        VAO = positionVBO = normalVBO = texCoordVBO = tangentVBO = 0;
    }
}
