#ifndef CITRO3D_GPU_TARGET_HPP
#define CITRO3D_GPU_TARGET_HPP

#include <citro3d.h>

/**
 * @brief Alvos de render do 3DS, compartilhados entre display e renderer.
 *
 * O 3DS tem duas telas. Por enquanto o engine desenha apenas na tela de cima
 * (400x240). Esta struct é criada pelo N3DSDisplayBackend::createMainDisplay
 * (que também faz gfxInitDefault/C3D_Init via a MultimediaLayer) e devolvida
 * como o "window" opaco que o Citro3DRendererBackend recebe em init(). O
 * backend usa `top` para C3D_FrameDrawOn / C3D_RenderTargetClear.
 *
 * @todo Expor a tela de baixo (bottom, 320x240) quando o engine suportar
 *       múltiplos alvos / render em duas telas.
 *
 * @see N3DSDisplayBackend, Citro3DRendererBackend
 */
struct Citro3DGpuTarget {
    C3D_RenderTarget* top = nullptr; ///< Alvo da tela de cima (400x240).
};

#endif // CITRO3D_GPU_TARGET_HPP
