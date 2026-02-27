#include "vulkan_shader_program.hpp"
#include "vulkan_renderer_backend.hpp"
#include "../../../shader_asset.hpp"
#include <cstring>
#include <array>

VulkanShaderProgram::~VulkanShaderProgram() {
    if (pipeline) {
        vkDestroyPipeline(backend->getDevice(), pipeline, nullptr);
    }
    if (pipelineLayout) {
        vkDestroyPipelineLayout(backend->getDevice(), pipelineLayout, nullptr);
    }
}

bool VulkanShaderProgram::attachShader(const ShaderAsset& shader) {
    VkShaderModule module = *static_cast<VkShaderModule*>(shader.getHandle());
    shaderModules.push_back(module);
    shaderTypes.push_back(shader.getType());
    return true;
}

bool VulkanShaderProgram::link() {
    return createPipeline();
}

bool VulkanShaderProgram::createPipeline() {
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    
    for (size_t i = 0; i < shaderModules.size(); i++) {
        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.module = shaderModules[i];
        stageInfo.pName = "main";
        
        if (shaderTypes[i] == ShaderType::VERTEX) {
            stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        } else if (shaderTypes[i] == ShaderType::FRAGMENT) {
            stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        
        shaderStages.push_back(stageInfo);
    }
    
    VkVertexInputBindingDescription bindingDescriptions[2] = {};
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = 3 * sizeof(float);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    bindingDescriptions[1].binding = 1;
    bindingDescriptions[1].stride = 3 * sizeof(float);
    bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attributeDescriptions[2] = {};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0;
    
    attributeDescriptions[1].binding = 1;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = 0;
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 2;
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    auto extent = backend->getSwapchainExtent();
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    auto descriptorSetLayout = backend->getDescriptorSetLayout();
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    
    if (vkCreatePipelineLayout(backend->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        return false;
    }
    
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = backend->getRenderPass();
    pipelineInfo.subpass = 0;
    
    return vkCreateGraphicsPipelines(backend->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) == VK_SUCCESS;
}

void VulkanShaderProgram::use() {
    // Em Vulkan, "use" é feito via vkCmdBindPipeline no command buffer
}

void VulkanShaderProgram::setUniformBuffer(const char* name, const void* data, size_t size) {

    //temporary fix
    int binding = 0;

    if (binding == 1 && backend) {
        void* mapped;
        vkMapMemory(backend->getDevice(), backend->getMaterialBufferMemory(), 0, size, 0, &mapped);
        memcpy(mapped, data, size);
        vkUnmapMemory(backend->getDevice(), backend->getMaterialBufferMemory());
    } else if (binding == 2 && backend) {
        void* mapped;
        vkMapMemory(backend->getDevice(), backend->getLightDataBufferMemory(), 0, size, 0, &mapped);
        memcpy(mapped, data, size);
        vkUnmapMemory(backend->getDevice(), backend->getLightDataBufferMemory());
    }
}

void* VulkanShaderProgram::getHandle() const {
    return (void*)pipeline;
}

bool VulkanShaderProgram::isValid() const {
    return pipeline != VK_NULL_HANDLE;
}
