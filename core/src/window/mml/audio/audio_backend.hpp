#ifndef AUDIO_BACKEND_HPP
#define AUDIO_BACKEND_HPP

#include <string>

/**
 * @brief Backend de áudio de uma plataforma.
 *
 * Terceiro membro da camada de multimídia (ao lado de display e input). Hoje é
 * só o contrato: abrir/fechar o dispositivo de áudio e controlar o volume
 * master. A reprodução de sons/música entra depois sobre esta interface.
 *
 * Não é dono do ciclo de vida do subsistema de plataforma (SDL_Init/SDL_Quit) —
 * isso pertence à MultimediaLayer. open()/close() aqui tratam apenas do
 * dispositivo de áudio em si.
 *
 * Cada plataforma fornece uma implementação (ex.: SDL2AudioBackend). A
 * MultimediaLayer cria e detém a instância.
 *
 * @see MultimediaLayer, SDL2AudioBackend
 */
class AudioBackend {
  public:
    virtual ~AudioBackend() = default;

    /**
     * @brief Abre o dispositivo de áudio da plataforma.
     * @return @c true em sucesso; @c false se o dispositivo não pôde ser aberto.
     */
    virtual bool open() = 0;

    /** @brief Fecha o dispositivo de áudio aberto por open(). */
    virtual void close() = 0;

    /**
     * @brief Define o volume master.
     * @param volume Valor em [0.0, 1.0]; fora do intervalo é saturado.
     */
    virtual void setMasterVolume(float volume) = 0;

    /** @brief Volume master atual, em [0.0, 1.0]. */
    virtual float getMasterVolume() const = 0;
};

#endif // AUDIO_BACKEND_HPP
