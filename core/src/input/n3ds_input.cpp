#include "input/n3ds_input.hpp"
#include <3ds.h>

namespace Yume {

void N3DSInput::processEvents(float deltaTime) {
    hidScanInput();
    downMask = hidKeysDown();
    heldMask = hidKeysHeld();
    upMask = hidKeysUp();

    // Convenção dos exemplos do 3DS: START encerra a aplicação. KEY_START vem de
    // um enum sem nome da libctru; o cast evita o erro de operand & com u32.
    if (downMask & static_cast<unsigned int>(KEY_START))
        quitRequested = true;

    // Dispara os callbacks conforme o tipo de evento de cada binding, passando
    // dt:
    //  - KeyDown: botão baixou neste frame (downMask).
    //  - KeyUp:   botão subiu neste frame (upMask).
    //  - KeyHold: botão mantido pressionado (heldMask).
    for (auto& [key, binding] : keyBindings) {
        if (!binding.callback)
            continue;
        unsigned int mask = heldMask;
        if (binding.eventType == KeyEventType::KeyDown)
            mask = downMask;
        else if (binding.eventType == KeyEventType::KeyUp)
            mask = upMask;
        if (mask & static_cast<unsigned int>(key))
            binding.callback(deltaTime);
    }
}

void N3DSInput::bindKey(KeyCode key, std::function<void(float)> callback, KeyEventType eventType) {
    keyBindings[key] = Binding{std::move(callback), eventType};
}

bool N3DSInput::getQuitEvent() { return quitRequested; }

void N3DSInput::requestQuit() { quitRequested = true; }

bool N3DSInput::isKeyPressed(KeyCode key) {
    return (heldMask & static_cast<unsigned int>(key)) != 0;
}

bool N3DSInput::wasKeyPressed(KeyCode key) {
    return (downMask & static_cast<unsigned int>(key)) != 0;
}

bool N3DSInput::wasKeyReleased(KeyCode key) {
    return (upMask & static_cast<unsigned int>(key)) != 0;
}

} // namespace Yume
