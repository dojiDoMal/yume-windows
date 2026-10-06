#ifndef DISPLAY_MANAGER_HPP
#define DISPLAY_MANAGER_HPP

#include "graphics_api.hpp"
#include "renderer/renderer.hpp"
#include "renderer_config.hpp"
#include "window/mml/display_backend.hpp"
#include "window/window_desc.hpp"

class DisplayManager {
  private:
    GraphicsAPI graphicsApi;
    RendererConfig rendererConfig;
    DisplayBackend* displayBackend =
        nullptr; ///< Backend de janela; detido pela MultimediaLayer (não por aqui).
    void* window = nullptr;
    Renderer* renderer = nullptr;

  public:
    ~DisplayManager();

    /**
     * @brief Cria o renderer e a janela principal.
     * @param displayBackend Backend de janela da plataforma (detido pela
     *        MultimediaLayer; o DisplayManager apenas o usa, não o destrói).
     * @param desc Título/dimensões da janela.
     */
    bool init(DisplayBackend& displayBackend, const WindowDesc& desc);
    void render(Scene& scene);
    void* getWindow() { return window; };
    Renderer* getRenderer() { return renderer; }
    void present();
    void setGraphicsApi(const GraphicsAPI& api) { graphicsApi = api; }

    // Presentation config (srgb/vsync). Applied to the backend after it is
    // created but before init(window). setGraphicsApi still selects the API;
    // typically you set graphicsApi = config.api at the call site.
    void setRendererConfig(const RendererConfig& config) { rendererConfig = config; }
};

#endif
