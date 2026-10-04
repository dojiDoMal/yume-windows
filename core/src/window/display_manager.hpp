#ifndef DISPLAY_MANAGER_HPP
#define DISPLAY_MANAGER_HPP

#include "graphics_api.hpp"
#include "renderer/renderer.hpp"
#include "renderer_config.hpp"
#include "window/mml/multimedia_layer.hpp"
#include "window/window_desc.hpp"

class DisplayManager {
  private:
    GraphicsAPI graphicsApi;
    RendererConfig rendererConfig;
    MultimediaLayer* multiMediaLayer = nullptr;
    void* window = nullptr;
    Renderer* renderer = nullptr;

  public:
    ~DisplayManager();

    bool init(const WindowDesc& desc);
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
