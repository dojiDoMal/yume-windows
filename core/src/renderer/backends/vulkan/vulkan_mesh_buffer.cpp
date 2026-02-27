#define CLASS_NAME "VulkanMeshBuffer"
#include "vulkan_mesh_buffer.hpp"
#include "renderer/backends/vulkan/vulkan_renderer_backend.hpp"
#include <cstring>
#include "log_macros.hpp"

VulkanMeshBuffer::~VulkanMeshBuffer() {
    destroy();
}

bool VulkanMeshBuffer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                   VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    
    VkDevice dev = backend->getDevice();
    if (dev == VK_NULL_HANDLE) {
        LOG_ERROR("Device is NULL!");
    }
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(backend->getDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        return false;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(backend->getDevice(), buffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(backend->getPhysicalDevice(), &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            allocInfo.memoryTypeIndex = i;
            break;
        }
    }
    
    if (vkAllocateMemory(backend->getDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        return false;
    }
    
    vkBindBufferMemory(backend->getDevice(), buffer, bufferMemory, 0);
    return true;
}

bool VulkanMeshBuffer::createBuffers(const std::vector<float>& vertices, const std::vector<float>& normals) {
    VkDeviceSize vertexBufferSize = sizeof(float) * vertices.size();
    VkDeviceSize normalBufferSize = sizeof(float) * normals.size();
    
    if (!createBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     vertexBuffer, vertexBufferMemory)) {
        return false;
    }
    
    void* data;
    vkMapMemory(backend->getDevice(), vertexBufferMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, vertices.data(), vertexBufferSize);
    vkUnmapMemory(backend->getDevice(), vertexBufferMemory);
    
    if (!normals.empty()) {
        if (!createBuffer(normalBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         normalBuffer, normalBufferMemory)) {
            return false;
        }
        
        vkMapMemory(backend->getDevice(), normalBufferMemory, 0, normalBufferSize, 0, &data);
        memcpy(data, normals.data(), normalBufferSize);
        vkUnmapMemory(backend->getDevice(), normalBufferMemory);
    }
    
    return true;
}

void VulkanMeshBuffer::bind() {
    // Binding é feito no command buffer, não aqui
}

void VulkanMeshBuffer::unbind() {
    // Não necessário em Vulkan
}

void VulkanMeshBuffer::destroy() {
    if (vertexBuffer) {
        vkDestroyBuffer(backend->getDevice(), vertexBuffer, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
    }
    if (vertexBufferMemory) {
        vkFreeMemory(backend->getDevice(), vertexBufferMemory, nullptr);
        vertexBufferMemory = VK_NULL_HANDLE;
    }
    if (normalBuffer) {
        vkDestroyBuffer(backend->getDevice(), normalBuffer, nullptr);
        normalBuffer = VK_NULL_HANDLE;
    }
    if (normalBufferMemory) {
        vkFreeMemory(backend->getDevice(), normalBufferMemory, nullptr);
        normalBufferMemory = VK_NULL_HANDLE;
    }
}

void* VulkanMeshBuffer::getHandle() const {
    return (void*)vertexBuffer;
}
