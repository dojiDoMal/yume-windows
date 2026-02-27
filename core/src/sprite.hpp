#ifndef SPRITE_HPP
#define SPRITE_HPP

class Sprite {
  private:
    unsigned int textureID = 0;
    float width = 1.0f;
    float height = 1.0f;

  public:
    Sprite(float w = 1.0f, float h = 1.0f) : width(w), height(h) {}

    void setTexture(unsigned int texID) { textureID = texID; }
    unsigned int getTexture() const { return textureID; }
    float getWidth() const { return width; }
    float getHeight() const { return height; }
};

#endif
