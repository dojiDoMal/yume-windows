#define CLASS_NAME "EGLShaderCompiler"
#include "../../../log_macros.hpp"

#include "egl_shader_compiler.hpp"
#include <cstdint>
#include <fstream>
#include <sstream>

bool EGLShaderCompiler::compile(const std::string& source, ShaderType type, void** outHandle) {

    // Ler o arquivo GLSL
    std::ifstream file(source);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open shader file: " + source);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string shaderSource = buffer.str();

    GLenum glType = toGLShaderType(type);
    GLuint shader = glCreateShader(glType);
    if (!shader) {
        LOG_ERROR("Cannot create shader (type " + std::to_string(static_cast<int>(type)) + ")");
        return false;
    }

    const char* sourcePtr = shaderSource.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        LOG_ERROR("Shader compile failed (" + source + "): " + std::string(infoLog));
        glDeleteShader(shader);
        return false;
    }

    *outHandle = reinterpret_cast<void*>(shader);
    return true;
}

void EGLShaderCompiler::destroy(void* handle) {
    if (handle) {
        auto value = reinterpret_cast<std::uintptr_t>(handle);
        GLuint shader = static_cast<GLuint>(value);
        glDeleteShader(shader);
    }
}

bool EGLShaderCompiler::isValid(void* handle) { return handle != nullptr; }

GLenum EGLShaderCompiler::toGLShaderType(ShaderType type) {
    switch (type) {
    case ShaderType::VERTEX:
        return GL_VERTEX_SHADER;
    case ShaderType::FRAGMENT:
        return GL_FRAGMENT_SHADER;
    default:
        return GL_VERTEX_SHADER;
    }
}
