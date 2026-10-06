#ifndef SDL2_GL_DISPLAY_BACKEND_HPP
#define SDL2_GL_DISPLAY_BACKEND_HPP

#include "window/mml/sdl2/sdl2_display_backend.hpp"

/**
 * @brief Backend de janela SDL2 para OpenGL / OpenGL ES.
 *
 * Acrescenta à base os serviços de contexto GL que dependem da janela:
 * criação/destruição do contexto, swap interval, loader de ponteiros de função
 * (para glad/GLEW) e troca de buffers. É o único ponto que chama SDL_GL_*; o
 * OpenGLRendererBackend consome tudo por void*.
 *
 * @see SDL2DisplayBackend, DisplayBackend, OpenGLRendererBackend
 */
class SDL2GLDisplayBackend : public SDL2DisplayBackend {
  public:
    unsigned int getApiWindowFlags() const override;
    void* createGLContext(void* window) override;
    void destroyGLContext(void* context) override;
    void setSwapInterval(bool vsync) override;
    void* getGLProcAddressLoader() override;
    void swapBuffers(void* window) override;
};

#endif // SDL2_GL_DISPLAY_BACKEND_HPP
