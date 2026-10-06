#include "multimedia_layer_factory.hpp"

#ifdef __3DS__
#include "3ds/n3ds_multimedia_layer.hpp"
#else
#include "sdl2/sdl2_multimedia_layer.hpp"
#endif // __3DS__

MultimediaLayer* MultimediaLayerFactory::create(const GraphicsAPI& api) {
    switch (api) {
    case GraphicsAPI::CITRO3D:
#ifdef __3DS__
        return new N3DSMultimediaLayer();
#else
        return nullptr;
#endif // __3DS__
    case GraphicsAPI::WEBGL:
    case GraphicsAPI::OPENGL:
    case GraphicsAPI::VULKAN:
    case GraphicsAPI::DIRECTX12:
#ifdef __3DS__
        return nullptr;
#else
        return new SDL2MultimediaLayer(api);
#endif // __3DS__
    default:
        return nullptr;
    }
}
