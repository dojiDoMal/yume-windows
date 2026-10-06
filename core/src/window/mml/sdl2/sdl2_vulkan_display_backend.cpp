#include "sdl2_vulkan_display_backend.hpp"
#define CLASS_NAME "SDL2VulkanDisplayBackend"
#include "log_macros.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

unsigned int SDL2VulkanDisplayBackend::getApiWindowFlags() const { return SDL_WINDOW_VULKAN; }

bool SDL2VulkanDisplayBackend::createVulkanSurface(void* window, void* vkInstance,
                                                   void* outSurface) {
    if (!window || !vkInstance || !outSurface) {
        LOG_ERROR("createVulkanSurface: null argument");
        return false;
    }

    // The VkInstance / VkSurfaceKHR handles are passed as void* so the public
    // DisplayBackend header stays free of Vulkan types. They are plain opaque
    // handles, so the casts below are safe.
    auto instance = static_cast<VkInstance>(vkInstance);
    auto* surface = static_cast<VkSurfaceKHR*>(outSurface);

    if (!SDL_Vulkan_CreateSurface(reinterpret_cast<SDL_Window*>(window), instance, surface)) {
        LOG_ERROR(std::string("SDL_Vulkan_CreateSurface failed: ") + SDL_GetError());
        return false;
    }
    return true;
}

std::vector<const char*> SDL2VulkanDisplayBackend::getVulkanInstanceExtensions() {
    unsigned int count = 0;
    SDL_Vulkan_GetInstanceExtensions(nullptr, &count, nullptr);
    std::vector<const char*> extensions(count);
    SDL_Vulkan_GetInstanceExtensions(nullptr, &count, extensions.data());
    return extensions;
}
