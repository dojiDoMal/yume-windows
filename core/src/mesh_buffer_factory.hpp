#ifndef MESH_BUFFER_FACTORY_HPP
#define MESH_BUFFER_FACTORY_HPP

#include "graphics_api.hpp"
#include "mesh_buffer.hpp"
#include <memory>

class MeshBufferFactory {
  public:
    static std::unique_ptr<MeshBuffer> create(GraphicsAPI api, void* context = nullptr);
};

#endif // MESHBUFFERFACTORY_HPP
