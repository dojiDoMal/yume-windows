#ifndef VULKAN_SHADER_COMPILER_HPP
#define VULKAN_SHADER_COMPILER_HPP

#include "../../../shader_compiler.hpp"
#include <vulkan/vulkan.h>

class VulkanRendererBackend;

class VulkanShaderCompiler : public ShaderCompiler {
private:
    VulkanRendererBackend* backend;
    
public:
    VulkanShaderCompiler(VulkanRendererBackend* backend) : backend(backend) {}
    ~VulkanShaderCompiler() = default;
    
    bool compile(const std::string& source, ShaderType type, void** outHandle) override;
    void destroy(void* handle) override;
    bool isValid(void* handle) override;
};

#endif // VULKAN_SHADER_COMPILER_HPP
