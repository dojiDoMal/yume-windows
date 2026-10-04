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
    CITRO3D,  ///< Citro 3D - Nintendo 3DS 
    OPENGL,   ///< OpenGL (desktop via GLEW) e OpenGL ES no Nintendo Switch (via EGL/glad).
    WEBGL,    ///< WebGL — builds para navegador (Emscripten).
    VULKAN,   ///< Vulkan — desktop de alto desempenho.
    DIRECTX12 ///< DirectX 12 — exclusivo Windows.
};

#endif // GRAPHICS_API_HPP
