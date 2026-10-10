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
    // Construtor e destrutor declarados aqui mas DEFINIDOS no .cpp (onde os
    // tipos completos de IInput/DisplayBackend/AudioBackend estão incluídos).
    // Sem isso, qualquer TU que apenas inclua este header e instancie/destrua a
    // classe (ex.: multimedia_layer_factory.cpp) tentaria gerar o ctor/dtor dos
    // membros unique_ptr<IInput> inline, exigindo o tipo completo de IInput ali
    // -- que é só forward-declared em multimedia_layer.hpp. Mantê-los no .cpp
    // é o idiom "pimpl/incomplete-type unique_ptr".
    N3DSMultimediaLayer();
    ~N3DSMultimediaLayer() override;

    bool init() override;
    void end() override;

    DisplayBackend& display() override;
    Yume::IInput& input() override;
    AudioBackend& audio() override;

  private:
    bool initialized = false;
    std::unique_ptr<DisplayBackend> displayBackend;
    std::unique_ptr<Yume::IInput> inputBackend;
    std::unique_ptr<AudioBackend> audioBackend;
};

#endif // N3DS_MULTIMEDIA_LAYER_HPP
