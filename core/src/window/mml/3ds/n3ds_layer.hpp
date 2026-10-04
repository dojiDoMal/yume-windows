#ifndef N3DS_LAYER_HPP
#define N3DS_LAYER_HPP

#include "graphics_api.hpp"
#include "window/mml/multimedia_layer.hpp"
#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <vector>

class N3DSLayer : public MultimediaLayer {
  private:

  public:
    ~N3DSLayer();

    void configureContext(const GraphicsAPI& graphicsAPI);
};

#endif // N3DS_LAYER_HPP
