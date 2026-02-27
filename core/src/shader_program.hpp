#ifndef SHADER_PROGRAM_HPP
#define SHADER_PROGRAM_HPP

#include <cstddef>

class ShaderAsset;

class ShaderProgram {
  public:
    virtual ~ShaderProgram() = default;
    virtual bool attachShader(const ShaderAsset& shader) = 0;
    virtual bool link() = 0;
    virtual void use() = 0;
    virtual void setUniformBuffer(const char* name, const void* data, size_t size) = 0;
    virtual void* getHandle() const = 0;
    virtual bool isValid() const = 0;
};

#endif // SHADERPROGRAM_HPP