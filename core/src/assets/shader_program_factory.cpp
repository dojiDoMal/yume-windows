/**
 * @file shader_program_factory.cpp
 * @brief Implementação de ShaderProgramFactory::create.
 *
 * Assim como a fábrica de compiladores, os backends disponíveis são decididos
 * por macros de plataforma (OpenGL ES/glad no Switch, WebGL no navegador,
 * OpenGL/Vulkan e DirectX 12 no desktop Windows). Retorna @c nullptr para APIs
 * indisponíveis.
 */
#include "assets/shader_program_factory.hpp"

#ifdef __3DS__
#include "renderer/backends/citro3d/citro3d_shader_program.hpp"
#elif defined(__SWITCH__)
#include "renderer/backends/opengl/open_gl_shader_program.hpp"
#elif PLATFORM_WEBGL
#include "renderer/backends/webgl/web_gl_shader_program.hpp"
#else
#include "renderer/backends/opengl/open_gl_shader_program.hpp"
#include "renderer/backends/vulkan/vulkan_shader_program.hpp"
#ifdef _WIN32
#include "renderer/backends/directx12/d3d12_shader_program.hpp"
#endif // _WIN32
#endif // __SWITCH__

std::unique_ptr<ShaderProgram> ShaderProgramFactory::create(GraphicsAPI api, void* context) {
    switch (api) {
#ifdef __3DS__
    case GraphicsAPI::CITRO3D:
        return std::make_unique<Citro3DShaderProgram>();
#elif defined(__SWITCH__)
    case GraphicsAPI::OPENGL:
        return std::make_unique<OpenGLShaderProgram>();
#elif PLATFORM_WEBGL
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
#endif // _WIN32
#endif // __3DS__
    default:
        return nullptr;
    }
}
