#ifndef MULTIMEDIA_LAYER_HPP
#define MULTIMEDIA_LAYER_HPP

#include "graphics_api.hpp"

class MultimediaLayer {
  public:
    ~MultimediaLayer();

    virtual void configureContext(const GraphicsAPI& graphicsAPI) = 0;
};

#endif // MULTIMEDIA_LAYER_HPP