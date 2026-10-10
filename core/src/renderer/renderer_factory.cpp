#include "renderer/renderer_factory.hpp"

#ifdef __3DS__
#include "backends/citro3d/citro3d_renderer_backend.hpp"
#elif defined(__SWITCH__)
#include "backends/opengl/open_gl_renderer_backend.hpp"
#elif PLATFORM_WEBGL
#include "backends/webgl/web_gl_renderer_backend.hpp"
#else
#include "backends/opengl/open_gl_renderer_backend.hpp"
#include "backends/vulkan/vulkan_renderer_backend.hpp"
#ifdef _WIN32
#include "backends/directx12/d3d12_renderer_backend.hpp"
#endif
#endif

RendererBackend* RendererFactory::create(const GraphicsAPI& api) {
    switch (api) {

    case GraphicsAPI::CITRO3D:
#ifdef __3DS__
        return new Citro3DRendererBackend();
#else
        return nullptr;
#endif

    case GraphicsAPI::WEBGL:
#ifdef PLATFORM_WEBGL
        return new WebGLRendererBackend();
#else
        return nullptr;
#endif

    // OpenGL covers both the desktop (GLEW) and the Switch (OpenGL ES via
    // EGL/glad); both share the single OpenGLRendererBackend implementation.
    case GraphicsAPI::OPENGL:
#if !defined(PLATFORM_WEBGL) && !defined(__3DS__)
        return new OpenGLRendererBackend();
#else
        return nullptr;
#endif

    case GraphicsAPI::VULKAN:
#if !defined(PLATFORM_WEBGL) && !defined(__SWITCH__) && !defined(__3DS__)
        return new VulkanRendererBackend();
#else
        return nullptr;
#endif

    case GraphicsAPI::DIRECTX12:
#if defined(_WIN32) && !defined(PLATFORM_WEBGL) && !defined(__3DS__)
        return new D3D12RendererBackend();
#else
        return nullptr;
#endif

    default:
        return nullptr;
    }
}
