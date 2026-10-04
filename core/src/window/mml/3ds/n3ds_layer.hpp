#ifndef N3DS_LAYER_HPP
#define N3DS_LAYER_HPP

#include "graphics_api.hpp"
#include "window/mml/multimedia_layer.hpp"

class N3DSLayer : public MultimediaLayer {
  private:
  public:
    ~N3DSLayer() = default;

    bool init() override;
    void configureContext(const GraphicsAPI& graphicsAPI) override;
    void* createMainDisplay(unsigned int flags, const WindowDesc& opts) override;
    void destroyDisplay(void* display) override;
    void end() override;
};

#endif // N3DS_LAYER_HPP
