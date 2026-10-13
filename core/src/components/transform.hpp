#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "math/matrix4.hpp"
#include "math/quaternion.hpp"
#include "math/vector3.hpp"

/**
 * @brief Posição, rotação e escala de um objeto no espaço 3D.
 *
 * Todo WorldObject tem um Transform. A matriz de modelo (model matrix) é
 * calculada a partir desses valores e fica em cache: só é recomputada quando
 * algum setter marca o estado como "sujo" (dirty), evitando trabalho redundante.
 *
 * A orientação é guardada internamente como um Quaternion (sem gimbal lock,
 * compõe bem — base da dinâmica rotacional da física). A API de Euler
 * (getRotation/setRotation, em GRAUS) é mantida como camada de compatibilidade:
 * cena, câmera e scripts continuam falando em Euler, enquanto a física usa o
 * quatérnion diretamente via getRotationQuat/setRotationQuat.
 */
class Transform {
  private:
    Vector3 position;                              ///< Posição no espaço de mundo.
    Quaternion orientation = QUATERNION::IDENTITY; ///< Orientação (fonte da verdade).
    Vector3 scale = {1.0f, 1.0f, 1.0f};            ///< Escala por eixo.

    // Euler (graus) espelhando a última orientação SETADA via setRotation. Serve
    // para que getRotation() devolva exatamente o que o usuário/script setou
    // (edição por eixo estável, sem o ping-pong de quat->euler->quat). É marcado
    // como "stale" quando a orientação muda por quatérnion (física) e então
    // getRotation() reconstrói o Euler a partir do quatérnion.
    mutable Vector3 cachedEuler = {0.0f, 0.0f, 0.0f};
    mutable bool eulerStale = false;

    mutable Matrix4 cachedMatrix{1.0f}; ///< Matriz de modelo em cache.
    mutable bool dirty = true;          ///< @c true quando a matriz precisa ser recomputada.

  public:
    /** @brief Retorna a matriz de modelo, recomputando-a se necessário. */
    Matrix4 getModelMatrix() const;

    /** @brief Retorna a posição atual. */
    Vector3 getPosition() const;
    /** @brief Define a posição e marca a matriz como suja. */
    void setPosition(const Vector3& pos);

    /**
     * @brief Retorna a rotação atual em ângulos de Euler (graus, por eixo).
     *
     * Camada de compatibilidade: devolve o Euler setado via setRotation, ou um
     * Euler derivado do quatérnion se a orientação foi alterada por
     * setRotationQuat (ex.: pela física).
     */
    Vector3 getRotation() const;
    /** @brief Define a rotação por ângulos de Euler (graus) e marca como suja. */
    void setRotation(const Vector3& rot);

    /** @brief Retorna a orientação como quatérnion (representação canônica). */
    Quaternion getRotationQuat() const;
    /** @brief Define a orientação por quatérnion (usado pela física). */
    void setRotationQuat(const Quaternion& q);

    /** @brief Retorna a escala atual. */
    Vector3 getScale() const;
    /** @brief Define a escala e marca a matriz como suja. */
    void setScale(const Vector3& scl);
};

#endif
