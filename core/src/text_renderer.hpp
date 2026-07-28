#ifndef TEXT_RENDERER_HPP
#define TEXT_RENDERER_HPP

#include "font_atlas.hpp"
#include "renderer/renderer_backend.hpp"
#include <glm/glm.hpp>
#include <string>

class TextRenderer {
  public:
    bool init(RendererBackend& backend, const FontAtlas& atlas, unsigned int textureID,
              const std::string& vertPath, const std::string& fragPath);
    void draw(const std::string& text, float x, float y, float scale, glm::vec4 color,
              int screenWidth, int screenHeight);

  private:
    RendererBackend* backend = nullptr;
};

#endif
