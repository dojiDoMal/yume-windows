#ifndef SPRITE_HPP
#define SPRITE_HPP

/**
 * @brief Imagem 2D desenhada no mundo: uma textura com largura e altura.
 *
 * Representa um quad texturizado. Guarda o id da textura (handle de GPU) e as
 * dimensões em unidades de mundo.
 */
class Sprite {
  private:
    unsigned int textureID = 0; ///< Handle da textura na GPU.
    float width = 1.0f;         ///< Largura do sprite.
    float height = 1.0f;        ///< Altura do sprite.

  public:
    /**
     * @brief Cria um sprite com as dimensões informadas.
     * @param w Largura (padrão 1.0).
     * @param h Altura (padrão 1.0).
     */
    Sprite(float w = 1.0f, float h = 1.0f) : width(w), height(h) {}

    /** @brief Define a textura do sprite pelo seu id de GPU. */
    void setTexture(unsigned int texID) { textureID = texID; }
    /** @brief Retorna o id da textura. */
    unsigned int getTexture() const { return textureID; }
    /** @brief Retorna a largura do sprite. */
    float getWidth() const { return width; }
    /** @brief Retorna a altura do sprite. */
    float getHeight() const { return height; }
};

#endif
