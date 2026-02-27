#ifndef DESKTOP_INPUT_HPP
#define DESKTOP_INPUT_HPP

#include "i_input.hpp"
#include "input_key.hpp"
#include <SDL_keycode.h>
#include <functional>

namespace Yume {
class DesktopInput : public IInput {

    using ActionCallback = std::function<void()>;

    void processEvents() override;
    void bindKey(KeyCode key, ActionCallback callback) override;
    bool getQuitEvent() override;
    void requestQuit() override;
    bool isKeyPressed(KeyCode key) override;

  private:
    const Uint8* keyboard_state = nullptr;
    bool quit_requested = false;
    std::unordered_map<KeyCode, ActionCallback> key_bindings;
};
} // namespace Yume

#endif