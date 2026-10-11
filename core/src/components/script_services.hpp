/**
 * @file script_services.hpp
 * @brief Serviços do engine disponibilizados aos scripts (.ys) em runtime.
 *
 * O ScriptComponent é criado pelo SceneLoader bem longe de onde o engine
 * monta seus subsistemas (input, config...), então não há como passar essas
 * dependências pela cadeia de construção dos componentes sem alterar várias
 * assinaturas. Como o engine roda uma única Application por processo (um input,
 * uma config), este holder expõe esses serviços de forma centralizada: a
 * Application os registra no boot (antes de iniciar os componentes) e o
 * ScriptComponent os lê ao montar a API `Yume.InputSystem` para o script.
 *
 * Não assume posse de nada: guarda apenas um ponteiro observador para o input
 * e uma cópia do mapa de aliases (nome lógico -> tecla estilo JS + eventType).
 */
#ifndef SCRIPT_SERVICES_HPP
#define SCRIPT_SERVICES_HPP

#include "renderer_config.hpp"

#include <string>
#include <unordered_map>

namespace Yume {

class IInput;

/**
 * @brief Ponto de injeção dos serviços do engine para os scripts.
 * @see Application::boot, ScriptComponent::start
 */
struct ScriptServices {
    /// @brief Tipo de um alias de input (nome lógico -> tecla + eventType).
    using InputAlias = RendererConfig::InputAlias;

    /** @brief Sistema de input ativo (não-dono). nullptr se ainda não registrado. */
    static IInput* input;
    /** @brief Aliases de input, por nome lógico (script). */
    static std::unordered_map<std::string, InputAlias> inputAliases;

    /** @brief Registra os serviços; chamado uma vez pela Application no boot. */
    static void configure(IInput* in, std::unordered_map<std::string, InputAlias> aliases);
};

} // namespace Yume

#endif // SCRIPT_SERVICES_HPP
