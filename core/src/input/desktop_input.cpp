#include "desktop_input.hpp"
#include <SDL_keycode.h>

void Yume::DesktopInput::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            quit_requested = true;
        }
        if (event.type == SDL_KEYDOWN) {
            auto it = key_bindings.find(event.key.keysym.sym);
            if (it != key_bindings.end()) {
                it->second();
            }
        }
    }
}

bool Yume::DesktopInput::getQuitEvent() { 
  return quit_requested; 
}
  
void Yume::DesktopInput::requestQuit() { 
  quit_requested = true; 
}

void Yume::DesktopInput::bindKey(KeyCode key, ActionCallback callback) {
    
    key_bindings[key] = callback;
}
