#ifndef I_INPUT_HPP
#define I_INPUT_HPP

#include "input/input_key.hpp"
#include <SDL2/SDL.h>
#include <functional>

namespace Yume {

/**
 * @brief Interface do sistema de input, independente de plataforma.
 *
 * Abstrai teclado e eventos de janela: o jogo associa callbacks a teclas com
 * bindKey e, a cada frame, processEvents() processa a fila de eventos e dispara
 * os callbacks. Também é possível consultar o estado de uma tecla diretamente
 * ou detectar o pedido de encerramento (fechar a janela).
 *
 * Cada plataforma fornece uma implementação (ex.: DesktopInput com SDL);
 * instancie via IInputFactory.
 *
 * @see DesktopInput, IInputFactory
 */
class IInput {
  public:
    virtual ~IInput() = default;

    /** @brief Processa a fila de eventos e dispara os callbacks de teclas; chame uma vez por frame. */
    virtual void processEvents() = 0;
    /**
     * @brief Associa um callback a uma tecla.
     * @param key      Tecla a observar.
     * @param callback Função chamada quando a tecla é acionada.
     */
    virtual void bindKey(KeyCode key, std::function<void()> callback) = 0;
    /** @brief Indica se foi solicitado o encerramento (ex.: janela fechada). */
    virtual bool getQuitEvent() = 0;
    /** @brief Solicita o encerramento da aplicação. */
    virtual void requestQuit() = 0;
    /** @brief Consulta se uma tecla está pressionada no momento. */
    virtual bool isKeyPressed(KeyCode key) = 0;
};
} // namespace Yume

#endif
