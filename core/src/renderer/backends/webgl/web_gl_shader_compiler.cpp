#include "web_gl_shader_compiler.hpp"
#include <cstdint>
#include <cstdio>

bool WebGLShaderCompiler::compile(const std::string& source, ShaderType type, void** outHandle) {
    printf("WebGLShaderCompiler::compile\n");
    GLenum glType = toGLShaderType(type);
    GLuint shader = glCreateShader(glType);
    
    printf("Compiling shader type %d, source length: %zu\n", type, source.length());
    printf("Source:\n%s\n", source.c_str());
    
    const char* sourcePtr = source.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        printf("Shader compilation error: %s\n", infoLog);
        glDeleteShader(shader);
        return false;
    }
    
    printf("Shader compiled successfully, ID: %d\n", shader);
    *outHandle = reinterpret_cast<void*>(static_cast<uintptr_t>(shader));
    return true;
}



void WebGLShaderCompiler::destroy(void* handle) {
    if (handle) {
        auto value = reinterpret_cast<std::uintptr_t>(handle);
        GLuint shader = static_cast<GLuint>(value);
        glDeleteShader(shader);
    }
}

bool WebGLShaderCompiler::isValid(void* handle) {
    return handle != nullptr;
}

GLenum WebGLShaderCompiler::toGLShaderType(ShaderType type) {
    switch(type) {
        case ShaderType::VERTEX: return GL_VERTEX_SHADER;
        case ShaderType::FRAGMENT: return GL_FRAGMENT_SHADER;
        default: return GL_VERTEX_SHADER;
    }
}
