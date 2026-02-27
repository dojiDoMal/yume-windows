#define CLASS_NAME "OpenGLShaderProgram"
#include "open_gl_shader_program.hpp"
#include "log_macros.hpp"
#include "shader_asset.hpp"
#include <cstdint>


OpenGLShaderProgram::~OpenGLShaderProgram() {
    for (auto& pair : uniformBuffers) {
        glDeleteBuffers(1, &pair.second);
    }
    if (programID != 0) {
        glDeleteProgram(programID);
    }
}

bool OpenGLShaderProgram::attachShader(const ShaderAsset& shader) {
    if (programID == 0) {
        programID = glCreateProgram();
    }

    auto value = reinterpret_cast<std::uintptr_t>(shader.getHandle());
    GLuint shaderID = static_cast<GLuint>(value);
    glAttachShader(programID, shaderID);
    return true;
}

bool OpenGLShaderProgram::link() {
    glLinkProgram(programID);

    GLint success;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        GLchar infoLog[512];
        glGetProgramInfoLog(programID, 512, nullptr, infoLog);
        LOG_ERROR("Shader program link error: " + infoLog);
        return false;
    }

    uniformBindings["ModelViewProjection"] = 0;
    uniformBindings["MaterialData"] = 1;
    uniformBindings["LightData"] = 2;

    return true;
}

void OpenGLShaderProgram::use() { glUseProgram(programID); }

void OpenGLShaderProgram::setUniformBuffer(const char* name, const void* data, size_t size) {
    auto it = uniformBindings.find(name);
    if (it == uniformBindings.end()) {
        LOG_WARN("Uniform binding for " + std::string(name) + " not found!");
        return;
    }

    //Spirv-cross prefixes uniforms with 'type_'
    GLuint blockIndex = glGetUniformBlockIndex(programID, ("type_" + std::string(name)).c_str());
    if (blockIndex == GL_INVALID_INDEX) {
        LOG_WARN("Uniform block index for " + std::string(name) + " not found!");
        return;
    }

    int binding = it->second;
    glUniformBlockBinding(programID, blockIndex, binding);

    // Se name não existe no mapa, cria uma entrada com valor 0
    // Se já existe, retorna referência ao GLuint existente
    GLuint& ubo = uniformBuffers[name];
    if (ubo == 0) {
        //Só cria o buffer OpenGL na primeira vez que é usado
        //O ID gerado é armazenado no mapa para reutilização
        glGenBuffers(1, &ubo);
    }
    
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

