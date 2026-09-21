#define CLASS_NAME "D3D12ShaderCompiler"
#include "log_macros.hpp"

#include "d3d12_shader_compiler.hpp"
#include <fstream>
#include <vector>

struct ShaderBytecode {
    std::vector<char> data;
};

// The DirectX 12 backend consumes precompiled DXIL bytecode (.cso), produced by
// the HLSL -> DXIL build step (dxc). This "compiler" is therefore a bytecode
// loader: it reads the .cso blob from disk and hands the raw bytes back.
bool D3D12ShaderCompiler::compile(const std::string& source, ShaderType type, void** outHandle) {
    std::ifstream file(source, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open compiled shader (.cso): " + source +
                  " - did the DXIL build step run? (dxc -T vs_6_0/ps_6_0)");
        return false;
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize <= 0) {
        LOG_ERROR("Compiled shader is empty: " + source);
        return false;
    }

    auto* bytecode = new ShaderBytecode();
    bytecode->data.resize(static_cast<size_t>(fileSize));

    file.seekg(0);
    if (!file.read(bytecode->data.data(), fileSize)) {
        LOG_ERROR("Failed to read compiled shader: " + source);
        delete bytecode;
        return false;
    }

    *outHandle = bytecode;
    return true;
}

void D3D12ShaderCompiler::destroy(void* handle) {
    if (handle) {
        delete static_cast<ShaderBytecode*>(handle);
    }
}

bool D3D12ShaderCompiler::isValid(void* handle) { return handle != nullptr; }
