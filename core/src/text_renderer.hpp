#ifndef TEXT_RENDERER_HPP
#define TEXT_RENDERER_HPP

#include "color.hpp"
#include "font_atlas.hpp"
#include "renderer/renderer_backend.hpp"
#include <string>

/**
 * @brief Desenha texto na tela usando um atlas de fonte MSDF.
 *
 * Inicializado com um FontAtlas e os shaders de texto, converte uma string em
 * quads texturizados (um por glifo) e os desenha em coordenadas de tela.
 *
 * @see FontAtlas
 */
class TextRenderer {
  public:
    /**
     * @brief Prepara o renderer de texto com fonte, textura e shaders.
     * @param backend   Backend de renderização ativo.
     * @param atlas     Atlas da fonte (métricas e posições dos glifos).
     * @param textureID Handle da textura do atlas na GPU.
     * @param vertPath  Caminho do shader de vértice.
     * @param fragPath  Caminho do shader de fragmento.
     * @return @c true em caso de sucesso.
     */
    bool init(RendererBackend& backend, const FontAtlas& atlas, unsigned int textureID,
              const std::string& vertPath, const std::string& fragPath);

    /**
     * @brief Desenha uma string na tela.
     * @param text         Texto a desenhar.
     * @param x            Posição horizontal, em pixels.
     * @param y            Posição vertical, em pixels.
     * @param scale        Fator de escala do texto.
     * @param color        Cor do texto.
     * @param screenWidth  Largura da tela, em pixels.
     * @param screenHeight Altura da tela, em pixels.
     */
    void draw(const std::string& text, float x, float y, float scale, ColorRGBA color,
              int screenWidth, int screenHeight);

  private:
    RendererBackend* backend = nullptr;
};

#endif
