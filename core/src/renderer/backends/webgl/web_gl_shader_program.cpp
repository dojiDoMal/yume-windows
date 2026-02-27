#include "web_gl_shader_program.hpp"
#include "shader_asset.hpp"

WebGLShaderProgram::~WebGLShaderProgram() {
    if (programID != 0) {
        glDeleteProgram(programID);
    }
}

bool WebGLShaderProgram::attachShader(const ShaderAsset& shader) {
    if (programID == 0) {
        programID = glCreateProgram();
    }

    auto value = reinterpret_cast<std::uintptr_t>(shader.getHandle());
    GLuint shaderID = static_cast<GLuint>(value);
    glAttachShader(programID, shaderID);
    return true;
}

bool WebGLShaderProgram::link() {    
    glLinkProgram(programID);
    
    GLint success;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(programID, 512, nullptr, infoLog);
        printf("Shader linking failed: %s\n", infoLog);
    } else {
        printf("Shader linked successfully! Program ID: %d\n", programID);
    }
    return success == GL_TRUE;
}


void WebGLShaderProgram::use() {
    glUseProgram(programID);
}
