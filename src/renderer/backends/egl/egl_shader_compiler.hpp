#ifndef EGL_SHADER_COMPILER_HPP
#define EGL_SHADER_COMPILER_HPP

#include "../../../shader_compiler.hpp"
#include <glad/glad.h> // (OpenGL loader)

class EGLShaderCompiler : public ShaderCompiler {
  public:
    bool compile(const std::string& source, ShaderType type, void** outHandle) override;
    void destroy(void* handle) override;
    bool isValid(void* handle) override;

  private:
    GLenum toGLShaderType(ShaderType type);
};

#endif // EGL_SHADER_COMPILER_HPP
