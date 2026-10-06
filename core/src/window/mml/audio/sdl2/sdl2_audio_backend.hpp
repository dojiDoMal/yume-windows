#ifndef SDL2_AUDIO_BACKEND_HPP
#define SDL2_AUDIO_BACKEND_HPP

#include "window/mml/audio/audio_backend.hpp"

/**
 * @brief Stub de áudio baseado em SDL2.
 *
 * Esqueleto: implementa a interface AudioBackend mas ainda não reproduz som.
 * open()/close() apenas registram o estado do dispositivo e setMasterVolume
 * guarda o valor saturado em [0,1]. O subsistema de áudio do SDL
 * (SDL_INIT_AUDIO) é inicializado pela SDL2MultimediaLayer, não aqui.
 *
 * Serve para firmar a arquitetura (display + input + audio sob a mesma camada)
 * antes de ligar a reprodução de verdade.
 *
 * @see AudioBackend, SDL2MultimediaLayer
 */
class SDL2AudioBackend : public AudioBackend {
  public:
    ~SDL2AudioBackend() = default;

    bool open() override;
    void close() override;
    void setMasterVolume(float volume) override;
    float getMasterVolume() const override;

  private:
    bool opened = false;         ///< Dispositivo lógico aberto.
    float masterVolume = 1.0f;   ///< Volume master atual, em [0,1].
};

#endif // SDL2_AUDIO_BACKEND_HPP
