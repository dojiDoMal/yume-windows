#include "text_renderer.hpp"

bool TextRenderer::init(RendererBackend& b, const FontAtlas& atlas, unsigned int textureID,
                        const std::string& vertPath, const std::string& fragPath) {
    backend = &b;
    return backend->initText(atlas, textureID, vertPath, fragPath);
}

void TextRenderer::draw(const std::string& text, float x, float y, float scale, glm::vec4 color,
                        int screenWidth, int screenHeight) {
    if (backend)
        backend->drawText(text, x, y, scale, color, screenWidth, screenHeight);
}
