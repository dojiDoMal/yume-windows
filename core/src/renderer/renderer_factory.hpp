#ifndef RENDERER_FACTORY_HPP
#define RENDERER_FACTORY_HPP

#include "renderer/renderer_backend.hpp"
#include "graphics_api.hpp"

class RendererFactory {
public:
    static RendererBackend* create(const GraphicsAPI& api);
};

#endif