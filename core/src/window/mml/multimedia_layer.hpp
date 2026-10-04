#ifndef MULTIMEDIA_LAYER_HPP
#define MULTIMEDIA_LAYER_HPP

#include "graphics_api.hpp"
#include "window/window_desc.hpp"

class MultimediaLayer {
  public:
    virtual ~MultimediaLayer() = default;

    virtual bool init() = 0;
    virtual void configureContext(const GraphicsAPI& graphicsAPI) = 0;
    virtual void* createMainDisplay(unsigned int flags, const WindowDesc& desc) = 0;
    virtual void destroyDisplay(void* display) = 0;
    virtual void end() = 0;
};

#endif // MULTIMEDIA_LAYER_HPP