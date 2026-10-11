#ifndef DESKTOP_INPUT_HPP
#define DESKTOP_INPUT_HPP

#include "input/i_input.hpp"
#include "input/input_key.hpp"
#include <SDL_keycode.h>
#include <functional>
#include <unordered_map>
#include <unordered_set>

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

    using ActionCallback = std::function<void(float)>;

    void processEvents(float deltaTime) override;
    void bindKey(KeyCode key, ActionCallback callback,
                 KeyEventType eventType = KeyEventType::KeyDown) override;
    bool getQuitEvent() override;
    void requestQuit() override;
    bool isKeyPressed(KeyCode key) override;
    bool wasKeyPressed(KeyCode key) override;
    bool wasKeyReleased(KeyCode key) override;

  private:
    /// @brief Callback de uma tecla + quando ele deve disparar.
    struct Binding {
        ActionCallback callback;
        KeyEventType eventType = KeyEventType::KeyDown;
    };

    const Uint8* keyboard_state = nullptr; ///< Estado atual do teclado (SDL).
    bool quit_requested = false;           ///< @c true quando o encerramento foi pedido.
    std::unordered_map<KeyCode, Binding> key_bindings; ///< Bindings associados por tecla.

    // Bordas deste frame, preenchidas pelos eventos SDL_KEYDOWN/UP em
    // processEvents e consultadas por wasKeyPressed/wasKeyReleased. Limpas no
    // início de cada processEvents.
    std::unordered_set<KeyCode> pressed_this_frame;  ///< Teclas que baixaram neste frame.
    std::unordered_set<KeyCode> released_this_frame; ///< Teclas que subiram neste frame.
};
} // namespace Yume

#endif
