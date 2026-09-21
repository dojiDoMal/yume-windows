#include "mesh_buffer_factory.hpp"
#include "shader_compiler_factory.hpp"
#define CLASS_NAME "VulkanRendererBackend"
#include "../../../log_macros.hpp"

#include "../../../color.hpp"
#include "../../../components/mesh_renderer.hpp"
#include "../../../font_atlas.hpp"
#include "../../../material.hpp"
#include "../../../stb_image.h"
#include "shader_program_factory.hpp"
#include "vulkan_mesh_buffer.hpp"
#include "vulkan_renderer_backend.hpp"
#include "vulkan_shader_program.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL_video.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <set>
#include <sstream>

GraphicsAPI VulkanRendererBackend::getGraphicsAPI() const { return GraphicsAPI::VULKAN; }

std::string VulkanRendererBackend::getShaderExtension() const { return ".spv"; }

std::unique_ptr<ShaderProgram> VulkanRendererBackend::createShaderProgram() {
    return ShaderProgramFactory::create(getGraphicsAPI(), this);
}

std::unique_ptr<MeshBuffer> VulkanRendererBackend::createMeshBuffer() {
    return MeshBufferFactory::create(getGraphicsAPI(), this);
}

std::unique_ptr<ShaderCompiler> VulkanRendererBackend::createShaderCompiler() {
    return ShaderCompilerFactory::create(getGraphicsAPI(), this);
}

VulkanRendererBackend::~VulkanRendererBackend() {
    if (device) {
        vkDeviceWaitIdle(device);

        if (inFlightFence)
            vkDestroyFence(device, inFlightFence, nullptr);
        if (renderFinishedSemaphore)
            vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        if (imageAvailableSemaphore)
            vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

        if (commandPool)
            vkDestroyCommandPool(device, commandPool, nullptr);

        for (auto framebuffer : framebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        if (renderPass)
            vkDestroyRenderPass(device, renderPass, nullptr);

        for (auto imageView : swapchainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        if (depthImageView)
            vkDestroyImageView(device, depthImageView, nullptr);
        if (depthImage)
            vkDestroyImage(device, depthImage, nullptr);
        if (depthImageMemory)
            vkFreeMemory(device, depthImageMemory, nullptr);

        if (uniformBuffer)
            vkDestroyBuffer(device, uniformBuffer, nullptr);
        if (uniformBufferMemory)
            vkFreeMemory(device, uniformBufferMemory, nullptr);
        if (materialBuffer)
            vkDestroyBuffer(device, materialBuffer, nullptr);
        if (materialBufferMemory)
            vkFreeMemory(device, materialBufferMemory, nullptr);
        if (lightDataBuffer)
            vkDestroyBuffer(device, lightDataBuffer, nullptr);
        if (lightDataBufferMemory)
            vkFreeMemory(device, lightDataBufferMemory, nullptr);

        if (instanceBuffer) {
            if (instanceBufferMapped)
                vkUnmapMemory(device, instanceBufferMemory);
            vkDestroyBuffer(device, instanceBuffer, nullptr);
        }
        if (instanceBufferMemory)
            vkFreeMemory(device, instanceBufferMemory, nullptr);

        // Text resources.
        if (textVertexBuffer) {
            if (textVertexMapped)
                vkUnmapMemory(device, textVertexMemory);
            vkDestroyBuffer(device, textVertexBuffer, nullptr);
        }
        if (textVertexMemory)
            vkFreeMemory(device, textVertexMemory, nullptr);
        if (textCB) {
            if (textCBMapped)
                vkUnmapMemory(device, textCBMemory);
            vkDestroyBuffer(device, textCB, nullptr);
        }
        if (textCBMemory)
            vkFreeMemory(device, textCBMemory, nullptr);
        if (textPipeline)
            vkDestroyPipeline(device, textPipeline, nullptr);
        if (textPipelineLayout)
            vkDestroyPipelineLayout(device, textPipelineLayout, nullptr);
        if (textDescriptorPool)
            vkDestroyDescriptorPool(device, textDescriptorPool, nullptr);
        if (textDescriptorSetLayout)
            vkDestroyDescriptorSetLayout(device, textDescriptorSetLayout, nullptr);

        // Textures + sampler.
        if (textureSampler)
            vkDestroySampler(device, textureSampler, nullptr);
        for (auto& tex : textures) {
            if (tex.view)
                vkDestroyImageView(device, tex.view, nullptr);
            if (tex.image)
                vkDestroyImage(device, tex.image, nullptr);
            if (tex.memory)
                vkFreeMemory(device, tex.memory, nullptr);
        }

        if (descriptorPool)
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        if (descriptorSetLayout)
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        if (swapchain)
            vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroyDevice(device, nullptr);
    }

    if (surface)
        vkDestroySurfaceKHR(instance, surface, nullptr);
    if (instance)
        vkDestroyInstance(instance, nullptr);
}

unsigned int VulkanRendererBackend::getRequiredWindowFlags() const { return SDL_WINDOW_VULKAN; };

bool VulkanRendererBackend::init(SDL_Window* win) {
    // Full Vulkan bring-up, driven from the window init (mirrors how the OpenGL
    // backend creates its context inside init(window)). Order matters:
    // instance -> surface -> device/swapchain/... (the rest lives in init()).
    if (!win) {
        LOG_ERROR("Window is null!");
        return false;
    }
    setWindow(win);

    if (!createInstance()) {
        LOG_ERROR("Failed to create Vulkan instance");
        return false;
    }

    VkSurfaceKHR surf = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(win, instance, &surf)) {
        LOG_ERROR(std::string("SDL_Vulkan_CreateSurface failed: ") + SDL_GetError());
        return false;
    }
    setSurface(surf);

    return init();
}

bool VulkanRendererBackend::initWindowContext() {
    // Instance creation happens in init(window) now (it needs to precede
    // surface creation). Kept as a no-op so the window flags path still works.
    return true;
}

bool VulkanRendererBackend::init() {
    printf("[Vulkan] init - starting full initialization\n");
    if (!pickPhysicalDevice()) {
        printf("Failed to pick physical device\n");
        return false;
    }
    if (!createLogicalDevice()) {
        printf("Failed to create logical device\n");
        return false;
    }
    if (!createSwapchain()) {
        printf("Failed to create swapchain\n");
        return false;
    }
    if (!createImageViews()) {
        printf("Failed to create image views\n");
        return false;
    }
    if (!createRenderPass()) {
        printf("Failed to create render pass\n");
        return false;
    }
    if (!createDepthResources()) {
        printf("Failed to create depth resources\n");
        return false;
    }
    if (!createFramebuffers()) {
        printf("Failed to create framebuffers\n");
        return false;
    }
    if (!createCommandPool()) {
        printf("Failed to create command pool\n");
        return false;
    }
    if (!createDescriptorSetLayout()) {
        printf("Failed to create descriptor set layout\n");
        return false;
    }
    if (!createUniformBuffer()) {
        printf("Failed to create uniform buffer\n");
        return false;
    }
    if (!createMaterialBuffer()) {
        printf("Failed to create material buffer\n");
        return false;
    }
    if (!createLightDataBuffer()) {
        printf("Failed to create light data buffer\n");
        return false;
    }
    if (!createInstanceBuffer()) {
        printf("Failed to create instance buffer\n");
        return false;
    }
    if (!createDescriptorPool()) {
        printf("Failed to create descriptor pool\n");
        return false;
    }
    if (!createCommandBuffers()) {
        printf("Failed to create command buffers\n");
        return false;
    }
    if (!createSyncObjects()) {
        printf("Failed to create sync objects\n");
        return false;
    }

    printf("[Vulkan] Initialization complete!\n");
    return true;
}

bool VulkanRendererBackend::createInstance() {
    printf("[Vulkan] Creating instance...\n");
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Renderer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Custom";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // SDL extensions
    unsigned int extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr);
    std::vector<const char*> extensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, extensions.data());

    printf("[Vulkan] Required extensions: %u\n", extensionCount);

    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 0;

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    printf("[Vulkan] Instance creation result: %d\n", result);
    return result == VK_SUCCESS;
}

bool VulkanRendererBackend::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        LOG_WARN("No physical device found!");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    physicalDevice = devices[0]; // Simplificado: pega o primeiro
    return true;
}

bool VulkanRendererBackend::createLogicalDevice() {
    // Encontrar queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    LOG_INFO("Queue family count: " + std::to_string(queueFamilyCount));

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                             queueFamilies.data());

    // Prefer a queue family that supports BOTH graphics and present on our
    // surface (the common case). Falls back to the first graphics family.
    bool foundGraphicsQueue = false;
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (!(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            continue;

        VkBool32 presentSupport = VK_FALSE;
        if (surface != VK_NULL_HANDLE)
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

        graphicsQueueFamily = i;
        presentQueueFamily = i;
        foundGraphicsQueue = true;
        LOG_INFO("Found graphics queue family at index: " + std::to_string(i) +
                 (presentSupport ? " (present supported)" : " (present unknown)"));
        if (presentSupport)
            break; // ideal: graphics + present in the same family
    }

    if (!foundGraphicsQueue) {
        LOG_ERROR("No graphics queue family found!");
        return false;
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsQueueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;

    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create logical device! Error code: " + std::to_string(result));
        return false;
    }

    std::ostringstream oss;
    oss << "[Vulkan] Logical device created successfully, handle: " << device;
    LOG_INFO(oss.str());

    vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentQueueFamily, 0, &presentQueue);

    return true;
}

bool VulkanRendererBackend::createSwapchain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

    // Pick the swapchain format according to the project's srgb setting.
    //  - srgb=false (default): a UNORM format, matching OpenGL/D3D12 which write
    //    colors without a linear->sRGB conversion.
    //  - srgb=true: an _SRGB format, so the GPU applies linear->sRGB on write.
    const VkFormat preferredBgra = srgbEnabled ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM;
    const VkFormat preferredRgba = srgbEnabled ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    bool picked = false;
    for (const auto& format : formats) {
        if ((format.format == preferredBgra || format.format == preferredRgba) &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = format;
            picked = true;
            break;
        }
    }
    // Fallback: any format matching the desired sRGB-ness (sRGB vs non-sRGB).
    if (!picked) {
        for (const auto& format : formats) {
            bool isSrgb = (format.format == VK_FORMAT_B8G8R8A8_SRGB ||
                           format.format == VK_FORMAT_R8G8B8A8_SRGB);
            if (isSrgb == srgbEnabled) {
                surfaceFormat = format;
                break;
            }
        }
    }

    swapchainFormat = surfaceFormat.format;
    swapchainExtent = capabilities.currentExtent;

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // Vsync from project.conf. FIFO (always supported) caps to refresh rate;
    // IMMEDIATE presents as fast as possible (may tear). If IMMEDIATE isn't
    // supported we keep FIFO.
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!vsyncEnabled) {
        uint32_t modeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modeCount, nullptr);
        std::vector<VkPresentModeKHR> modes(modeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modeCount,
                                                  modes.data());
        for (auto m : modes) {
            if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                break;
            }
        }
    }
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        return false;
    }

    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

    return true;
}

bool VulkanRendererBackend::createImageViews() {
    swapchainImageViews.resize(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapchainImageViews[i]) !=
            VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool VulkanRendererBackend::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    return vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) == VK_SUCCESS;
}

bool VulkanRendererBackend::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding bindings[4] = {};

    // binding 0: Matrices UBO (vertex)
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // binding 1: MaterialData UBO (fragment)
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 2: LightData UBO (fragment)
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 3: per-instance model matrices (storage buffer). Dynamic offset
    // so each instanced draw can point at its own slice of the per-frame arena.
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 4;
    layoutInfo.pBindings = bindings;

    return vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) ==
           VK_SUCCESS;
}

bool VulkanRendererBackend::createFramebuffers() {
    framebuffers.resize(swapchainImageViews.size());

    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {swapchainImageViews[i], depthImageView};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) !=
            VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool VulkanRendererBackend::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    return vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) == VK_SUCCESS;
}

bool VulkanRendererBackend::createDepthResources() {
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapchainExtent.width;
    imageInfo.extent.height = swapchainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &depthImage) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, depthImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &depthImageMemory) != VK_SUCCESS) {
        return false;
    }

    vkBindImageMemory(device, depthImage, depthImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    return vkCreateImageView(device, &viewInfo, nullptr, &depthImageView) == VK_SUCCESS;
}

bool VulkanRendererBackend::createUniformBuffer() {
    if (device == VK_NULL_HANDLE) {
        LOG_WARN("Device is null in createUniformBuffer\n");
        return false;
    }

    VkDeviceSize bufferSize = 4 * sizeof(glm::mat4);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffer) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, uniformBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &uniformBufferMemory) != VK_SUCCESS) {
        return false;
    }

    vkBindBufferMemory(device, uniformBuffer, uniformBufferMemory, 0);
    return true;
}

bool VulkanRendererBackend::createMaterialBuffer() {
    // Sized generously (like the OpenGL UBO): the shader only reads a vec4, but
    // keeping headroom avoids any host-write overflow.
    VkDeviceSize bufferSize = 256;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &materialBuffer) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, materialBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &materialBufferMemory) != VK_SUCCESS) {
        return false;
    }

    vkBindBufferMemory(device, materialBuffer, materialBufferMemory, 0);
    return true;
}

bool VulkanRendererBackend::createLightDataBuffer() {
    // The shader reads only vec3 lightDirection, but Material::applyLight writes
    // a larger struct (direction + padding + color + intensity). Size to 256 to
    // safely hold it (matches the OpenGL LightData UBO).
    VkDeviceSize bufferSize = 256;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &lightDataBuffer) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, lightDataBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &lightDataBufferMemory) != VK_SUCCESS) {
        return false;
    }

    vkBindBufferMemory(device, lightDataBuffer, lightDataBufferMemory, 0);
    return true;
}

bool VulkanRendererBackend::createDescriptorPool() {
    VkDescriptorPoolSize poolSizes[2] = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 3;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    poolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        return false;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    descriptorSets.resize(1);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        return false;
    }

    VkDescriptorBufferInfo bufferInfos[4] = {};
    bufferInfos[0].buffer = uniformBuffer;
    bufferInfos[0].offset = 0;
    bufferInfos[0].range = 4 * sizeof(glm::mat4);

    bufferInfos[1].buffer = materialBuffer;
    bufferInfos[1].offset = 0;
    bufferInfos[1].range = 256;

    bufferInfos[2].buffer = lightDataBuffer;
    bufferInfos[2].offset = 0;
    bufferInfos[2].range = 256;

    // Dynamic storage buffer. With a dynamic offset per draw, VK_WHOLE_SIZE
    // means "from the dynamic offset to the end of the buffer", which is exactly
    // what an unbounded StructuredBuffer (mat4 _m0[]) wants. Using a fixed range
    // instead would overflow the buffer once dynamicOffset > 0.
    bufferInfos[3].buffer = instanceBuffer;
    bufferInfos[3].offset = 0;
    bufferInfos[3].range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptorWrites[4] = {};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSets[0];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfos[0];

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSets[0];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &bufferInfos[1];

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = descriptorSets[0];
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &bufferInfos[2];

    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = descriptorSets[0];
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pBufferInfo = &bufferInfos[3];

    vkUpdateDescriptorSets(device, 4, descriptorWrites, 0, nullptr);

    return true;
}

bool VulkanRendererBackend::createInstanceBuffer() {
    // Query the required alignment for dynamic storage buffer offsets.
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    instanceAlignment = props.limits.minStorageBufferOffsetAlignment;
    if (instanceAlignment == 0)
        instanceAlignment = sizeof(glm::mat4);

    instanceBufferCapacity = 1024; // matrices; grows if needed

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = instanceBufferCapacity * sizeof(glm::mat4);
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &instanceBuffer) != VK_SUCCESS)
        return false;

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, instanceBuffer, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memReq.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &instanceBufferMemory) != VK_SUCCESS)
        return false;

    vkBindBufferMemory(device, instanceBuffer, instanceBufferMemory, 0);
    vkMapMemory(device, instanceBufferMemory, 0, bufferInfo.size, 0, &instanceBufferMapped);
    return true;
}

void VulkanRendererBackend::beginInstanceFrame() { instanceBufferCursor = 0; }

bool VulkanRendererBackend::appendInstanceData(const glm::mat4* models, size_t count,
                                               uint32_t& outOffset) {
    if (count == 0)
        return false;

    // Align the cursor's byte offset to the device's dynamic-offset requirement.
    size_t byteOffset = instanceBufferCursor * sizeof(glm::mat4);
    size_t alignedByte = (byteOffset + instanceAlignment - 1) & ~(instanceAlignment - 1);
    size_t alignedSlot = alignedByte / sizeof(glm::mat4);

    if (alignedSlot + count > instanceBufferCapacity) {
        LOG_WARN("Instance buffer overflow this frame");
        return false;
    }

    auto* dst = static_cast<glm::mat4*>(instanceBufferMapped) + alignedSlot;
    memcpy(dst, models, count * sizeof(glm::mat4));
    instanceBufferCursor = alignedSlot + count;
    outOffset = static_cast<uint32_t>(alignedByte);
    return true;
}

bool VulkanRendererBackend::createCommandBuffers() {
    commandBuffers.resize(framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    return vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) == VK_SUCCESS;
}

bool VulkanRendererBackend::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    return vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) ==
               VK_SUCCESS &&
           vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) ==
               VK_SUCCESS &&
           vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) == VK_SUCCESS;
}

uint32_t VulkanRendererBackend::findMemoryType(uint32_t typeFilter,
                                               VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}

void VulkanRendererBackend::onCameraSet() {
    // Atualizar clear color se necessário
}

void VulkanRendererBackend::clear(Camera* camera) {
    // New frame: reset the text per-frame arenas (vertex + constant buffers).
    beginTextFrame();

    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE,
                          &currentImageIndex);

    vkResetCommandBuffer(commandBuffers[currentImageIndex], 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(commandBuffers[currentImageIndex], &beginInfo);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[currentImageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent;

    // Use the camera passed into clear() (mirrors the OpenGL backend). mainCamera
    // is only populated via setCamera(), which the render path doesn't call, so
    // relying on it here left the clear color black.
    std::array<VkClearValue, 2> clearValues{};
    ColorRGBA bgColor = camera ? camera->getBackgroundColor() : COLOR::BLACK;
    clearValues[0].color = {{bgColor.r, bgColor.g, bgColor.b, bgColor.a}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[currentImageIndex], &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRendererBackend::draw(const Mesh& mesh) {
    auto* vkMeshBuffer = static_cast<VulkanMeshBuffer*>(mesh.getMeshBuffer());
    VkBuffer vertexBuffers[] = {vkMeshBuffer->getVertexBuffer(), vkMeshBuffer->getNormalBuffer()};
    VkDeviceSize offsets[] = {0, 0};
    vkCmdBindVertexBuffers(commandBuffers[currentImageIndex], 0, 2, vertexBuffers, offsets);
    vkCmdDraw(commandBuffers[currentImageIndex], mesh.getVertices().size() / 3, 1, 0, 0);
}

void VulkanRendererBackend::setUniforms(ShaderProgram* shaderProgram) {
    // Bind only the pipeline here. The descriptor set carries a dynamic offset
    // for the per-instance storage buffer, so it must be bound per-draw (with
    // that offset) in renderWorldObjects, not here.
    if (shaderProgram && shaderProgram->isValid()) {
        VkPipeline pipeline = static_cast<VkPipeline>(shaderProgram->getHandle());
        vkCmdBindPipeline(commandBuffers[currentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline);
    }
}

void VulkanRendererBackend::bindCamera(Camera* camera) {
    if (!camera)
        return;
    WorldObject* cameraObj = camera->getOwner();
    if (!cameraObj)
        return;

    // Identical camera convention to the OpenGL backend (right-handed, -Z
    // forward). The one Vulkan-specific fix is flipping the projection Y
    // (projection[1][1] *= -1), since Vulkan's clip space has +Y pointing down
    // relative to OpenGL. Depth range [0,1] is Vulkan's default in GLM here.
    glm::mat4 model = glm::mat4(1.0f);

    const auto camPos = cameraObj->getTransform().getPosition();
    const auto camRot = cameraObj->getTransform().getRotation();

    float yawRad = glm::radians(camRot.y);
    float pitchRad = glm::radians(camRot.x);
    glm::vec3 forward;
    forward.x = cos(pitchRad) * sin(yawRad);
    forward.y = sin(pitchRad);
    forward.z = cos(pitchRad) * cos(yawRad);
    forward = glm::normalize(forward);
    forward = -forward; // match OpenGL's -Z forward

    glm::vec3 camPosVec(camPos.x, camPos.y, camPos.z);
    glm::mat4 view = glm::lookAt(camPosVec, camPosVec + forward, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection;
    if (camera->isOrthographic()) {
        float orthoSize = camera->getOrthoSize();
        float aspect = camera->getAspectRatio();
        projection = glm::ortho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize,
                                camera->getNearDistance(), camera->getFarDistance());
    } else {
        projection = glm::perspective(glm::radians(camera->getFov()), camera->getAspectRatio(),
                                      camera->getNearDistance(), camera->getFarDistance());
    }
    projection[1][1] *= -1; // Vulkan Y flip

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    } ubo{model, view, projection};

    void* data;
    vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, uniformBufferMemory);
}

void VulkanRendererBackend::applyMaterial(Material* material) {
    if (!material)
        return;
    auto* program = material->getShaderProgram();
    if (!program || !program->isValid())
        return;
    setUniforms(program);
}

void VulkanRendererBackend::setBufferDataImpl(const std::string& name, const void* data,
                                              size_t size) {
    // Route named uniform blocks to their host-visible buffers (binding 0/1/2).
    VkDeviceMemory target = VK_NULL_HANDLE;
    if (name == "ModelViewProjection")
        target = uniformBufferMemory;
    else if (name == "MaterialData")
        target = materialBufferMemory;
    else if (name == "LightData")
        target = lightDataBufferMemory;
    else
        return;

    void* mapped;
    vkMapMemory(device, target, 0, size, 0, &mapped);
    memcpy(mapped, data, size);
    vkUnmapMemory(device, target);
}

void VulkanRendererBackend::renderWorldObjects(const std::vector<WorldObject*>& objects,
                                               const std::vector<Light*>& lights) {
    drawnObjects = 0;
    drawnVerts = 0;
    drawnTris = 0;

    beginInstanceFrame();
    nonInstancedObjects.clear();
    for (auto& [key, group] : instanceGroups)
        group.models.clear();

    // Bucket into instanced groups vs. non-instanced, like OpenGL/D3D12.
    for (auto* obj : objects) {
        if (!obj->hasMesh())
            continue;
        auto* meshRenderer = obj->getComponent<MeshRenderer>();
        if (!meshRenderer || !meshRenderer->getMaterial())
            continue;

        auto* mat = meshRenderer->getMaterial();
        if (!mat->isInstancingEnabled()) {
            nonInstancedObjects.push_back(obj);
            continue;
        }

        auto* mesh = obj->getMesh();
        auto* program = mat->getShaderProgram();
        if (!program)
            continue;

        RenderKey key{mesh->getMeshBuffer(), program->getHandle()};
        auto& group = instanceGroups[key];
        if (!group.mesh) {
            group.mesh = mesh;
            group.material = mat;
        }
        group.models.push_back(obj->getTransform().getModelMatrix());
    }

    VkCommandBuffer cmd = commandBuffers[currentImageIndex];

    auto drawGroup = [&](const Mesh* mesh, Material* mat, const glm::mat4* models, size_t count) {
        uint32_t instanceOffset = 0;
        if (!appendInstanceData(models, count, instanceOffset))
            return;

        mat->use();
        applyMaterial(mat); // binds pipeline + writes material/light UBOs
        if (!lights.empty())
            mat->applyLight(*lights[0]);

        auto* vkProgram = static_cast<VulkanShaderProgram*>(mat->getShaderProgram());
        VkPipelineLayout layout = vkProgram->getPipelineLayout();
        // Bind the descriptor set with the dynamic offset pointing at this
        // group's slice of the instance storage buffer.
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1,
                                &descriptorSets[0], 1, &instanceOffset);

        auto* vkMeshBuffer = static_cast<VulkanMeshBuffer*>(mesh->getMeshBuffer());
        VkBuffer vbs[] = {vkMeshBuffer->getVertexBuffer(), vkMeshBuffer->getNormalBuffer()};
        VkDeviceSize offsets[] = {0, 0};
        vkCmdBindVertexBuffers(cmd, 0, 2, vbs, offsets);

        const uint32_t vertsPerInstance = static_cast<uint32_t>(mesh->getVertices().size() / 3);
        const uint32_t instanceCount = static_cast<uint32_t>(count);
        vkCmdDraw(cmd, vertsPerInstance, instanceCount, 0, 0);

        drawnObjects += static_cast<int>(instanceCount);
        drawnVerts += static_cast<int>(vertsPerInstance) * static_cast<int>(instanceCount);
        drawnTris += static_cast<int>(vertsPerInstance / 3) * static_cast<int>(instanceCount);
    };

    for (auto& [key, group] : instanceGroups) {
        if (group.models.empty())
            continue;
        drawGroup(group.mesh, group.material, group.models.data(), group.models.size());
    }

    for (auto* obj : nonInstancedObjects) {
        auto* meshRenderer = obj->getComponent<MeshRenderer>();
        auto* mat = meshRenderer->getMaterial();
        glm::mat4 model = obj->getTransform().getModelMatrix();
        drawGroup(obj->getMesh(), mat, &model, 1);
    }
}

unsigned int VulkanRendererBackend::createCubemapTexture(const std::vector<std::string>& faces) {
    return 0;
}

void VulkanRendererBackend::renderSkybox(const Mesh& mesh, unsigned int shaderProgram,
                                         unsigned int textureID) {
    // Implementar skybox Vulkan
}

void VulkanRendererBackend::present(SDL_Window* window) {
    vkCmdEndRenderPass(commandBuffers[currentImageIndex]);

    if (vkEndCommandBuffer(commandBuffers[currentImageIndex]) != VK_SUCCESS) {
        printf("Failed to record command buffer\n");
        return;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentImageIndex];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
        printf("Failed to submit draw command buffer\n");
        return;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &currentImageIndex;

    vkQueuePresentKHR(presentQueue, &presentInfo);
}

// ---------------------------------------------------------------------------
// Texture loading
// ---------------------------------------------------------------------------

unsigned int VulkanRendererBackend::loadTexture(const std::string& path, uint8_t filterType) {
    int width, height, channels;
    // Force RGBA so the upload format is always R8G8B8A8_UNORM.
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        LOG_ERROR("Failed to load texture: " + path);
        return 0;
    }
    LOG_INFO("Texture loaded: " + path + " (" + std::to_string(width) + "x" +
             std::to_string(height) + ")");

    VkDeviceSize imageSize = (VkDeviceSize)width * height * 4;

    // Staging buffer (host-visible) to hold the pixels for the copy.
    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory stagingMem = VK_NULL_HANDLE;
    {
        VkBufferCreateInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bi.size = imageSize;
        bi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device, &bi, nullptr, &staging) != VK_SUCCESS) {
            stbi_image_free(pixels);
            return 0;
        }
        VkMemoryRequirements mr;
        vkGetBufferMemoryRequirements(device, staging, &mr);
        VkMemoryAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = mr.size;
        ai.memoryTypeIndex =
            findMemoryType(mr.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkAllocateMemory(device, &ai, nullptr, &stagingMem);
        vkBindBufferMemory(device, staging, stagingMem, 0);

        void* data;
        vkMapMemory(device, stagingMem, 0, imageSize, 0, &data);
        memcpy(data, pixels, (size_t)imageSize);
        vkUnmapMemory(device, stagingMem);
    }
    stbi_image_free(pixels);

    // Device-local image (sampled).
    TextureEntry entry;
    {
        VkImageCreateInfo ii{};
        ii.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ii.imageType = VK_IMAGE_TYPE_2D;
        ii.extent = {(uint32_t)width, (uint32_t)height, 1};
        ii.mipLevels = 1;
        ii.arrayLayers = 1;
        ii.format = VK_FORMAT_R8G8B8A8_UNORM;
        ii.tiling = VK_IMAGE_TILING_OPTIMAL;
        ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ii.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        ii.samples = VK_SAMPLE_COUNT_1_BIT;
        ii.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateImage(device, &ii, nullptr, &entry.image) != VK_SUCCESS) {
            vkDestroyBuffer(device, staging, nullptr);
            vkFreeMemory(device, stagingMem, nullptr);
            return 0;
        }
        VkMemoryRequirements mr;
        vkGetImageMemoryRequirements(device, entry.image, &mr);
        VkMemoryAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = mr.size;
        ai.memoryTypeIndex = findMemoryType(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(device, &ai, nullptr, &entry.memory);
        vkBindImageMemory(device, entry.image, entry.memory, 0);
    }

    // One-shot command buffer: transition UNDEFINED->TRANSFER_DST, copy, then
    // TRANSFER_DST->SHADER_READ_ONLY. loadTexture runs at scene-load time,
    // outside the per-frame command buffer, so we use a throwaway one here.
    {
        VkCommandBufferAllocateInfo cbai{};
        cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbai.commandPool = commandPool;
        cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbai.commandBufferCount = 1;
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &cbai, &cmd);

        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &bi);

        VkImageMemoryBarrier toCopy{};
        toCopy.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toCopy.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toCopy.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toCopy.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toCopy.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toCopy.image = entry.image;
        toCopy.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        toCopy.srcAccessMask = 0;
        toCopy.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &toCopy);

        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent = {(uint32_t)width, (uint32_t)height, 1};
        vkCmdCopyBufferToImage(cmd, staging, entry.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &region);

        VkImageMemoryBarrier toRead = toCopy;
        toRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        toRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &toRead);

        vkEndCommandBuffer(cmd);

        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cmd;
        vkQueueSubmit(graphicsQueue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue); // simple sync; load-time only
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    }

    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, stagingMem, nullptr);

    // Image view.
    VkImageViewCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image = entry.image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = VK_FORMAT_R8G8B8A8_UNORM;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    if (vkCreateImageView(device, &vi, nullptr, &entry.view) != VK_SUCCESS) {
        vkDestroyImage(device, entry.image, nullptr);
        vkFreeMemory(device, entry.memory, nullptr);
        return 0;
    }

    textures.push_back(entry);
    return static_cast<unsigned int>(textures.size()); // 1-based id
}

void VulkanRendererBackend::drawSprite(const Sprite& sprite) {}

// ---------------------------------------------------------------------------
// Text rendering (MSDF)
// ---------------------------------------------------------------------------

bool VulkanRendererBackend::createTextureSampler() {
    if (textureSampler != VK_NULL_HANDLE)
        return true;
    VkSamplerCreateInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    si.magFilter = VK_FILTER_LINEAR;
    si.minFilter = VK_FILTER_LINEAR;
    si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    si.anisotropyEnable = VK_FALSE;
    si.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    return vkCreateSampler(device, &si, nullptr, &textureSampler) == VK_SUCCESS;
}

VkShaderModule VulkanRendererBackend::loadShaderModule(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open shader: " + path);
        return VK_NULL_HANDLE;
    }
    size_t size = (size_t)file.tellg();
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);

    VkShaderModuleCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = buffer.size();
    ci.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
    VkShaderModule module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &ci, nullptr, &module) != VK_SUCCESS) {
        LOG_ERROR("Failed to create shader module: " + path);
        return VK_NULL_HANDLE;
    }
    return module;
}

bool VulkanRendererBackend::createTextPipeline(const std::string& vertPath,
                                               const std::string& fragPath) {
    VkShaderModule vs = loadShaderModule(vertPath);
    VkShaderModule fs = loadShaderModule(fragPath);
    if (!vs || !fs)
        return false;

    // Descriptor set layout: image(b0), sampler(b1), projection UBO(b4),
    // color UBO(b5) — matching the SPIR-V reflection.
    VkDescriptorSetLayoutBinding b[4] = {};
    b[0].binding = 0;
    b[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    b[0].descriptorCount = 1;
    b[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    b[1].binding = 1;
    b[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    b[1].descriptorCount = 1;
    b[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    b[2].binding = 4;
    b[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    b[2].descriptorCount = 1;
    b[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    b[3].binding = 5;
    b[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    b[3].descriptorCount = 1;
    b[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo li{};
    li.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    li.bindingCount = 4;
    li.pBindings = b;
    if (vkCreateDescriptorSetLayout(device, &li, nullptr, &textDescriptorSetLayout) != VK_SUCCESS)
        return false;

    VkPipelineLayoutCreateInfo pli{};
    pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &textDescriptorSetLayout;
    if (vkCreatePipelineLayout(device, &pli, nullptr, &textPipelineLayout) != VK_SUCCESS)
        return false;

    // Descriptor pool + set.
    VkDescriptorPoolSize sizes[3] = {};
    sizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    sizes[0].descriptorCount = 1;
    sizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    sizes[1].descriptorCount = 1;
    sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    sizes[2].descriptorCount = 2;
    VkDescriptorPoolCreateInfo pi{};
    pi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pi.poolSizeCount = 3;
    pi.pPoolSizes = sizes;
    pi.maxSets = 1;
    if (vkCreateDescriptorPool(device, &pi, nullptr, &textDescriptorPool) != VK_SUCCESS)
        return false;

    VkDescriptorSetAllocateInfo dsai{};
    dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsai.descriptorPool = textDescriptorPool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts = &textDescriptorSetLayout;
    if (vkAllocateDescriptorSets(device, &dsai, &textDescriptorSet) != VK_SUCCESS)
        return false;

    // Pipeline: 2D input (vec2 pos @loc0, vec2 uv @loc1) interleaved, no depth,
    // alpha blending.
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vs;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fs;
    stages[1].pName = "main";

    VkVertexInputBindingDescription bind{};
    bind.binding = 0;
    bind.stride = 4 * sizeof(float);
    bind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription attrs[2] = {};
    attrs[0].location = 0;
    attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[0].offset = 0;
    attrs[1].location = 1;
    attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[1].offset = 2 * sizeof(float);

    VkPipelineVertexInputStateCreateInfo vin{};
    vin.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vin.vertexBindingDescriptionCount = 1;
    vin.pVertexBindingDescriptions = &bind;
    vin.vertexAttributeDescriptionCount = 2;
    vin.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    auto extent = getSwapchainExtent();
    VkViewport vp{0, 0, (float)extent.width, (float)extent.height, 0.0f, 1.0f};
    VkRect2D sc{{0, 0}, extent};
    VkPipelineViewportStateCreateInfo vps{};
    vps.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vps.viewportCount = 1;
    vps.pViewports = &vp;
    vps.scissorCount = 1;
    vps.pScissors = &sc;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_FALSE;
    ds.depthWriteEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState cba{};
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    cba.blendEnable = VK_TRUE;
    cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba.colorBlendOp = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba.alphaBlendOp = VK_BLEND_OP_ADD;
    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments = &cba;

    VkGraphicsPipelineCreateInfo gp{};
    gp.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp.stageCount = 2;
    gp.pStages = stages;
    gp.pVertexInputState = &vin;
    gp.pInputAssemblyState = &ia;
    gp.pViewportState = &vps;
    gp.pRasterizationState = &rs;
    gp.pMultisampleState = &ms;
    gp.pDepthStencilState = &ds;
    gp.pColorBlendState = &cb;
    gp.layout = textPipelineLayout;
    gp.renderPass = getRenderPass();
    gp.subpass = 0;

    VkResult r = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gp, nullptr, &textPipeline);
    vkDestroyShaderModule(device, vs, nullptr);
    vkDestroyShaderModule(device, fs, nullptr);
    return r == VK_SUCCESS;
}

bool VulkanRendererBackend::initText(const FontAtlas& atlas, unsigned int textureID,
                                     const std::string& vertPath, const std::string& fragPath) {
    // textureID is 1-based (0 = load failed). Bail out safely so we never index
    // textures[-1] below.
    if (textureID == 0 || textureID > textures.size()) {
        LOG_ERROR("initText called with invalid textureID; text disabled");
        return false;
    }

    textAtlas = &atlas;
    textTextureID = textureID; // 1-based

    if (!createTextureSampler())
        return false;
    if (!createTextPipeline(vertPath, fragPath))
        return false;

    // UBO dynamic offset alignment.
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    uboAlignment = props.limits.minUniformBufferOffsetAlignment;
    if (uboAlignment == 0)
        uboAlignment = 64;

    // Each CB slot holds either the projection (mat4 = 64B) or the color block
    // (vec4 + float). Round the slot up to the alignment.
    VkDeviceSize slotContent = sizeof(glm::mat4); // 64, larger than color block
    textCBSlotSize = (slotContent + uboAlignment - 1) & ~(uboAlignment - 1);
    textCBSlots = 256; // plenty for many lines/frame (2 slots per drawText)

    // Per-frame text vertex arena.
    textVertexCapacity = 6 * 4 * 4096;
    {
        VkBufferCreateInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bi.size = textVertexCapacity * sizeof(float);
        bi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device, &bi, nullptr, &textVertexBuffer) != VK_SUCCESS)
            return false;
        VkMemoryRequirements mr;
        vkGetBufferMemoryRequirements(device, textVertexBuffer, &mr);
        VkMemoryAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = mr.size;
        ai.memoryTypeIndex =
            findMemoryType(mr.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkAllocateMemory(device, &ai, nullptr, &textVertexMemory);
        vkBindBufferMemory(device, textVertexBuffer, textVertexMemory, 0);
        vkMapMemory(device, textVertexMemory, 0, bi.size, 0, &textVertexMapped);
    }

    // Per-frame text constant buffer arena (dynamic UBO).
    {
        VkBufferCreateInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bi.size = textCBSlotSize * textCBSlots;
        bi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device, &bi, nullptr, &textCB) != VK_SUCCESS)
            return false;
        VkMemoryRequirements mr;
        vkGetBufferMemoryRequirements(device, textCB, &mr);
        VkMemoryAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = mr.size;
        ai.memoryTypeIndex =
            findMemoryType(mr.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkAllocateMemory(device, &ai, nullptr, &textCBMemory);
        vkBindBufferMemory(device, textCB, textCBMemory, 0);
        vkMapMemory(device, textCBMemory, 0, bi.size, 0, &textCBMapped);
    }

    // Write the descriptor set once: image(b0), sampler(b1), and the dynamic
    // UBOs (b4 projection, b5 color) pointing at the CB arena base (range = one
    // slot; the per-draw dynamic offset selects the slot).
    const TextureEntry& tex = textures[textTextureID - 1];

    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imgInfo.imageView = tex.view;
    VkDescriptorImageInfo sampInfo{};
    sampInfo.sampler = textureSampler;

    VkDescriptorBufferInfo projInfo{};
    projInfo.buffer = textCB;
    projInfo.offset = 0;
    projInfo.range = sizeof(glm::mat4);
    VkDescriptorBufferInfo colorInfo{};
    colorInfo.buffer = textCB;
    colorInfo.offset = 0;
    colorInfo.range = sizeof(glm::vec4) + sizeof(float) * 4; // color + distRange (+pad)

    VkWriteDescriptorSet w[4] = {};
    w[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = textDescriptorSet;
    w[0].dstBinding = 0;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    w[0].descriptorCount = 1;
    w[0].pImageInfo = &imgInfo;
    w[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[1].dstSet = textDescriptorSet;
    w[1].dstBinding = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    w[1].descriptorCount = 1;
    w[1].pImageInfo = &sampInfo;
    w[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[2].dstSet = textDescriptorSet;
    w[2].dstBinding = 4;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    w[2].descriptorCount = 1;
    w[2].pBufferInfo = &projInfo;
    w[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[3].dstSet = textDescriptorSet;
    w[3].dstBinding = 5;
    w[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    w[3].descriptorCount = 1;
    w[3].pBufferInfo = &colorInfo;
    vkUpdateDescriptorSets(device, 4, w, 0, nullptr);

    return true;
}

void VulkanRendererBackend::beginTextFrame() {
    textVertexCursor = 0;
    textCBCursor = 0;
}

void VulkanRendererBackend::drawText(const std::string& text, float x, float y, float scale,
                                     glm::vec4 color, int screenWidth, int screenHeight) {
    if (!textAtlas || textPipeline == VK_NULL_HANDLE || textTextureID == 0)
        return;
    if (textCBCursor + 2 > textCBSlots)
        return;

    // Build glyph quads (same layout/order as OpenGL/D3D12).
    std::vector<float> vertices;
    vertices.reserve(text.size() * 6 * 4);
    float cursorX = x;
    uint32_t prevChar = 0;
    for (char c : text) {
        uint32_t unicode = (uint32_t)(unsigned char)c;
        auto it = textAtlas->glyphs.find(unicode);
        if (it == textAtlas->glyphs.end()) {
            prevChar = unicode;
            continue;
        }
        const GlyphInfo& g = it->second;
        cursorX += textAtlas->getKerning(prevChar, unicode) * scale;
        if (g.hasGeometry) {
            float x0 = cursorX + g.planeLeft * scale, x1 = cursorX + g.planeRight * scale;
            float y0 = y - g.planeTop * scale, y1 = y - g.planeBottom * scale;
            float u0 = g.atlasLeft / textAtlas->atlasWidth,
                  u1 = g.atlasRight / textAtlas->atlasWidth;
            float v0 = 1.0f - (g.atlasBottom / textAtlas->atlasHeight);
            float v1 = 1.0f - (g.atlasTop / textAtlas->atlasHeight);
            float quad[6][4] = {
                {x0, y0, u0, v1}, {x0, y1, u0, v0}, {x1, y1, u1, v0},
                {x0, y0, u0, v1}, {x1, y1, u1, v0}, {x1, y0, u1, v1},
            };
            for (auto& v : quad)
                vertices.insert(vertices.end(), v, v + 4);
        }
        cursorX += g.advance * scale;
        prevChar = unicode;
    }
    if (vertices.empty())
        return;
    if (textVertexCursor + vertices.size() > textVertexCapacity)
        return;

    // Append vertices to the per-frame arena.
    size_t vtxOffsetFloats = textVertexCursor;
    memcpy(static_cast<float*>(textVertexMapped) + vtxOffsetFloats, vertices.data(),
           vertices.size() * sizeof(float));
    textVertexCursor += vertices.size();

    // Projection (b4): top-left origin ortho, Y flipped for Vulkan clip space,
    // Top-left origin (screen y=0 at top, y=height at bottom) mapped into
    // Vulkan's Y-down clip space. We do the flip by swapping bottom/top in the
    // ortho args (bottom=0, top=height) rather than post-multiplying
    // [1][1] *= -1: the latter flips only the scale and leaves the Y
    // translation unchanged, which pushed the text off the top of the screen.
    glm::mat4 proj = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight, -1.0f, 1.0f);

    uint32_t projSlot = textCBCursor++;
    uint32_t colorSlot = textCBCursor++;
    uint8_t* cbBase = static_cast<uint8_t*>(textCBMapped);
    memcpy(cbBase + projSlot * textCBSlotSize, glm::value_ptr(proj), sizeof(glm::mat4));

    struct ColorBlock {
        glm::vec4 color;
        float distRange;
        float pad[3];
    } colorData{color, textAtlas->distanceRange * (scale / textAtlas->atlasSize), {0, 0, 0}};
    memcpy(cbBase + colorSlot * textCBSlotSize, &colorData, sizeof(ColorBlock));

    // Record the draw into the frame's command buffer (open since clear()).
    VkCommandBuffer cmd = commandBuffers[currentImageIndex];
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, textPipeline);

    uint32_t dynOffsets[2] = {projSlot * (uint32_t)textCBSlotSize,
                              colorSlot * (uint32_t)textCBSlotSize};
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, textPipelineLayout, 0, 1,
                            &textDescriptorSet, 2, dynOffsets);

    VkDeviceSize vbOffset = vtxOffsetFloats * sizeof(float);
    vkCmdBindVertexBuffers(cmd, 0, 1, &textVertexBuffer, &vbOffset);
    vkCmdDraw(cmd, (uint32_t)(vertices.size() / 4), 1, 0, 0);
}
