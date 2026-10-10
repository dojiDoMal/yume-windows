#define CLASS_NAME "N3DSDisplayBackend"
#include "n3ds_display_backend.hpp"
#include "log_macros.hpp"

#include <3ds.h>
#include <citro3d.h>

// Flags de transferência do framebuffer da tela de cima para a saída (iguais
// aos do exemplo oficial do citro3d): sem flip, sem tiling de saída, entrada
// RGBA8 -> saída RGB8, sem escala.
#define DISPLAY_TRANSFER_FLAGS                                                                      \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |                \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |  \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

N3DSDisplayBackend::~N3DSDisplayBackend() { destroyDisplay(&gpuTarget); }

void N3DSDisplayBackend::configureContext(const GraphicsAPI& graphicsAPI) {
    // Nada a configurar antes da criação do display no 3DS (sem contexto GL/EGL
    // para ajustar). Mantido para cumprir a interface.
    (void)graphicsAPI;
}

void* N3DSDisplayBackend::createMainDisplay(unsigned int flags, const WindowDesc& opts) {
    (void)flags;
    (void)opts; // As telas do 3DS têm dimensões fixas; a WindowDesc não se aplica.

    // Inicializa a GPU (citro3d). O serviço de vídeo (gfxInitDefault) já foi
    // ligado pela N3DSMultimediaLayer antes desta chamada.
    if (!c3dInitialized) {
        if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
            LOG_ERROR("C3D_Init failed");
            return nullptr;
        }
        c3dInitialized = true;
    }

    // Render target da tela de cima: 240x400 (a tela é desenhada "de lado", daí
    // altura 240 e largura 400 nesta ordem), com cor RGBA8 e depth24+stencil8.
    gpuTarget.top = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    if (!gpuTarget.top) {
        LOG_ERROR("C3D_RenderTargetCreate (top) failed");
        return nullptr;
    }
    C3D_RenderTargetSetOutput(gpuTarget.top, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    // TODO: criar também o alvo da tela de baixo (GFX_BOTTOM, 240x320) quando o
    //       engine suportar render em múltiplas telas.
    return &gpuTarget;
}

void N3DSDisplayBackend::destroyDisplay(void* display) {
    (void)display; // Sempre o nosso gpuTarget interno.

    if (gpuTarget.top) {
        C3D_RenderTargetDelete(gpuTarget.top);
        gpuTarget.top = nullptr;
    }
    if (c3dInitialized) {
        C3D_Fini();
        c3dInitialized = false;
    }
}
