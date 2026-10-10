#ifndef N3DS_INPUT_HPP
#define N3DS_INPUT_HPP

#include "input/i_input.hpp"
#include "input/input_key.hpp"
#include <functional>
#include <unordered_map>

namespace Yume {

/**
 * @brief Implementação de IInput para o Nintendo 3DS (libctru/hid).
 *
 * Lê os botões do console via hidScanInput/hidKeysDown a cada processEvents().
 * O pedido de encerramento segue a convenção dos exemplos do 3DS: START fecha a
 * aplicação. Os KeyCode usados em bindKey/isKeyPressed são as máscaras KEY_*
 * da libctru (ex.: KEY_A), passadas como intptr_t.
 *
 * @note Backend mínimo para destravar a camada de multimídia do 3DS; o
 *       mapeamento completo de teclas/eventos pode evoluir depois.
 *
 * @see IInput
 */
class N3DSInput : public IInput {
  public:
    void processEvents() override;
    void bindKey(KeyCode key, std::function<void()> callback) override;
    bool getQuitEvent() override;
    void requestQuit() override;
    bool isKeyPressed(KeyCode key) override;

  private:
    bool quitRequested = false;
    // Nomes propositalmente diferentes de hidKeysDown()/hidKeysHeld() da libctru
    // para não colidir com as funções (um campo "keysDown" sombrearia a função
    // e "keysDown = hidKeysDown()" viraria atribuição à função).
    unsigned int downMask = 0; ///< Máscara de teclas pressionadas neste frame.
    unsigned int heldMask = 0; ///< Máscara de teclas mantidas pressionadas.
    std::unordered_map<KeyCode, std::function<void()>> keyBindings;
};

} // namespace Yume

#endif // N3DS_INPUT_HPP
