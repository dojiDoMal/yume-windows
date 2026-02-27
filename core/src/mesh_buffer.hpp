#ifndef MESH_BUFFER_HPP
#define MESH_BUFFER_HPP

#include <vector>

class MeshBuffer {
  public:
    virtual ~MeshBuffer() = default;
    virtual bool createBuffers(const std::vector<float>& vertices,
                               const std::vector<float>& normals) = 0;
    virtual void bind() = 0;
    virtual void unbind() = 0;
    virtual void destroy() = 0;
    virtual void* getHandle() const = 0;
};

#endif // MESHBUFFER_HPP
