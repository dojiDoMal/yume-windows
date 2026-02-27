#include "vulkan_shader_compiler.hpp"
#include "vulkan_renderer_backend.hpp"
#include <fstream>
#include <vector>

bool VulkanShaderCompiler::compile(const std::string& source, ShaderType type, void** outHandle) {
    // Vulkan usa SPIR-V diretamente - carregar arquivo .spv
    std::ifstream file(source, std::ios::ate | std::ios::binary);
    if (!file.is_open()) return false;
    
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
    
    VkShaderModule* shaderModule = new VkShaderModule();
    if (vkCreateShaderModule(backend->getDevice(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
        delete shaderModule;
        return false;
    }
    
    *outHandle = shaderModule;
    return true;
}

void VulkanShaderCompiler::destroy(void* handle) {
    if (handle) {
        VkShaderModule* module = static_cast<VkShaderModule*>(handle);
        vkDestroyShaderModule(backend->getDevice(), *module, nullptr);
        delete module;
    }
}

bool VulkanShaderCompiler::isValid(void* handle) {
    return handle != nullptr;
}
