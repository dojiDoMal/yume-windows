#define CLASS_NAME "RendererConfig"
#include "log_macros.hpp"

#include "nlohmann/json.hpp"
#include "renderer_config.hpp"
#include <algorithm>
#include <fstream>

namespace {

// Case-insensitive string -> GraphicsAPI. Unknown values fall back to the
// provided default so a typo never hard-fails the boot.
GraphicsAPI parseApi(const std::string& raw, GraphicsAPI fallback) {
    std::string s = raw;
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (s == "opengl" || s == "gl")
        return GraphicsAPI::OPENGL;
    if (s == "vulkan" || s == "vk")
        return GraphicsAPI::VULKAN;
    if (s == "directx12" || s == "d3d12" || s == "dx12" || s == "directx")
        return GraphicsAPI::DIRECTX12;
    if (s == "webgl")
        return GraphicsAPI::WEBGL;

    LOG_WARN("Unknown renderer.api '" + raw + "', keeping default");
    return fallback;
}

} // namespace

RendererConfig loadRendererConfig(const std::string& path) {
    RendererConfig config; // starts at safe defaults

    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_WARN("Config file not found: " + path +
                 " - using defaults (api=opengl, srgb=false, "
                 "vsync=true)");
        return config;
    }

    nlohmann::json j;
    try {
        // ignore_comments = true so project.conf can carry // reference notes
        // next to each value (nlohmann rejects comments by default).
        j = nlohmann::json::parse(file, /*cb=*/nullptr, /*allow_exceptions=*/true,
                                  /*ignore_comments=*/true);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse " + path + ": " + e.what() + " - using defaults");
        return config;
    }

    // All fields are optional; each missing/invalid one keeps its default.
    if (j.contains("renderer") && j["renderer"].is_object()) {
        const auto& r = j["renderer"];

        if (r.contains("api") && r["api"].is_string())
            config.api = parseApi(r["api"].get<std::string>(), config.api);

        if (r.contains("srgb") && r["srgb"].is_boolean())
            config.srgb = r["srgb"].get<bool>();

        if (r.contains("vsync") && r["vsync"].is_boolean())
            config.vsync = r["vsync"].get<bool>();
    } else {
        LOG_WARN("Config " + path + " has no 'renderer' object - using defaults");
    }

    return config;
}
