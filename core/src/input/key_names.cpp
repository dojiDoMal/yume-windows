/**
 * @file key_names.cpp
 * @brief Implementação da tradução de nomes de tecla por plataforma.
 *
 * O corpo é condicional por plataforma: no 3DS mapeia para as máscaras KEY_*
 * da libctru; nas demais (desktop/Switch/Web, todas baseadas em SDL) mapeia
 * para SDL_Keycode. Os nomes seguem o padrão dos keycodes de JavaScript
 * (ArrowUp, KeyA, Space, ...), com alguns apelidos comuns por conveniência.
 */
#include "input/key_names.hpp"

#include <unordered_map>

#if defined(__3DS__)
#include <3ds.h>
#else
#include <SDL_keycode.h>
#endif

namespace Yume {

namespace {

// Tabela nome -> KeyCode da plataforma. Construída uma vez (static local).
const std::unordered_map<std::string, KeyCode>& keyNameTable() {
#if defined(__3DS__)
    // 3DS (libctru): só existem os botões físicos do console. As setas do
    // padrão JS mapeiam para o D-pad; não há teclado, então letras/espaço não
    // têm correspondência e simplesmente não entram na tabela.
    static const std::unordered_map<std::string, KeyCode> table = {
        {"ArrowUp", static_cast<KeyCode>(KEY_DUP)},
        {"ArrowDown", static_cast<KeyCode>(KEY_DDOWN)},
        {"ArrowLeft", static_cast<KeyCode>(KEY_DLEFT)},
        {"ArrowRight", static_cast<KeyCode>(KEY_DRIGHT)},
        // Apelidos curtos.
        {"Up", static_cast<KeyCode>(KEY_DUP)},
        {"Down", static_cast<KeyCode>(KEY_DDOWN)},
        {"Left", static_cast<KeyCode>(KEY_DLEFT)},
        {"Right", static_cast<KeyCode>(KEY_DRIGHT)},
        // Botões de ação do console (úteis para quem quiser mapear além das setas).
        {"Enter", static_cast<KeyCode>(KEY_A)},
        {"Escape", static_cast<KeyCode>(KEY_B)},
        {"Space", static_cast<KeyCode>(KEY_A)},
    };
    return table;
#else
    // Desktop/Switch/Web (SDL): setas + um conjunto comum de teclas.
    static const std::unordered_map<std::string, KeyCode> table = {
        // Setas (requisito mínimo).
        {"ArrowUp", static_cast<KeyCode>(SDLK_UP)},
        {"ArrowDown", static_cast<KeyCode>(SDLK_DOWN)},
        {"ArrowLeft", static_cast<KeyCode>(SDLK_LEFT)},
        {"ArrowRight", static_cast<KeyCode>(SDLK_RIGHT)},
        // Apelidos curtos.
        {"Up", static_cast<KeyCode>(SDLK_UP)},
        {"Down", static_cast<KeyCode>(SDLK_DOWN)},
        {"Left", static_cast<KeyCode>(SDLK_LEFT)},
        {"Right", static_cast<KeyCode>(SDLK_RIGHT)},
        // Teclas de controle comuns.
        {"Space", static_cast<KeyCode>(SDLK_SPACE)},
        {"Enter", static_cast<KeyCode>(SDLK_RETURN)},
        {"Escape", static_cast<KeyCode>(SDLK_ESCAPE)},
        {"Tab", static_cast<KeyCode>(SDLK_TAB)},
        {"Backspace", static_cast<KeyCode>(SDLK_BACKSPACE)},
        {"ShiftLeft", static_cast<KeyCode>(SDLK_LSHIFT)},
        {"ShiftRight", static_cast<KeyCode>(SDLK_RSHIFT)},
        {"ControlLeft", static_cast<KeyCode>(SDLK_LCTRL)},
        {"ControlRight", static_cast<KeyCode>(SDLK_RCTRL)},
        // Letras: nome JS "KeyA".."KeyZ".
        {"KeyA", static_cast<KeyCode>(SDLK_a)}, {"KeyB", static_cast<KeyCode>(SDLK_b)},
        {"KeyC", static_cast<KeyCode>(SDLK_c)}, {"KeyD", static_cast<KeyCode>(SDLK_d)},
        {"KeyE", static_cast<KeyCode>(SDLK_e)}, {"KeyF", static_cast<KeyCode>(SDLK_f)},
        {"KeyG", static_cast<KeyCode>(SDLK_g)}, {"KeyH", static_cast<KeyCode>(SDLK_h)},
        {"KeyI", static_cast<KeyCode>(SDLK_i)}, {"KeyJ", static_cast<KeyCode>(SDLK_j)},
        {"KeyK", static_cast<KeyCode>(SDLK_k)}, {"KeyL", static_cast<KeyCode>(SDLK_l)},
        {"KeyM", static_cast<KeyCode>(SDLK_m)}, {"KeyN", static_cast<KeyCode>(SDLK_n)},
        {"KeyO", static_cast<KeyCode>(SDLK_o)}, {"KeyP", static_cast<KeyCode>(SDLK_p)},
        {"KeyQ", static_cast<KeyCode>(SDLK_q)}, {"KeyR", static_cast<KeyCode>(SDLK_r)},
        {"KeyS", static_cast<KeyCode>(SDLK_s)}, {"KeyT", static_cast<KeyCode>(SDLK_t)},
        {"KeyU", static_cast<KeyCode>(SDLK_u)}, {"KeyV", static_cast<KeyCode>(SDLK_v)},
        {"KeyW", static_cast<KeyCode>(SDLK_w)}, {"KeyX", static_cast<KeyCode>(SDLK_x)},
        {"KeyY", static_cast<KeyCode>(SDLK_y)}, {"KeyZ", static_cast<KeyCode>(SDLK_z)},
        // Dígitos: nome JS "Digit0".."Digit9".
        {"Digit0", static_cast<KeyCode>(SDLK_0)}, {"Digit1", static_cast<KeyCode>(SDLK_1)},
        {"Digit2", static_cast<KeyCode>(SDLK_2)}, {"Digit3", static_cast<KeyCode>(SDLK_3)},
        {"Digit4", static_cast<KeyCode>(SDLK_4)}, {"Digit5", static_cast<KeyCode>(SDLK_5)},
        {"Digit6", static_cast<KeyCode>(SDLK_6)}, {"Digit7", static_cast<KeyCode>(SDLK_7)},
        {"Digit8", static_cast<KeyCode>(SDLK_8)}, {"Digit9", static_cast<KeyCode>(SDLK_9)},
    };
    return table;
#endif
}

} // namespace

bool resolveKeyName(const std::string& name, KeyCode& out) {
    const auto& table = keyNameTable();
    auto it = table.find(name);
    if (it == table.end())
        return false;
    out = it->second;
    return true;
}

} // namespace Yume
