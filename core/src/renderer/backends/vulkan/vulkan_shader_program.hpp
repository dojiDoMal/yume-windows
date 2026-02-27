#ifndef VULKAN_SHADER_PROGRAM_HPP
#define VULKAN_SHADER_PROGRAM_HPP

#include "../../../shader_program.hpp"
#include "../../../shader_type.hpp"
#include "material.hpp"
#include <vulkan/vulkan.h>
#include <vector>

class VulkanRendererBackend;

class VulkanShaderProgram : public ShaderProgram {
private:
    VulkanRendererBackend* backend;
    std::vector<VkShaderModule> shaderModules;
    std::vector<ShaderType> shaderTypes;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    
    bool createPipeline();
    
public:
    VulkanShaderProgram(VulkanRendererBackend* backend) : backend(backend) {}
    ~VulkanShaderProgram();
    
    bool attachShader(const ShaderAsset& shader) override;
    bool link() override;
    void use() override;
    void setUniformBuffer(const char* name, const void* data, size_t size) override;
    void* getHandle() const override;
    bool isValid() const override;
    
    VkPipeline getPipeline() const { return pipeline; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
};

#endif // VULKAN_SHADER_PROGRAM_HPP
