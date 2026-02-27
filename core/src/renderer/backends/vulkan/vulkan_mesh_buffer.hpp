#ifndef VULKAN_MESH_BUFFER_HPP
#define VULKAN_MESH_BUFFER_HPP

#include "mesh_buffer.hpp"
#include <vulkan/vulkan.h>

class VulkanRendererBackend;

class VulkanMeshBuffer : public MeshBuffer {
private:
    VulkanRendererBackend* backend;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer normalBuffer = VK_NULL_HANDLE;
    VkDeviceMemory normalBufferMemory = VK_NULL_HANDLE;
    
    bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                     VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    
public:
    VulkanMeshBuffer(VulkanRendererBackend* backend) : backend(backend) {}
    ~VulkanMeshBuffer();
    
    bool createBuffers(const std::vector<float>& vertices, const std::vector<float>& normals) override;
    void bind() override;
    void unbind() override;
    void destroy() override;
    void* getHandle() const override;
    
    VkBuffer getVertexBuffer() const { return vertexBuffer; }
    VkBuffer getNormalBuffer() const { return normalBuffer; }
};

#endif // VULKAN_MESH_BUFFER_HPP
