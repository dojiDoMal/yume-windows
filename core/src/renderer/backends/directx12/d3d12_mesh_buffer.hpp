#ifndef D3D12_MESH_BUFFER_HPP
#define D3D12_MESH_BUFFER_HPP

#include "../../../mesh_buffer.hpp"
#include <d3d12.h>
#include <vector>

class D3D12RendererBackend;

class D3D12MeshBuffer : public MeshBuffer {
private:
    D3D12RendererBackend* backend;
    ID3D12Resource* vertexBuffer = nullptr;
    ID3D12Resource* normalBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    D3D12_VERTEX_BUFFER_VIEW normalBufferView = {};
    
public:
    D3D12MeshBuffer(D3D12RendererBackend* backend) : backend(backend) {}
    ~D3D12MeshBuffer();
    
    bool createBuffers(const std::vector<float>& vertices, const std::vector<float>& normals) override;
    void bind() override;
    void unbind() override;
    void destroy() override;
    void* getHandle() const override;
    
    D3D12_VERTEX_BUFFER_VIEW* getVertexBufferView() { return &vertexBufferView; }
    D3D12_VERTEX_BUFFER_VIEW* getNormalBufferView() { return &normalBufferView; }
};

#endif
