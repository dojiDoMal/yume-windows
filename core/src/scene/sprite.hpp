#ifndef SPRITE_HPP
#define SPRITE_HPP

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief Imagem 2D desenhada no mundo: uma textura (ou um recorte dela).
 *
 * Representa um quad texturizado. Guarda o id da textura (handle de GPU), as
 * dimensões em unidades de mundo e, opcionalmente, uma tabela de @ref Frame
 * carregada de um sprite sheet (arquivo @c .ssm). Quando há frames, o sprite
 * mostra apenas o recorte do frame atual (via UV offset/scale) em vez da
 * textura inteira; sem frames, o comportamento é o legado (textura cheia,
 * UV 0..1).
 */
class Sprite {
  public:
    /**
     * @brief Recorte retangular da textura (um "frame" do sprite sheet).
     *
     * UVs normalizados [0,1] no espaço de textura do OpenGL (origem no canto
     * inferior esquerdo). @c u0,v0 é o canto inferior-esquerdo e @c u1,v1 o
     * superior-direito. @c pixelWidth/pixelHeight guardam o tamanho do recorte
     * em pixels, usado para dimensionar o quad proporcionalmente.
     */
    struct Frame {
        float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
        float pixelWidth = 1.0f, pixelHeight = 1.0f;
    };

  private:
    unsigned int textureID = 0; ///< Handle da textura na GPU.
    float width = 1.0f;         ///< Largura do sprite (unidades de mundo).
    float height = 1.0f;        ///< Altura do sprite (unidades de mundo).

    /// Grupos de animação nomeados (ex.: "front", "side"), cada um com N frames.
    std::unordered_map<std::string, std::vector<Frame>> groups;
    /// Ordem de inserção dos grupos (para resolver "grupo inicial" de forma estável).
    std::vector<std::string> groupOrder;

    bool hasRegions = false;    ///< @c true se frames foram carregados de um .ssm.
    Frame current;              ///< Recorte atualmente exibido.
    bool flipX = false;         ///< Espelha horizontalmente o recorte atual.
    float scaleFactor = 1.0f;   ///< Pixels -> unidades de mundo (do .scn).

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

    /** @brief Fator pixels->mundo usado ao trocar de frame. */
    void setScaleFactor(float f) { scaleFactor = f; }

    /**
     * @brief Registra os frames de um grupo (recortes já em UV normalizado).
     *
     * Chamado pelo loader ao interpretar o .ssm. A ordem de registro define o
     * grupo "inicial" (o primeiro) usado para dimensionar o quad por padrão.
     */
    void addGroup(const std::string& name, std::vector<Frame> frames) {
        if (groups.find(name) == groups.end())
            groupOrder.push_back(name);
        groups[name] = std::move(frames);
        hasRegions = true;
    }

    /** @brief Indica se o sprite usa recortes (sprite sheet) em vez da textura cheia. */
    bool hasFrames() const { return hasRegions; }

    /** @brief Número de frames de um grupo; 0 se o grupo não existe. */
    size_t frameCount(const std::string& group) const {
        auto it = groups.find(group);
        return it == groups.end() ? 0 : it->second.size();
    }

    /** @brief Indica se existe um grupo com esse nome. */
    bool hasGroup(const std::string& group) const {
        return groups.find(group) != groups.end();
    }

    /**
     * @brief Seleciona o frame @p index do grupo @p group como recorte atual.
     *
     * Ajusta o tamanho do quad (width/height) ao tamanho do frame em pixels,
     * aplicando o scaleFactor. Índices fora do intervalo são ignorados (mantém
     * o frame anterior). Retorna @c true se o frame foi aplicado.
     */
    bool setFrame(const std::string& group, size_t index) {
        auto it = groups.find(group);
        if (it == groups.end() || index >= it->second.size())
            return false;
        current = it->second[index];
        width = current.pixelWidth * scaleFactor;
        height = current.pixelHeight * scaleFactor;
        return true;
    }

    /** @brief Define se o recorte atual é espelhado horizontalmente. */
    void setFlipX(bool v) { flipX = v; }
    /** @brief Indica se o recorte atual está espelhado horizontalmente. */
    bool getFlipX() const { return flipX; }

    /** @brief Recorte atualmente exibido (UVs normalizados + tamanho em px). */
    const Frame& getCurrentFrame() const { return current; }

    /** @brief Nome do primeiro grupo registrado, ou string vazia se não há. */
    const std::string& firstGroup() const {
        static const std::string empty;
        return groupOrder.empty() ? empty : groupOrder.front();
    }
};

#endif
