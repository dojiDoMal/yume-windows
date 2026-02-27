#ifndef D3D12_SHADER_COMPILER_HPP
#define D3D12_SHADER_COMPILER_HPP

#include "shader_compiler.hpp"
#include <d3d12.h>
#include <vector>

class D3D12RendererBackend;

class D3D12ShaderCompiler : public ShaderCompiler {
private:
    D3D12RendererBackend* backend;
    
public:
    D3D12ShaderCompiler(D3D12RendererBackend* backend) : backend(backend) {}
    ~D3D12ShaderCompiler() = default;
    
    bool compile(const std::string& source, ShaderType type, void** outHandle) override;
    void destroy(void* handle) override;
    bool isValid(void* handle) override;
};

#endif
