/**
 * @file shader_compiler_factory.cpp
 * @brief Implementação de ShaderCompilerFactory::create.
 *
 * O conjunto de backends disponíveis é definido em tempo de compilação por
 * macros de plataforma: o Switch usa EGL, builds WebGL usam WebGL e o desktop
 * usa OpenGL/Vulkan (mais DirectX 12 no Windows). O @c switch devolve a
 * implementação correspondente ou @c nullptr para APIs indisponíveis.
 */
#include "shader_compiler_factory.hpp"

#ifdef __SWITCH__
#include "renderer/backends/egl/egl_shader_compiler.hpp"
#elif PLATFORM_WEBGL
#include "renderer/backends/webgl/web_gl_shader_compiler.hpp"
#else
#include "renderer/backends/opengl/open_gl_shader_compiler.hpp"
#include "renderer/backends/vulkan/vulkan_shader_compiler.hpp"
#ifdef _WIN32
#include "renderer/backends/directx12/d3d12_shader_compiler.hpp"
#endif // _WIN32
#endif // __SWITCH__

std::unique_ptr<ShaderCompiler> ShaderCompilerFactory::create(GraphicsAPI api, void* context) {
    switch (api) {
#ifdef __SWITCH__
    case GraphicsAPI::EGL:
        return std::make_unique<EGLShaderCompiler>();
#elif PLATFORM_WEBGL
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
#endif // _WIN32
#endif // __SWITCH__
    default:
        return nullptr;
    }
}
