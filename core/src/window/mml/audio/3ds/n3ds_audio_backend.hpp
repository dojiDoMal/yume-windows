#ifndef N3DS_AUDIO_BACKEND_HPP
#define N3DS_AUDIO_BACKEND_HPP

#include "window/mml/audio/audio_backend.hpp"

/**
 * @brief Backend de áudio do Nintendo 3DS (stub).
 *
 * Implementação mínima para completar a camada de multimídia do 3DS. Hoje só
 * guarda o volume master; a reprodução via o serviço de áudio da libctru (ndsp)
 * entra depois sobre esta mesma interface.
 *
 * @todo Integrar ndsp (ndspInit/ndspChn*) para reprodução real de áudio.
 * @see AudioBackend
 */
class N3DSAudioBackend : public AudioBackend {
  public:
    bool open() override { return true; }
    void close() override {}
    void setMasterVolume(float volume) override {
        if (volume < 0.0f)
            volume = 0.0f;
        if (volume > 1.0f)
            volume = 1.0f;
        masterVolume = volume;
    }
    float getMasterVolume() const override { return masterVolume; }

  private:
    float masterVolume = 1.0f;
};

#endif // N3DS_AUDIO_BACKEND_HPP
