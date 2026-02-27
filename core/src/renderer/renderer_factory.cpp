#include "renderer_factory.hpp"

#ifdef PLATFORM_WEBGL
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

        case GraphicsAPI::WEBGL:
        #ifdef PLATFORM_WEBGL
            return new WebGLRendererBackend();
        #else
            return nullptr;
        #endif

        case GraphicsAPI::OPENGL:
        #ifndef PLATFORM_WEBGL
            return new OpenGLRendererBackend();
        #else
            return nullptr;
        #endif

        case GraphicsAPI::VULKAN:
        #ifndef PLATFORM_WEBGL
            return new VulkanRendererBackend();
        #else
            return nullptr;
        #endif

        case GraphicsAPI::DIRECTX12:
        #if defined(_WIN32) && !defined(PLATFORM_WEBGL)
            return new D3D12RendererBackend();
        #else
            return nullptr;
        #endif

        default:
            return nullptr;
    }
}
