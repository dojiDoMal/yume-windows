#ifndef TEXT_RENDERER_COMPONENT_HPP
#define TEXT_RENDERER_COMPONENT_HPP

#include "../font_atlas.hpp"
#include "../text_renderer.hpp"
#include "component.hpp"
#include <memory>

class TextRendererComponent : public Component {
  private:
    std::unique_ptr<FontAtlas> fontAtlas;
    std::unique_ptr<TextRenderer> textRenderer;

  public:
    void setFontAtlas(std::unique_ptr<FontAtlas> atlas) { fontAtlas = std::move(atlas); }
    void setTextRenderer(std::unique_ptr<TextRenderer> renderer) { textRenderer = std::move(renderer); }

    FontAtlas* getFontAtlas() { return fontAtlas.get(); }
    TextRenderer* getTextRenderer() { return textRenderer.get(); }
    bool hasTextRenderer() const { return textRenderer != nullptr; }
};

#endif
