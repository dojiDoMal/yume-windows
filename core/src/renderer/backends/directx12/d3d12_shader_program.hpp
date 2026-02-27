#ifndef D3D12_SHADER_PROGRAM_HPP
#define D3D12_SHADER_PROGRAM_HPP

#include "../../../shader_program.hpp"
#include "../../../shader_type.hpp"
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <vector>

class D3D12RendererBackend;

class D3D12ShaderProgram : public ShaderProgram {
  private:
    D3D12RendererBackend* backend;
    std::vector<void*> shaderBytecodes;
    std::vector<ShaderType> shaderTypes;
    ID3D12PipelineState* pipelineState = nullptr;
    ID3D12RootSignature* rootSignature = nullptr;

    std::unordered_map<std::string, int> uniformBindings;
    std::unordered_map<std::string, ID3D12Resource*> constantBuffers;

    bool createPipeline();

  public:
    D3D12ShaderProgram(D3D12RendererBackend* backend) : backend(backend) {}
    ~D3D12ShaderProgram();

    bool attachShader(const ShaderAsset& shader) override;
    bool link() override;
    void use() override;
    void setUniformBuffer(const char* name, const void* data, size_t size) override;
    void* getHandle() const override;
    bool isValid() const override;

    D3D12_GPU_VIRTUAL_ADDRESS getConstantBufferAddress(const char* name) const {
        auto it = constantBuffers.find(name);
        return (it != constantBuffers.end() && it->second) ? it->second->GetGPUVirtualAddress() : 0;
    }

    ID3D12PipelineState* getPipelineState() const { return pipelineState; }
    ID3D12RootSignature* getRootSignature() const { return rootSignature; }
};

#endif
