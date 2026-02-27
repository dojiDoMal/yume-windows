#include "shader_program_factory.hpp"

#ifdef PLATFORM_WEBGL
#include "renderer/backends/webgl/web_gl_shader_program.hpp"
#else
#include "renderer/backends/opengl/open_gl_shader_program.hpp"
#include "renderer/backends/vulkan/vulkan_shader_program.hpp"
#ifdef _WIN32
#include "renderer/backends/directx12/d3d12_shader_program.hpp"
#endif
#endif

std::unique_ptr<ShaderProgram> ShaderProgramFactory::create(GraphicsAPI api, void* context) {
    switch (api) {
#ifdef PLATFORM_WEBGL
    case GraphicsAPI::WEBGL:
        return std::make_unique<WebGLShaderProgram>();
#else
    case GraphicsAPI::OPENGL:
        return std::make_unique<OpenGLShaderProgram>();
    case GraphicsAPI::VULKAN:
        return std::make_unique<VulkanShaderProgram>(static_cast<VulkanRendererBackend*>(context));
#ifdef _WIN32
    case GraphicsAPI::DIRECTX12:
        return std::make_unique<D3D12ShaderProgram>(static_cast<D3D12RendererBackend*>(context));
#endif
#endif
    default:
        return nullptr;
    }
}
