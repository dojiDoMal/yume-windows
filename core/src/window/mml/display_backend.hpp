#ifndef DISPLAY_BACKEND_HPP
#define DISPLAY_BACKEND_HPP

#include "graphics_api.hpp"
#include "window/window_desc.hpp"
#include <vector>

/**
 * @brief Backend de janela/apresentação de uma plataforma.
 *
 * É a parte de vídeo da camada de multimídia e, por desenho, o ÚNICO ponto que
 * conversa com a plataforma a respeito da janela. Além de criar/destruir a
 * janela, concentra os serviços dependentes da janela que cada API gráfica
 * precisa: contexto GL e swap, surface Vulkan + extensões de instância, e o
 * handle nativo (HWND) para o D3D12. Os renderer backends consomem esses
 * serviços por tipos opacos (void*), sem nunca ver um SDL_Window — é isso que
 * mantém o renderer agnóstico de SDL.
 *
 * Não é dono do ciclo de vida do subsistema de plataforma (SDL_Init/SDL_Quit) —
 * isso pertence à MultimediaLayer. Cada plataforma/API fornece uma
 * implementação (ex.: SDL2GLDisplayBackend). A MultimediaLayer cria e detém a
 * instância, escolhendo-a pela GraphicsAPI ativa.
 *
 * Os serviços específicos de API têm default inócuo (no-op / retorno nulo), de
 * modo que um backend que não suporta certa API simplesmente não os
 * sobrescreve.
 *
 * @see MultimediaLayer, SDL2DisplayBackend
 */
class DisplayBackend {
  public:
    virtual ~DisplayBackend() = default;

    // --- Janela ------------------------------------------------------------

    /** @brief Ajusta atributos do contexto gráfico ANTES de criar a janela (GL/EGL). No-op para
     * Vulkan/D3D12. */
    virtual void configureContext(const GraphicsAPI& graphicsAPI) = 0;
    /**
     * @brief Flags de janela exigidas pela API deste backend (ex.: a flag de
     *        janela OpenGL/Vulkan da plataforma). OR-adas às flags do chamador
     *        em createMainDisplay. Default 0 (nenhuma flag específica).
     *
     * Antes isso vivia no renderer backend como getRequiredWindowFlags(); como
     * o display backend já é por-API, a flag passou a ser conhecimento dele e o
     * renderer deixou de depender das constantes de janela do SDL.
     */
    virtual unsigned int getApiWindowFlags() const { return 0; }
    /**
     * @brief Cria a janela principal.
     * @param flags Flags da plataforma OR-adas com as exigidas pela API (getApiWindowFlags).
     * @param desc  Título/dimensões da janela.
     * @return Handle opaco da janela (ex.: SDL_Window*), ou nullptr em falha.
     */
    virtual void* createMainDisplay(unsigned int flags, const WindowDesc& desc) = 0;
    /** @brief Destrói a janela criada por createMainDisplay. */
    virtual void destroyDisplay(void* display) = 0;

    // --- Serviços de contexto OpenGL --------------------------------------
    // Dependem da janela, por isso vivem aqui e não no renderer backend.

    /**
     * @brief Cria o contexto GL associado à janela e o torna corrente.
     * @param window Handle opaco da janela.
     * @return Handle opaco do contexto (ex.: SDL_GLContext), ou nullptr em falha.
     */
    virtual void* createGLContext(void* window) { return nullptr; }
    /** @brief Destrói um contexto GL criado por createGLContext. */
    virtual void destroyGLContext(void* context) {}
    /** @brief Define o swap interval (true = vsync/cap à taxa do monitor). */
    virtual void setSwapInterval(bool vsync) {}
    /**
     * @brief Loader de endereços de função GL para o glad/GLEW.
     * @return Ponteiro para a função getProcAddress da plataforma (ex.:
     *         &SDL_GL_GetProcAddress), ou nullptr se não aplicável.
     */
    virtual void* getGLProcAddressLoader() { return nullptr; }
    /** @brief Apresenta o frame (troca os buffers) da janela GL. */
    virtual void swapBuffers(void* window) {}

    // --- Serviços Vulkan ---------------------------------------------------

    /**
     * @brief Cria a surface Vulkan para a janela.
     * @param window     Handle opaco da janela.
     * @param vkInstance VkInstance (passado como void* para não acoplar o header a Vulkan).
     * @param outSurface Recebe a VkSurfaceKHR criada (saída via void*).
     * @return @c true em sucesso.
     */
    virtual bool createVulkanSurface(void* window, void* vkInstance, void* outSurface) {
        return false;
    }
    /** @brief Extensões de instância Vulkan exigidas pela plataforma para apresentar na janela. */
    virtual std::vector<const char*> getVulkanInstanceExtensions() { return {}; }

    // --- Handle nativo (D3D12 e afins) ------------------------------------

    /**
     * @brief Handle nativo da janela da plataforma.
     * @return HWND (no Windows) como void*, ou nullptr se indisponível.
     */
    virtual void* getNativeWindowHandle(void* window) { return nullptr; }
};

#endif // DISPLAY_BACKEND_HPP
