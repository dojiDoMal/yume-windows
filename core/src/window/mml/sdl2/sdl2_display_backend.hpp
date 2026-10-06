#ifndef SDL2_DISPLAY_BACKEND_HPP
#define SDL2_DISPLAY_BACKEND_HPP

#include "window/mml/display_backend.hpp"

/**
 * @brief Base SDL2 do backend de janela (parte neutra, independente de API).
 *
 * Implementa a criação/destruição da SDL_Window e os atributos de contexto
 * GL/EGL (configureContext). Os serviços específicos de cada API gráfica
 * (contexto GL, surface Vulkan, HWND nativo) ficam nas subclasses por-API
 * (SDL2GLDisplayBackend, SDL2VulkanDisplayBackend, SDL2D3D12DisplayBackend),
 * compiladas apenas nas plataformas que têm aquela API — assim nada de Vulkan
 * ou D3D12 entra, por exemplo, no build do 3DS.
 *
 * O SDL_Init/SDL_Quit é da SDL2MultimediaLayer, não daqui.
 *
 * @see DisplayBackend, SDL2MultimediaLayer
 */
class SDL2DisplayBackend : public DisplayBackend {
  public:
    ~SDL2DisplayBackend() override = default;

    void configureContext(const GraphicsAPI& graphicsAPI) override;
    void* createMainDisplay(unsigned int flags, const WindowDesc& opts) override;
    void destroyDisplay(void* display) override;
};

#endif // SDL2_DISPLAY_BACKEND_HPP
