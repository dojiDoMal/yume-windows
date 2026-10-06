#ifndef SDL2_VULKAN_DISPLAY_BACKEND_HPP
#define SDL2_VULKAN_DISPLAY_BACKEND_HPP

#include "window/mml/sdl2/sdl2_display_backend.hpp"

/**
 * @brief Backend de janela SDL2 para Vulkan.
 *
 * Acrescenta à base os serviços Vulkan que dependem da janela: criação da
 * surface e consulta das extensões de instância exigidas pela plataforma. É o
 * único ponto que chama SDL_Vulkan_*; o VulkanRendererBackend consome via
 * void*. Compilado apenas em plataformas com Vulkan (fora de 3DS/Switch/Web).
 *
 * @see SDL2DisplayBackend, DisplayBackend, VulkanRendererBackend
 */
class SDL2VulkanDisplayBackend : public SDL2DisplayBackend {
  public:
    unsigned int getApiWindowFlags() const override;
    bool createVulkanSurface(void* window, void* vkInstance, void* outSurface) override;
    std::vector<const char*> getVulkanInstanceExtensions() override;
};

#endif // SDL2_VULKAN_DISPLAY_BACKEND_HPP
