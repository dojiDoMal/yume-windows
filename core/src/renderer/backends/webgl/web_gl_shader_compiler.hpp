#ifndef WEB_GL_SHADER_COMPILER_HPP
#define WEB_GL_SHADER_COMPILER_HPP

#include "shader_compiler.hpp"
#include <GLES3/gl3.h>

class WebGLShaderCompiler : public ShaderCompiler {
public:
    bool compile(const std::string& source, ShaderType type, void** outHandle) override;
    void destroy(void* handle) override;
    bool isValid(void* handle) override;

private:
    GLenum toGLShaderType(ShaderType type);
};

#endif 
