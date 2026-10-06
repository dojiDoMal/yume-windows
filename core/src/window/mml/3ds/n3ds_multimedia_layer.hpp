#ifndef N3DS_MULTIMEDIA_LAYER_HPP
#define N3DS_MULTIMEDIA_LAYER_HPP

#include "window/mml/multimedia_layer.hpp"
#include <memory>

/**
 * @brief Camada de multimídia para o Nintendo 3DS (Citro3D).
 *
 * Stub compilado apenas sob __3DS__, espelhando a convenção de camada por
 * plataforma. A implementação concreta (incluindo os sub-backends de display,
 * input e áudio do 3DS) chega junto com o suporte efetivo ao 3DS.
 *
 * @see MultimediaLayer
 */
class N3DSMultimediaLayer : public MultimediaLayer {
  public:
    bool init() override;
    void end() override;

    DisplayBackend& display() override;
    Yume::IInput& input() override;
    AudioBackend& audio() override;

  private:
    std::unique_ptr<DisplayBackend> displayBackend;
    std::unique_ptr<Yume::IInput> inputBackend;
    std::unique_ptr<AudioBackend> audioBackend;
};

#endif // N3DS_MULTIMEDIA_LAYER_HPP
