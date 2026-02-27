#define CLASS_NAME "D3D12ShaderProgram"
#include "log_macros.hpp"

#include "d3d12_shader_program.hpp"
#include "d3d12_renderer_backend.hpp"
#include "log_macros.hpp"
#include "shader_asset.hpp"
#include <vector>

struct ShaderBytecode {
    std::vector<char> data;
};

D3D12ShaderProgram::~D3D12ShaderProgram() {
    for (auto& pair : constantBuffers) {
        if (pair.second) pair.second->Release();
    }
    if (pipelineState) pipelineState->Release();
    if (rootSignature) rootSignature->Release();
}

bool D3D12ShaderProgram::attachShader(const ShaderAsset& shader) {
    shaderBytecodes.push_back(shader.getHandle());
    shaderTypes.push_back(shader.getType());
    return true;
}

bool D3D12ShaderProgram::link() {
    return createPipeline();
}

bool D3D12ShaderProgram::createPipeline() {
    auto device = backend->getDevice();
    
    D3D12_ROOT_PARAMETER rootParams[3] = {};
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[1].Descriptor.ShaderRegister = 1;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[2].Descriptor.ShaderRegister = 2;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    
    uniformBindings["ModelViewProjection"] = 0;
    uniformBindings["MaterialData"] = 1;
    uniformBindings["LightData"] = 2;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 3;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    
    ID3DBlob* signature;
    ID3DBlob* error;
    if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
        LOG_ERROR("Failed to serialize root signature");
        return false;
    }
    if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)))) {
        LOG_ERROR("Failed to create root signature");
        signature->Release();
        return false;
    }
    signature->Release();
    
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSignature;
    
    for (size_t i = 0; i < shaderBytecodes.size(); i++) {
        auto* bytecode = static_cast<ShaderBytecode*>(shaderBytecodes[i]);
        if (shaderTypes[i] == ShaderType::VERTEX) {
            psoDesc.VS = {bytecode->data.data(), bytecode->data.size()};
        } else if (shaderTypes[i] == ShaderType::FRAGMENT) {
            psoDesc.PS = {bytecode->data.data(), bytecode->data.size()};
        }
    }
    
    psoDesc.InputLayout = {inputLayout, 2};
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    
    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create pipeline state");
        return false;
    }
    return true;
}


void D3D12ShaderProgram::use() {
}

void D3D12ShaderProgram::setUniformBuffer(const char* name, const void* data, size_t size) {
    auto it = uniformBindings.find(name);
    if (it == uniformBindings.end()) {
        LOG_WARN("Uniform " + std::string(name) + " not found!");
        return;
    }

    ID3D12Resource*& buffer = constantBuffers[name];
    if (buffer == nullptr) {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        
        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = (size + 255) & ~255;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        
        backend->getDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, 
            &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));
    }
    
    void* mappedData;
    buffer->Map(0, nullptr, &mappedData);
    memcpy(mappedData, data, size);
    buffer->Unmap(0, nullptr);
}

void* D3D12ShaderProgram::getHandle() const {
    return pipelineState;
}

bool D3D12ShaderProgram::isValid() const {
    return pipelineState != nullptr;
}
