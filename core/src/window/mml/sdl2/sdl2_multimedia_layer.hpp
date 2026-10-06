#ifndef SDL2_MULTIMEDIA_LAYER_HPP
#define SDL2_MULTIMEDIA_LAYER_HPP

#include "window/mml/multimedia_layer.hpp"
#include <memory>

/**
 * @brief Camada de multimídia baseada em SDL2 (desktop e Switch).
 *
 * Dona do ciclo de vida do SDL: init() faz SDL_Init(VIDEO | AUDIO) uma única
 * vez e cria os sub-backends; end() fecha-os e chama SDL_Quit(). Centraliza
 * aqui o que antes ficava espalhado (vídeo no antigo SDL2Layer e o estado
 * global implícito do qual o input dependia).
 *
 * @see MultimediaLayer, SDL2DisplayBackend, SDL2AudioBackend
 */
class SDL2MultimediaLayer : public MultimediaLayer {
  public:
    /**
     * @brief Constrói a camada para a API gráfica ativa.
     * @param api API gráfica resolvida; seleciona qual display backend SDL2
     *        (GL/Vulkan/D3D12) será instanciado em init().
     */
    explicit SDL2MultimediaLayer(const GraphicsAPI& api);
    ~SDL2MultimediaLayer() override;

    bool init() override;
    void end() override;

    DisplayBackend& display() override;
    Yume::IInput& input() override;
    AudioBackend& audio() override;

  private:
    GraphicsAPI graphicsApi;
    bool initialized = false;
    std::unique_ptr<DisplayBackend> displayBackend;
    std::unique_ptr<Yume::IInput> inputBackend;
    std::unique_ptr<AudioBackend> audioBackend;
};

#endif // SDL2_MULTIMEDIA_LAYER_HPP
