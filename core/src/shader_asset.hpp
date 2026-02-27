#ifndef SHADER_ASSET_HPP
#define SHADER_ASSET_HPP

#include "asset.hpp"
#include "shader_compiler.hpp"
#include <memory>


class ShaderAsset : public Asset {
  private:
    ShaderType shaderType;
    void* shaderHandle = nullptr;
    bool isCompiled = false;
    std::unique_ptr<ShaderCompiler> compiler;

  public:
    ShaderAsset(const std::string& path, ShaderType type);
    ~ShaderAsset() override { unload(); }

    bool load() override;
    void unload() override;

    void* getHandle() const { return shaderHandle; }
    ShaderType getType() const { return shaderType; }

    void setShaderCompiler(std::unique_ptr<ShaderCompiler>);
};

#endif // SHADERASSET_HPP
