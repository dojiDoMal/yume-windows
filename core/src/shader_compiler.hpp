#ifndef SHADER_COMPILER_HPP
#define SHADER_COMPILER_HPP

#include "shader_type.hpp"
#include <string>


class ShaderCompiler {
  public:
    virtual ~ShaderCompiler() = default;
    virtual bool compile(const std::string& source, ShaderType type, void** outHandle) = 0;
    virtual void destroy(void* handle) = 0;
    virtual bool isValid(void* handle) = 0;
};

#endif // SHADERCOMPILER_HPP
