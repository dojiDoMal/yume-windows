#ifndef SHADER_COMPILER_FACTORY_HPP
#define SHADER_COMPILER_FACTORY_HPP

#include "graphics_api.hpp"
#include "shader_compiler.hpp"
#include <memory>

class ShaderCompilerFactory {
  public:
    static std::unique_ptr<ShaderCompiler> create(GraphicsAPI api, void* context = nullptr);
};

#endif // SHADERCOMPILERFACTORY_HPP
