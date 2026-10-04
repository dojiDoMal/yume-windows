#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "matrix4.hpp"
#include "vector3.hpp"

/**
 * @brief Posição, rotação e escala de um objeto no espaço 3D.
 *
 * Todo WorldObject tem um Transform. A matriz de modelo (model matrix) é
 * calculada a partir desses valores e fica em cache: só é recomputada quando
 * algum setter marca o estado como "sujo" (dirty), evitando trabalho redundante.
 *
 * A rotação é guardada como ângulos de Euler em um Vector3 (um ângulo por eixo).
 */
class Transform {
  private:
    Vector3 position;              ///< Posição no espaço de mundo.
    Vector3 rotation;              ///< Rotação em ângulos de Euler (por eixo).
    Vector3 scale;                 ///< Escala por eixo.
    mutable Matrix4 cachedMatrix{1.0f}; ///< Matriz de modelo em cache.
    mutable bool dirty = true;     ///< @c true quando a matriz precisa ser recomputada.

  public:
    /** @brief Retorna a matriz de modelo, recomputando-a se necessário. */
    Matrix4 getModelMatrix() const;

    /** @brief Retorna a posição atual. */
    Vector3 getPosition() const;
    /** @brief Define a posição e marca a matriz como suja. */
    void setPosition(const Vector3& pos);

    /** @brief Retorna a rotação atual (Euler, por eixo). */
    Vector3 getRotation() const;
    /** @brief Define a rotação e marca a matriz como suja. */
    void setRotation(const Vector3& rot);

    /** @brief Retorna a escala atual. */
    Vector3 getScale() const;
    /** @brief Define a escala e marca a matriz como suja. */
    void setScale(const Vector3& scl);
};

#endif
