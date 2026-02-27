#include "d3d12_shader_compiler.hpp"
#include <fstream>
#include <vector>

struct ShaderBytecode {
    std::vector<char> data;
};

bool D3D12ShaderCompiler::compile(const std::string& source, ShaderType type, void** outHandle) {
    std::ifstream file(source, std::ios::ate | std::ios::binary);
    if (!file.is_open()) return false;
    
    size_t fileSize = (size_t)file.tellg();
    auto* bytecode = new ShaderBytecode();
    bytecode->data.resize(fileSize);
    
    file.seekg(0);
    file.read(bytecode->data.data(), fileSize);
    file.close();
    
    *outHandle = bytecode;
    return true;
}

void D3D12ShaderCompiler::destroy(void* handle) {
    if (handle) {
        delete static_cast<ShaderBytecode*>(handle);
    }
}

bool D3D12ShaderCompiler::isValid(void* handle) {
    return handle != nullptr;
}
