#ifndef YUME_APPLICATION_HPP
#define YUME_APPLICATION_HPP

#include "engine_context.hpp"
#include "input/i_input.hpp"
#include "renderer/renderer_backend.hpp"
#include "renderer_config.hpp"
#include "scene_manager.hpp"
#include "window/window_desc.hpp"
#include "window/window_manager.hpp"

#include <memory>
#include <string>

namespace Yume {

/**
 * @brief Framework de aplicação reutilizável — ponto de partida de um projeto.
 *
 * O engine assume todo o trabalho repetitivo: cria a janela, o backend de
 * renderização, o gerenciador de cenas, o sistema de input e o laço principal.
 * O projeto herda de Application e preenche só o que é específico dele,
 * sobrescrevendo os hooks:
 *
 * - onInit():     chamado uma vez, após o engine subir e a cena inicial (de
 *                 project.conf) ser carregada. Use para configurar input e
 *                 obter referências a objetos.
 * - onUpdate(dt): chamado a cada frame, com o delta em segundos. Lógica de jogo
 *                 por frame vai aqui (ex.: girar um objeto).
 *
 * Um main() de projeto fica assim:
 * @code
 * class MyApp : public Yume::Application { ... };
 * int main() { MyApp app; return app.run(); }
 * @endcode
 *
 * As configurações (tamanho/título da janela, API gráfica, cena inicial) vêm de
 * project.conf, resolvido relativo ao diretório de trabalho — que o build
 * aponta para a pasta com os assets compilados.
 */
class Application {
  public:
    Application();
    virtual ~Application();

    /**
     * @brief Inicializa o engine, roda o laço principal até o encerramento e desliga tudo.
     * @return Código de saída do processo (0 = sucesso).
     */
    int run();

  protected:
    /// @brief Hook chamado uma vez após a inicialização. Padrão: não faz nada.
    virtual void onInit() {}
    /**
     * @brief Hook chamado a cada frame. Padrão: não faz nada.
     * @param deltaTime Tempo decorrido desde o frame anterior, em segundos.
     */
    virtual void onUpdate(float deltaTime) { (void)deltaTime; }

    /// @brief Acesso ao gerenciador de cenas (para uso em onInit/onUpdate).
    SceneManager& scenes() { return *sceneManager; }
    /// @brief Acesso ao sistema de input.
    IInput& input() { return engine->getInputSystem(); }
    /// @brief Acesso ao backend de renderização ativo.
    RendererBackend* renderer() { return rendererBackend; }
    /// @brief Descrição da janela (tamanho, título).
    const WindowDesc& window() const { return winDesc; }

    WindowDesc winDesc;                      ///< Config da janela; project.conf sobrescreve título/tamanho.
    std::string configPath = "project.conf"; ///< Caminho do arquivo de configuração do projeto.

  private:
    bool boot();
    void mainLoop();
    void shutdown();

    /**
     * @brief Overlay de depuração mantido pelo engine (FPS/estatísticas da cena
     *        e alternância de frustum cull).
     *
     * Roda todo frame, independente do onUpdate do projeto. Não faz nada quando
     * a cena ativa não tem um TextRendererComponent.
     */
    void updateDebugOverlay(float deltaTime);

    RendererConfig rendererConfig;
    std::unique_ptr<WindowManager> screenManager;
    std::unique_ptr<SceneManager> sceneManager;
    RendererBackend* rendererBackend = nullptr;

    std::unique_ptr<IInput> inputMan;
    std::unique_ptr<Context> engine;
};

} // namespace Yume

#endif // YUME_APPLICATION_HPP
