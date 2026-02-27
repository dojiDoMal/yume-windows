#include "shader_compiler_factory.hpp"

#ifdef PLATFORM_WEBGL
#include "renderer/backends/webgl/web_gl_shader_compiler.hpp"
#else
#include "renderer/backends/opengl/open_gl_shader_compiler.hpp"
#include "renderer/backends/vulkan/vulkan_shader_compiler.hpp"
#ifdef _WIN32
#include "renderer/backends/directx12/d3d12_shader_compiler.hpp"
#endif
#endif

std::unique_ptr<ShaderCompiler> ShaderCompilerFactory::create(GraphicsAPI api, void* context) {
    switch (api) {
#ifdef PLATFORM_WEBGL
    case GraphicsAPI::WEBGL:
        return std::make_unique<WebGLShaderCompiler>();
#else
    case GraphicsAPI::OPENGL:
        return std::make_unique<OpenGLShaderCompiler>();
    case GraphicsAPI::VULKAN:
        return std::make_unique<VulkanShaderCompiler>(static_cast<VulkanRendererBackend*>(context));
#ifdef _WIN32
    case GraphicsAPI::DIRECTX12:
        return std::make_unique<D3D12ShaderCompiler>(static_cast<D3D12RendererBackend*>(context));
#endif
#endif
    default:
        return nullptr;
    }
}
