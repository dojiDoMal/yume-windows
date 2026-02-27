#include "mesh_buffer_factory.hpp"

#ifdef PLATFORM_WEBGL
#include "renderer/backends/webgl/web_gl_mesh_buffer.hpp"
#else
#include "renderer/backends/opengl/open_gl_mesh_buffer.hpp"
#include "renderer/backends/vulkan/vulkan_mesh_buffer.hpp"
#ifdef _WIN32
#include "renderer/backends/directx12/d3d12_mesh_buffer.hpp"
#endif
#endif

std::unique_ptr<MeshBuffer> MeshBufferFactory::create(GraphicsAPI api, void* context) {
    switch (api) {
#ifdef PLATFORM_WEBGL
    case GraphicsAPI::WEBGL:
        return std::make_unique<WebGLMeshBuffer>();
#else
    case GraphicsAPI::OPENGL:
        return std::make_unique<OpenGLMeshBuffer>();
    case GraphicsAPI::VULKAN:
        return std::make_unique<VulkanMeshBuffer>(static_cast<VulkanRendererBackend*>(context));
#ifdef _WIN32
    case GraphicsAPI::DIRECTX12:
        return std::make_unique<D3D12MeshBuffer>(static_cast<D3D12RendererBackend*>(context));
#endif
#endif
    default:
        return nullptr;
    }
}
