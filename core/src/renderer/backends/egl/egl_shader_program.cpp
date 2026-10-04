#define CLASS_NAME "EGLShaderProgram"
#include "egl_shader_program.hpp"
#include "log_macros.hpp"
#include "shader_asset.hpp"
#include <cstdint>

EGLShaderProgram::~EGLShaderProgram() {
    for (auto& pair : uniformBuffers) {
        glDeleteBuffers(1, &pair.second);
    }
    if (programID != 0) {
        glDeleteProgram(programID);
    }
}

bool EGLShaderProgram::attachShader(const ShaderAsset& shader) {
    if (programID == 0) {
        programID = glCreateProgram();
    }

    auto value = reinterpret_cast<std::uintptr_t>(shader.getHandle());
    GLuint shaderID = static_cast<GLuint>(value);
    glAttachShader(programID, shaderID);
    return true;
}

bool EGLShaderProgram::link() {
    glLinkProgram(programID);

    GLint success;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        char infoLog[512];
        glGetProgramInfoLog(programID, sizeof(infoLog), nullptr, infoLog);
        LOG_INFO(std::string("Link error: ") + infoLog);
        return false;
    }

    uniformBindings["ModelViewProjection"] = 0;
    uniformBindings["MaterialData"] = 1;
    uniformBindings["LightData"] = 2;

    return true;
}

void EGLShaderProgram::use() { glUseProgram(programID); }

void EGLShaderProgram::setUniformBuffer(const char* name, const void* data, size_t size) {
    auto it = uniformBindings.find(name);
    if (it == uniformBindings.end()) {
        LOG_WARN(std::string("Uniform binding for ") + name + " not found!");
        return;
    }

    // Spirv-cross prefixes uniforms with 'type_'
    GLuint blockIndex = glGetUniformBlockIndex(programID, ("type_" + std::string(name)).c_str());
    if (blockIndex == GL_INVALID_INDEX) {
        LOG_WARN(std::string("Uniform block index for ") + name + " not found!");
        return;
    }

    int binding = it->second;
    glUniformBlockBinding(programID, blockIndex, binding);

    // Se name não existe no mapa, cria uma entrada com valor 0
    // Se já existe, retorna referência ao GLuint existente
    GLuint& ubo = uniformBuffers[name];
    if (ubo == 0) {
        // Só cria o buffer OpenGL na primeira vez que é usado
        // O ID gerado é armazenado no mapa para reutilização
        glGenBuffers(1, &ubo);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
