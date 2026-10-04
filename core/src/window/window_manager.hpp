#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include "graphics_api.hpp"
#include "renderer/renderer.hpp"
#include "renderer_config.hpp"
#include "window/window_desc.hpp"
#include "window/multimedia_layer.hpp"
#include <SDL2/SDL.h>

// TODO: talvez precise mudaar o nome dessa classe pra algo como rendertargetmanaager ou displaymanager
class WindowManager {
  private:
    GraphicsAPI graphicsApi;
    RendererConfig rendererConfig;
    MultimediaLayer* multiMediaLayer = nullptr;
    SDL_Window* window = nullptr;
    Renderer* renderer = nullptr;

  public:
    ~WindowManager();

    bool init(const WindowDesc& desc);
    void render(Scene& scene);
    SDL_Window* getWindow() { return window; };
    Renderer* getRenderer() { return renderer; }
    void present();
    void setGraphicsApi(const GraphicsAPI& api) { graphicsApi = api; }

    // Presentation config (srgb/vsync). Applied to the backend after it is
    // created but before init(window). setGraphicsApi still selects the API;
    // typically you set graphicsApi = config.api at the call site.
    void setRendererConfig(const RendererConfig& config) { rendererConfig = config; }
};

#endif