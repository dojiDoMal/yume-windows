#ifndef N3DS_DISPLAY_BACKEND_HPP
#define N3DS_DISPLAY_BACKEND_HPP

#include "graphics_api.hpp"
#include "window/mml/display_backend.hpp"

/**
 * @brief Backend de janela para o Nintendo 3DS (Citro3D).
 *
 * Stub compilado apenas sob __3DS__. Mantido para espelhar a convenção de
 * backend por plataforma; a implementação concreta chega junto com o suporte
 * efetivo ao 3DS.
 *
 * @see DisplayBackend, N3DSMultimediaLayer
 */
class N3DSDisplayBackend : public DisplayBackend {
  public:
    ~N3DSDisplayBackend() = default;

    void configureContext(const GraphicsAPI& graphicsAPI) override;
    void* createMainDisplay(unsigned int flags, const WindowDesc& opts) override;
    void destroyDisplay(void* display) override;
};

#endif // N3DS_DISPLAY_BACKEND_HPP
