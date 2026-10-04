#ifndef ENGINE_CONTEXT_HPP
#define ENGINE_CONTEXT_HPP

#include "input/i_input.hpp"

namespace Yume {

/**
 * @brief Agrega os subsistemas compartilhados do engine em um único contexto.
 *
 * Dá acesso centralizado a serviços de longa duração (hoje, o sistema de
 * input) para quem precisar deles durante a execução. É criado e mantido pela
 * Application.
 */
class Context {
  public:
    /**
     * @brief Constrói o contexto com os subsistemas fornecidos.
     * @param input Sistema de input a ser exposto (não assume a posse).
     */
    Context(IInput* input);
    ~Context();

    IInput* input; ///< Sistema de input ativo.

    /** @brief Retorna o sistema de input como referência. */
    IInput& getInputSystem();
};
} // namespace Yume

#endif
