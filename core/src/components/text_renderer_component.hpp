#ifndef TEXT_RENDERER_COMPONENT_HPP
#define TEXT_RENDERER_COMPONENT_HPP

#include "../font_atlas.hpp"
#include "../text_renderer.hpp"
#include "component.hpp"
#include <memory>

/**
 * @brief Componente que permite a um objeto desenhar texto na tela.
 *
 * Agrupa o atlas de fonte e o TextRenderer usados para renderizar texto. O
 * overlay de depuração do engine (FPS/estatísticas) procura por este componente
 * na cena ativa.
 *
 * @see Component, TextRenderer, FontAtlas
 */
class TextRendererComponent : public Component {
  private:
    std::unique_ptr<FontAtlas> fontAtlas;       ///< Atlas da fonte usada.
    std::unique_ptr<TextRenderer> textRenderer; ///< Renderer de texto associado.

  public:
    /** @brief Define o atlas de fonte (assume a posse). */
    void setFontAtlas(std::unique_ptr<FontAtlas> atlas) { fontAtlas = std::move(atlas); }
    /** @brief Define o renderer de texto (assume a posse). */
    void setTextRenderer(std::unique_ptr<TextRenderer> renderer) { textRenderer = std::move(renderer); }

    /** @brief Retorna o atlas de fonte, ou @c nullptr. */
    FontAtlas* getFontAtlas() { return fontAtlas.get(); }
    /** @brief Retorna o renderer de texto, ou @c nullptr. */
    TextRenderer* getTextRenderer() { return textRenderer.get(); }
    /** @brief Indica se há um renderer de texto definido. */
    bool hasTextRenderer() const { return textRenderer != nullptr; }
};

#endif
