#ifndef DESKTOP_INPUT_HPP
#define DESKTOP_INPUT_HPP

#include "i_input.hpp"
#include "input_key.hpp"
#include <SDL_keycode.h>
#include <functional>

namespace Yume {

/**
 * @brief Implementação de IInput para desktop, baseada em SDL.
 *
 * Lê o estado do teclado e a fila de eventos do SDL, dispara os callbacks das
 * teclas associadas e rastreia o pedido de encerramento (fechar a janela).
 * Criada pela IInputFactory no desktop.
 *
 * @see IInput, IInputFactory
 */
class DesktopInput : public IInput {

    using ActionCallback = std::function<void()>;

    void processEvents() override;
    void bindKey(KeyCode key, ActionCallback callback) override;
    bool getQuitEvent() override;
    void requestQuit() override;
    bool isKeyPressed(KeyCode key) override;

  private:
    const Uint8* keyboard_state = nullptr;                        ///< Estado atual do teclado (SDL).
    bool quit_requested = false;                                  ///< @c true quando o encerramento foi pedido.
    std::unordered_map<KeyCode, ActionCallback> key_bindings;     ///< Callbacks associados por tecla.
};
} // namespace Yume

#endif
