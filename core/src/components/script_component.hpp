/**
 * @file script_component.hpp
 * @brief Componente que executa um script YumeScript (.ys) por frame.
 *
 * Guarda o caminho de um arquivo .ys e, no ciclo de vida do componente, roda o
 * script: em start() carrega o fonte, injeta a API disponível (o `transform` do
 * objeto dono e funções nativas como `log`) e chama a função start() do script,
 * se existir; em update(dt) chama a função update(dt) do script, se existir.
 *
 * Esta é a ponte da Fase 1 (ver yumescript/DESIGN.md): o .ys é lido e analisado
 * em runtime. O acesso a input/assets/renderer fica para fases futuras; por ora
 * o script manipula o Transform do próprio objeto.
 */
#ifndef SCRIPT_COMPONENT_HPP
#define SCRIPT_COMPONENT_HPP

#include "components/component.hpp"
#include "yumescript/yumescript.hpp"

#include <memory>
#include <string>

/**
 * @brief Componente de script YumeScript anexável a um WorldObject.
 * @see Component, yumescript::Script
 */
class ScriptComponent : public Component {
  public:
    /** @brief Cria o componente apontando para um arquivo .ys. */
    explicit ScriptComponent(std::string scriptPath);
    ~ScriptComponent() override = default;

    /** @brief Carrega o script, injeta a API e chama o start() do script. */
    void start() override;
    /** @brief Chama a função update(dt) do script, se definida. */
    void update(float deltaTime) override;

    /** @brief Caminho do arquivo .ys associado. */
    const std::string& getScriptPath() const { return scriptPath; }

  private:
    std::string scriptPath;                     ///< Caminho do arquivo .ys.
    std::unique_ptr<yumescript::Script> script; ///< Runtime do script carregado.
    bool loaded = false;                        ///< @c true se o script carregou sem erro.
    bool hasUpdate = false;                     ///< @c true se o script define update().

    /** @brief Monta o objeto `this` exposto ao script (o objeto dono). */
    yumescript::Value buildThisObject();
    /** @brief Monta o objeto `transform` do dono, acessível via `this.transform`. */
    yumescript::Value buildTransformObject();
};

#endif // SCRIPT_COMPONENT_HPP
