#ifndef RENDERER_CONFIG_HPP
#define RENDERER_CONFIG_HPP

#include "graphics_api.hpp"
#include <string>

// Project-level renderer configuration, loaded once at boot from a JSON file
// (project.conf). It centralizes presentation decisions that are properties of
// the engine/application rather than of any single scene.
//
// Fields:
//  - api:   which graphics backend to create (replaces the hardcoded value in
//           main.cpp).
//  - srgb:  false (default) = linear/UNORM swapchain/framebuffer, matching the
//           current behavior across all backends (colors written as-is).
//           true = sRGB swapchain/framebuffer, so the GPU applies a
//           linear->sRGB conversion on write. NOTE (phase 2): this only changes
//           the OUTPUT color space; it does NOT reinterpret input colors from
//           the .scn or textures as sRGB. Gamma-correct input handling is a
//           separate, not-yet-implemented step.
//  - vsync: true (default) = cap to the display refresh (GL swap interval 1 /
//           Vulkan FIFO / D3D12 Present(1)). false = uncapped.
struct RendererConfig {
    GraphicsAPI api = GraphicsAPI::OPENGL;
    bool srgb = false;
    bool vsync = true;
};

// Loads the config from `path` (default "project.conf"). Missing file or any
// parse error is non-fatal: the returned config keeps its safe defaults, and a
// warning is logged. This guarantees the engine still boots without the file.
RendererConfig loadRendererConfig(const std::string& path = "project.conf");

#endif // RENDERER_CONFIG_HPP
