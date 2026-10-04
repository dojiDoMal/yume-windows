#ifndef IINPUT_FACTORY_HPP
#define IINPUT_FACTORY_HPP

#include "i_input.hpp"

namespace Yume {

/**
 * @brief Fábrica que cria a implementação de IInput adequada à plataforma.
 *
 * Esconde a escolha do backend de input: no desktop devolve um DesktopInput
 * (baseado em SDL), e assim por diante conforme a plataforma de compilação.
 *
 * @see IInput, DesktopInput
 */
class IInputFactory {
  public:
    /**
     * @brief Cria o sistema de input para a plataforma atual.
     * @return Ponteiro para a implementação de IInput (o chamador assume a posse).
     */
    static IInput* create();
};
} // namespace Yume

#endif
