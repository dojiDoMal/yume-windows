#ifndef MULTIMEDIA_LAYER_HPP
#define MULTIMEDIA_LAYER_HPP

#include "graphics_api.hpp"
#include "window/mml/audio/audio_backend.hpp"
#include "window/mml/display_backend.hpp"

namespace Yume {
class IInput;
}

/**
 * @brief Camada de multimídia da plataforma: agrupa display, input e áudio.
 *
 * É o "nível acima" dos backends: um único ponto que inicializa o subsistema de
 * multimídia da plataforma (ex.: SDL_Init de vídeo+áudio) e detém os três
 * sub-backends que compartilham esse mesmo subsistema:
 *
 * - display(): janela/apresentação (DisplayBackend)
 * - input():   teclado/eventos (IInput)
 * - audio():   dispositivo de áudio (AudioBackend)
 *
 * A posse do ciclo de vida da plataforma vive aqui: init() liga tudo e end()
 * desliga, de forma determinística. Os sub-backends não chamam SDL_Init/SDL_Quit
 * por conta própria.
 *
 * Cada plataforma fornece uma implementação (ex.: SDL2MultimediaLayer,
 * N3DSMultimediaLayer); instancie via MultimediaLayerFactory.
 *
 * @see DisplayBackend, AudioBackend, Yume::IInput, MultimediaLayerFactory
 */
class MultimediaLayer {
  public:
    virtual ~MultimediaLayer() = default;

    /**
     * @brief Inicializa o subsistema de multimídia da plataforma e os sub-backends.
     * @return @c true em sucesso; @c false se o subsistema não pôde subir.
     */
    virtual bool init() = 0;

    /** @brief Desliga os sub-backends e finaliza o subsistema de multimídia. */
    virtual void end() = 0;

    /** @brief Backend de janela/apresentação. Válido após init(). */
    virtual DisplayBackend& display() = 0;
    /** @brief Sistema de input. Válido após init(). */
    virtual Yume::IInput& input() = 0;
    /** @brief Backend de áudio. Válido após init(). */
    virtual AudioBackend& audio() = 0;
};

#endif // MULTIMEDIA_LAYER_HPP
