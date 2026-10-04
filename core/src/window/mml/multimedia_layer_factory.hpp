#ifndef MULTIMEDIA_LAYER_FACTORY_HPP
#define MULTIMEDIA_LAYER_FACTORY_HPP

#include "graphics_api.hpp"
#include "multimedia_layer.hpp"

class MultimediaLayerFactory {
public:
    static MultimediaLayer* create(const GraphicsAPI& api);
};

#endif // MULTIMEDIA_LAYER_FACTORY_HPP