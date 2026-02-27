#ifndef OPEN_GL_SHADER_COMPILER_HPP
#define OPEN_GL_SHADER_COMPILER_HPP

#include "../../../shader_compiler.hpp"
#include <GL/glew.h>

class OpenGLShaderCompiler : public ShaderCompiler {
public:
    bool compile(const std::string& source, ShaderType type, void** outHandle) override;
    void destroy(void* handle) override;
    bool isValid(void* handle) override;

private:
    GLenum toGLShaderType(ShaderType type);
};

#endif // OPENGLSHADERCOMPILER_HPP
