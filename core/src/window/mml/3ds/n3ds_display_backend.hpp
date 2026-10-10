#ifndef N3DS_DISPLAY_BACKEND_HPP
#define N3DS_DISPLAY_BACKEND_HPP

#include "graphics_api.hpp"
#include "window/mml/display_backend.hpp"

#include "renderer/backends/citro3d/citro3d_gpu_target.hpp"

/**
 * @brief Backend de janela/apresentação para o Nintendo 3DS (Citro3D).
 *
 * No 3DS não há "janela": há duas telas físicas e uma GPU (PICA200) acionada
 * via citro3d. Seguindo a convenção do engine ("o DisplayBackend é o único a
 * falar com a plataforma de vídeo"), é aqui que a GPU é inicializada:
 * createMainDisplay faz C3D_Init e cria o render target da tela de cima,
 * devolvendo um Citro3DGpuTarget como o "window" opaco que o
 * Citro3DRendererBackend recebe em init(). A inicialização do serviço de vídeo
 * em si (gfxInitDefault) é feita pela N3DSMultimediaLayer, dona do ciclo de
 * vida do subsistema.
 *
 * @see DisplayBackend, N3DSMultimediaLayer, Citro3DRendererBackend
 */
class N3DSDisplayBackend : public DisplayBackend {
  private:
    Citro3DGpuTarget gpuTarget;  ///< Alvos de render (tela de cima por ora).
    bool c3dInitialized = false; ///< @c true após C3D_Init bem-sucedido.

  public:
    ~N3DSDisplayBackend() override;

    void configureContext(const GraphicsAPI& graphicsAPI) override;
    void* createMainDisplay(unsigned int flags, const WindowDesc& opts) override;
    void destroyDisplay(void* display) override;
};

#endif // N3DS_DISPLAY_BACKEND_HPP
