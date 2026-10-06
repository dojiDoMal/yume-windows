#ifndef SDL2_D3D12_DISPLAY_BACKEND_HPP
#define SDL2_D3D12_DISPLAY_BACKEND_HPP

#include "window/mml/sdl2/sdl2_display_backend.hpp"

/**
 * @brief Backend de janela SDL2 para DirectX 12 (Windows).
 *
 * Acrescenta à base a extração do handle nativo (HWND) via SDL_GetWindowWMInfo,
 * de que o D3D12 precisa para criar a swapchain. É o único ponto que chama
 * SDL_syswm; o D3D12RendererBackend consome via void*. Compilado apenas no
 * Windows.
 *
 * @see SDL2DisplayBackend, DisplayBackend, D3D12RendererBackend
 */
class SDL2D3D12DisplayBackend : public SDL2DisplayBackend {
  public:
    void* getNativeWindowHandle(void* window) override;
};

#endif // SDL2_D3D12_DISPLAY_BACKEND_HPP
