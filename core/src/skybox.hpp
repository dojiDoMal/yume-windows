#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include "material.hpp"
#include "mesh.hpp"
#include <memory>

/**
 * @brief Céu de fundo desenhado como um cubo texturizado (cubemap).
 *
 * Mantém a malha do cubo, o material usado para desenhá-lo e o id da textura de
 * cubemap (as 6 faces). Normalmente é renderizado "atrás" de tudo, usando só a
 * parte de rotação da matriz de visão para parecer infinitamente distante.
 *
 * @see Mesh, Material
 */
class Skybox {
  private:
    std::unique_ptr<Mesh> cubeMesh;          ///< Malha do cubo do skybox.
    std::unique_ptr<Material> skyboxMaterial; ///< Material usado no desenho.
    unsigned int textureID = 0;              ///< Handle da textura de cubemap.

  public:
    Skybox();
    ~Skybox() = default;

    /**
     * @brief Prepara a malha do cubo e o material para renderização.
     * @return @c true em caso de sucesso.
     */
    bool init();
    /** @brief Define o id da textura de cubemap. */
    void setTextureID(unsigned int id) { textureID = id; }
    /** @brief Retorna o id da textura de cubemap. */
    unsigned int getTextureID() const { return textureID; }
    /** @brief Retorna a malha do cubo. */
    Mesh* getMesh() const { return cubeMesh.get(); }
    /** @brief Retorna o material do skybox. */
    Material* getMaterial() const { return skyboxMaterial.get(); }
    /** @brief Define o material do skybox (assume a posse). */
    void setMaterial(std::unique_ptr<Material> material);
};

#endif
