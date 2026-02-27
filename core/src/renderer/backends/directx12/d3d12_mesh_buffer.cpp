#define CLASS_NAME "D3D12MeshBuffer"
#include "d3d12_mesh_buffer.hpp"
#include "d3d12_renderer_backend.hpp"
#include "log_macros.hpp"

D3D12MeshBuffer::~D3D12MeshBuffer() {
    destroy();
}

bool D3D12MeshBuffer::createBuffers(const std::vector<float>& vertices, const std::vector<float>& normals) {
    auto device = backend->getDevice();
    
    UINT vertexBufferSize = vertices.size() * sizeof(float);
    UINT normalBufferSize = normals.size() * sizeof(float);
    
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = vertexBufferSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    
    if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer)))) {
        return false;
    }
    
    void* pData;
    vertexBuffer->Map(0, nullptr, &pData);
    memcpy(pData, vertices.data(), vertexBufferSize);
    vertexBuffer->Unmap(0, nullptr);
    
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = vertexBufferSize;
    vertexBufferView.StrideInBytes = 3 * sizeof(float);
    
    if (!normals.empty()) {
        bufferDesc.Width = normalBufferSize;
        if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&normalBuffer)))) {
            return false;
        }
        
        normalBuffer->Map(0, nullptr, &pData);
        memcpy(pData, normals.data(), normalBufferSize);
        normalBuffer->Unmap(0, nullptr);
        
        normalBufferView.BufferLocation = normalBuffer->GetGPUVirtualAddress();
        normalBufferView.SizeInBytes = normalBufferSize;
        normalBufferView.StrideInBytes = 3 * sizeof(float);
    }
    
    return true;
}

void D3D12MeshBuffer::bind() {
}

void D3D12MeshBuffer::unbind() {
}

void D3D12MeshBuffer::destroy() {
    if (vertexBuffer) {
        vertexBuffer->Release();
        vertexBuffer = nullptr;
    }
    if (normalBuffer) {
        normalBuffer->Release();
        normalBuffer = nullptr;
    }
}

void* D3D12MeshBuffer::getHandle() const {
    return vertexBuffer;
}
