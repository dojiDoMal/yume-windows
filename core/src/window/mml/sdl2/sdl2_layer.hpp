#ifndef SDL2_LAYER_HPP
#define SDL2_LAYER_HPP

#include "window/mml/multimedia_layer.hpp"
#include <GL/glew.h>
#include <string>

class SDL2Layer : public MultimediaLayer {
  private:
  public:
    ~SDL2Layer() = default;

    bool init() override;
    void configureContext(const GraphicsAPI& graphicsAPI) override;
    void* createMainDisplay(unsigned int flags, const WindowDesc& opts) override;
    void destroyDisplay(void* display) override;
    void end() override;
};

#endif
