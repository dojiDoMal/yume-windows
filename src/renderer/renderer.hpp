#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "../graphics_api.hpp"
#include "../scene.hpp"
#include "renderer_backend.hpp"

class Material;

class Renderer {
  private:
    RendererBackend* backend = nullptr;

  public:
    ~Renderer();
    void setRendererBackend(RendererBackend* backend);
    RendererBackend* getRendererBackend();
    bool initBackend(const GraphicsAPI& graphicsApi);
    bool initWindow(SDL_Window* win);
    void render(const Scene& scene);
    void present(SDL_Window* window);
};

#endif
