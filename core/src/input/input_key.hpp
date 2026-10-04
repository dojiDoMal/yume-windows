#ifndef INPUT_KEY_HPP
#define INPUT_KEY_HPP

#include <cstdint>

namespace Yume {
/**
 * @brief Identificador de uma tecla ou botão.
 *
 * É um tipo inteiro largo o bastante para acomodar os códigos de tecla de
 * diferentes backends (ex.: SDL_Keycode no desktop), evitando acoplar o resto
 * do engine a uma biblioteca específica.
 */
using KeyCode = intptr_t;
}

#endif
