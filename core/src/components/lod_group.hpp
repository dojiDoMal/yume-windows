#ifndef LOD_GROUP_HPP
#define LOD_GROUP_HPP

#include "../math.hpp"
#include "../mesh.hpp"
#include "../vector3.hpp"
#include "component.hpp"
#include <cmath>
#include <memory>
#include <vector>

/// @brief Um nível de detalhe (LOD): uma malha e o limiar que o ativa.
struct LodLevel {
    std::shared_ptr<Mesh> mesh;  ///< Malha deste nível de detalhe.
    float screenSpaceThreshold;  ///< Fração mínima da tela para usar este nível.
};

/**
 * @brief Componente de nível de detalhe (LOD): troca a malha conforme o tamanho na tela.
 *
 * Guarda vários LodLevel, do mais detalhado (LOD0) ao mais simples. A cada
 * frame, update() estima que fração da altura da viewport o objeto ocupa e
 * escolhe o nível apropriado; se o objeto for pequeno demais para qualquer
 * limiar, é descartado (culled). Reduz o custo de desenhar objetos distantes.
 *
 * @see Component, Mesh
 */
class LodGroup : public Component {
  private:
    std::vector<LodLevel> levels; ///< Níveis de detalhe, do mais ao menos detalhado.
    int activeLevelIndex = 0;     ///< Índice do nível selecionado no último update().

    /**
     * @brief Estima a fração da altura da viewport ocupada pelo objeto.
     * @return Tamanho projetado como fração da altura da viewport.
     */
    float computeScreenSpacePercentage(const Vector3& objPos, float radius, const Vector3& camPos,
                                       float fovRad, float viewportHeight) const {
        float dist = Yume::Math::length(objPos - camPos);
        if (dist < 0.0001f)
            return 1.0f;
        float projectedSize = (radius / (dist * std::tan(fovRad * 0.5f)));
        return projectedSize; // já é fração da altura da viewport
    }

  public:
    /**
     * @brief Adiciona um nível de detalhe.
     * @param mesh      Malha do nível.
     * @param threshold Fração mínima da tela para que este nível seja escolhido.
     */
    void addLevel(std::shared_ptr<Mesh> mesh, float threshold) {
        levels.push_back({std::move(mesh), threshold});
    }

    /**
     * @brief Seleciona o nível de detalhe adequado para o frame atual.
     * @param objPos         Posição do objeto no mundo.
     * @param boundingRadius Raio da esfera envolvente do objeto.
     * @param camPos         Posição da câmera.
     * @param fovDeg         Campo de visão vertical da câmera, em graus.
     * @param viewportHeight Altura da viewport.
     * @return @c true se o objeto deve ser renderizado; @c false se foi descartado (culled).
     */
    bool update(const Vector3& objPos, float boundingRadius, const Vector3& camPos, float fovDeg,
                float viewportHeight) {
        float fovRad = Yume::Math::radians(fovDeg);
        float ssp =
            computeScreenSpacePercentage(objPos, boundingRadius, camPos, fovRad, viewportHeight);

        // Percorre do LOD0 (maior threshold) ao último
        for (int i = 0; i < (int)levels.size(); i++) {
            if (ssp >= levels[i].screenSpaceThreshold) {
                activeLevelIndex = i;
                return true;
            }
        }
        // Culled
        return false;
    }

    /** @brief Retorna a malha do nível selecionado no último update(), ou @c nullptr. */
    Mesh* getActiveMesh() const {
        if (activeLevelIndex < (int)levels.size())
            return levels[activeLevelIndex].mesh.get();
        return nullptr;
    }

    /** @brief Versão shared_ptr de getActiveMesh(). */
    std::shared_ptr<Mesh> getActiveMeshShared() const {
        if (activeLevelIndex < (int)levels.size())
            return levels[activeLevelIndex].mesh;
        return nullptr;
    }

    /** @brief Retorna o número de níveis de detalhe cadastrados. */
    int getLevelCount() const { return (int)levels.size(); }
};

#endif
