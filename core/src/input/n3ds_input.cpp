#include "input/n3ds_input.hpp"
#include <3ds.h>

namespace Yume {

void N3DSInput::processEvents() {
    hidScanInput();
    downMask = hidKeysDown();
    heldMask = hidKeysHeld();

    // Convenção dos exemplos do 3DS: START encerra a aplicação. KEY_START vem de
    // um enum sem nome da libctru; o cast evita o erro de operand & com u32.
    if (downMask & static_cast<unsigned int>(KEY_START))
        quitRequested = true;

    // Dispara os callbacks das teclas acionadas neste frame.
    for (auto& [key, callback] : keyBindings) {
        if ((downMask & static_cast<unsigned int>(key)) && callback)
            callback();
    }
}

void N3DSInput::bindKey(KeyCode key, std::function<void()> callback) {
    keyBindings[key] = std::move(callback);
}

bool N3DSInput::getQuitEvent() { return quitRequested; }

void N3DSInput::requestQuit() { quitRequested = true; }

bool N3DSInput::isKeyPressed(KeyCode key) {
    return (heldMask & static_cast<unsigned int>(key)) != 0;
}

} // namespace Yume
