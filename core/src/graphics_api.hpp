#ifndef GRAPHICS_API_HPP
#define GRAPHICS_API_HPP

/**
 * @brief Backends de renderização suportados pelo engine.
 *
 * O backend ativo é escolhido em tempo de execução (ou de compilação, via
 * macros de plataforma) e determina qual implementação concreta das fábricas
 * de shader, mesh buffer e renderer será instanciada.
 *
 * @see ShaderCompilerFactory, MeshBufferFactory
 */
enum class GraphicsAPI {
    OPENGL,   ///< OpenGL — desktop (Windows, Linux, macOS).
    WEBGL,    ///< WebGL — builds para navegador (Emscripten).
    VULKAN,   ///< Vulkan — desktop de alto desempenho.
    DIRECTX12, ///< DirectX 12 — exclusivo Windows.
    EGL       ///< EGL/OpenGL ES — compatível com o Nintendo Switch.
};

#endif // GRAPHICS_API_HPP
