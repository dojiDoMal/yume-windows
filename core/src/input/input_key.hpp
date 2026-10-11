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

/**
 * @brief Quando um binding de tecla dispara seu callback.
 *
 * - @c KeyDown: dispara uma vez, na borda de pressionar (padrão). Transição
 *   solto->pressionado neste frame.
 * - @c KeyHold: dispara a cada frame enquanto a tecla permanece pressionada,
 *   útil para movimento contínuo (ex.: segurar a seta para andar).
 * - @c KeyUp: dispara uma vez, na borda de soltar. Transição pressionado->solto
 *   neste frame.
 *
 * No project.conf é escolhido por alias com
 * `"eventType": "keydown" | "keyhold" | "keyup"` (keydown é o padrão).
 */
enum class KeyEventType {
    KeyDown, ///< Borda de pressionar (uma vez por toque).
    KeyHold, ///< Enquanto mantida pressionada (todo frame).
    KeyUp,   ///< Borda de soltar (uma vez ao liberar).
};
} // namespace Yume

#endif
