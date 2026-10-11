#include "input/desktop_input.hpp"
#include <SDL_keycode.h>

void Yume::DesktopInput::processEvents(float deltaTime) {
    // Zera as bordas do frame anterior; serão repreenchidas pelos eventos abaixo.
    pressed_this_frame.clear();
    released_this_frame.clear();

    // 1) Eventos de borda do SDL. KEYDOWN com repeat==0 = pressionou agora;
    //    KEYUP = soltou agora. Guardamos em conjuntos para wasKeyPressed/
    //    wasKeyReleased e para disparar os callbacks KeyDown/KeyUp.
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            quit_requested = true;
        } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
            pressed_this_frame.insert(event.key.keysym.sym);
        } else if (event.type == SDL_KEYUP) {
            released_this_frame.insert(event.key.keysym.sym);
        }
    }

    // 2) Dispara os callbacks conforme o tipo de cada binding, passando dt:
    //    - KeyDown: tecla baixou neste frame.
    //    - KeyUp:   tecla subiu neste frame.
    //    - KeyHold: tecla está pressionada agora (estado contínuo).
    for (auto& [key, binding] : key_bindings) {
        if (!binding.callback)
            continue;
        switch (binding.eventType) {
        case KeyEventType::KeyDown:
            if (pressed_this_frame.count(key))
                binding.callback(deltaTime);
            break;
        case KeyEventType::KeyUp:
            if (released_this_frame.count(key))
                binding.callback(deltaTime);
            break;
        case KeyEventType::KeyHold:
            if (isKeyPressed(key))
                binding.callback(deltaTime);
            break;
        }
    }
}

bool Yume::DesktopInput::getQuitEvent() { return quit_requested; }

void Yume::DesktopInput::requestQuit() { quit_requested = true; }

void Yume::DesktopInput::bindKey(KeyCode key, ActionCallback callback, KeyEventType eventType) {
    key_bindings[key] = Binding{std::move(callback), eventType};
}

bool Yume::DesktopInput::isKeyPressed(KeyCode key) {
    if (!keyboard_state) {
        keyboard_state = SDL_GetKeyboardState(nullptr);
    }
    return keyboard_state[SDL_GetScancodeFromKey(key)];
}

bool Yume::DesktopInput::wasKeyPressed(KeyCode key) { return pressed_this_frame.count(key) != 0; }

bool Yume::DesktopInput::wasKeyReleased(KeyCode key) { return released_this_frame.count(key) != 0; }
