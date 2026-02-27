#define CLASS_NAME "ShaderAsset"
#include "shader_asset.hpp"
#include "log_macros.hpp"

ShaderAsset::ShaderAsset(const std::string& path, ShaderType type)
    : Asset(path), shaderType(type) {}

void ShaderAsset::setShaderCompiler(std::unique_ptr<ShaderCompiler> comp) {
    compiler = std::move(comp);
}

bool ShaderAsset::load() {
    if (compiler && compiler->compile(getPath(), shaderType, &shaderHandle)) {
        loaded = true;
        return true;
    }

    LOG_ERROR("Shader compilation failed for: " + getPath());
    return false;
}

void ShaderAsset::unload() {
    if (compiler && shaderHandle) {
        compiler->destroy(shaderHandle);
        shaderHandle = nullptr;
    }
    loaded = false;
}
