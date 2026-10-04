#ifndef SDL2_LAYER_HPP
#define SDL2_LAYER_HPP

#include "window/mml/multimedia_layer.hpp"
#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <vector>

class SDL2Layer : public MultimediaLayer {
  private:

  public:
    ~SDL2Layer();

    void configureContext(const GraphicsAPI& graphicsAPI);
};

#endif
