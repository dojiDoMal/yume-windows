#ifndef ENGINE_CONTEXT_HPP
#define ENGINE_CONTEXT_HPP

#include "input/i_input.hpp"

class MultimediaLayer;
class AudioBackend;
class DisplayBackend;

namespace Yume {

/**
 * @brief Agrega os subsistemas compartilhados do engine em um único contexto.
 *
 * Dá acesso centralizado a serviços de longa duração para quem precisar deles
 * durante a execução. Hoje gira em torno da MultimediaLayer — a camada que
 * agrupa display, input e áudio sobre o mesmo backend de plataforma. É criado e
 * mantido pela Application, que também é dona da MultimediaLayer.
 */
class Context {
  public:
    /**
     * @brief Constrói o contexto sobre a camada de multimídia fornecida.
     * @param multimedia Camada de multimídia ativa (não assume a posse; a
     *        Application continua dona dela).
     */
    explicit Context(MultimediaLayer* multimedia);
    ~Context();

    MultimediaLayer* multimedia; ///< Camada de multimídia ativa (display + input + áudio).

    /** @brief Retorna o sistema de input como referência. */
    IInput& getInputSystem();
    /** @brief Retorna o backend de áudio como referência. */
    AudioBackend& getAudioSystem();
    /** @brief Retorna o backend de display como referência. */
    DisplayBackend& getDisplaySystem();
};
} // namespace Yume

#endif
