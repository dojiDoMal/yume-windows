#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "graphics_api.hpp"
#include "renderer/renderer_backend.hpp"
#include "scene/scene.hpp"

class Material;
class DisplayBackend;

class Renderer {
  private:
    RendererBackend* backend = nullptr;

  public:
    ~Renderer();
    void setRendererBackend(RendererBackend* backend);
    RendererBackend* getRendererBackend();
    bool initBackend(const GraphicsAPI& graphicsApi);
    bool initWindow(void* window, DisplayBackend& display);
    void render(const Scene& scene);
    void present(void* window);
};

#endif
