/**
 * @file key_names.hpp
 * @brief Tradução de nomes de tecla (estilo JavaScript) para KeyCode nativo.
 *
 * Os aliases de input em project.conf usam nomes de tecla no padrão dos
 * keycodes de JavaScript (ex.: "ArrowUp", "KeyA", "Space"), ver
 * https://www.toptal.com/developers/keycode. Como cada plataforma representa
 * teclas de um jeito diferente (SDL_Keycode no desktop, máscaras KEY_* da
 * libctru no 3DS), a tradução para o KeyCode nativo acontece aqui, isolada por
 * plataforma, de modo que o resto do engine só lida com nomes lógicos.
 */
#ifndef INPUT_KEY_NAMES_HPP
#define INPUT_KEY_NAMES_HPP

#include "input/input_key.hpp"

#include <string>

namespace Yume {

/**
 * @brief Converte um nome de tecla (estilo JS) para o KeyCode da plataforma.
 *
 * @param name  Nome da tecla, ex.: "ArrowUp". A busca é sensível a maiúsculas
 *              para casar com os nomes do padrão JavaScript, mas também aceita
 *              apelidos comuns ("Up", "Left"...) por conveniência.
 * @param out   Recebe o KeyCode nativo correspondente quando houver mapeamento.
 * @return @c true se o nome foi reconhecido nesta plataforma; @c false caso
 *         contrário (nesse caso @p out não é alterado).
 */
bool resolveKeyName(const std::string& name, KeyCode& out);

} // namespace Yume

#endif // INPUT_KEY_NAMES_HPP
