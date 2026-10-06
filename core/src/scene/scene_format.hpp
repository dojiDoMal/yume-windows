/**
 * @file scene_format.hpp
 * @brief Formato binário de cena (.scnb) e as structs POD que o compõem.
 *
 * Define os dados puros (sem lógica) que descrevem uma cena em disco: objetos,
 * seus componentes (câmera, luz, renderer de malha/sprite, texto, LOD) e os
 * recursos que cada um referencia. O SceneLoader lê essas structs e as converte
 * em WorldObject; o compilador de cena faz o caminho inverso.
 *
 * As structs usam campos de tamanho fixo (char[], arrays) de propósito, para
 * poderem ser lidas/escritas em bloco como POD.
 */
#ifndef SCENE_FORMAT_HPP
#define SCENE_FORMAT_HPP

#include "color.hpp"
#include "math/vector3.hpp"
#include <cstdint>
#include <vector>

#ifndef MAX_WORLD_OBJECTS
/// @brief Limite de sanidade para o número de objetos ao validar uma cena.
#define MAX_WORLD_OBJECTS 1024
#endif

#ifndef MAX_COMPONENTS_PER_OBJECT
/// @brief Número máximo de componentes por objeto no formato de disco.
#define MAX_COMPONENTS_PER_OBJECT 8
#endif

#ifndef MAX_LOD_LEVELS
/// @brief Número máximo de níveis de LOD por grupo.
#define MAX_LOD_LEVELS 4
#endif

/// @brief Dados de uma luz (tipo, direção, cor e intensidade).
struct LightData {
    uint8_t type; ///< 0=DIRECTIONAL, 1=POINT, 2=SPOT.
    Vector3 direction;
    float color[4];
    float intensity;
    // float position[3];
};

/// @brief Material: caminhos dos shaders de vértice/fragmento e cor base.
struct MaterialData {
    char vertexShaderPath[256];
    char fragmentShaderPath[256];
    ColorRGBA color;
};

/// @brief Textura: caminho, dimensões, fator de escala e filtro.
struct TextureData {
    char path[256];
    float width;
    float height;
    float scaleFactor;
    uint8_t filterType; ///< 0=NEAREST, 1=LINEAR.
};

/// @brief Malha: caminho do arquivo e se usa sombreamento suave.
struct MeshData {
    char path[256];
    bool shadeSmooth;
};

/// @brief Skybox: as 6 texturas do cubemap e o material usado para desenhá-lo.
struct SkyboxData {
    char cubeMapTextures[6][256];
    MaterialData material;
};

/// @brief Dados de câmera no nível da cena (variante com posição em double).
struct SceneCameraData {
    float background_color[4];
    float fov;
    float view_rect[2];
    double position[3];
    bool orthographic;
    float orthoSize;
    bool hasSkybox;
    SkyboxData skybox;
};

/// @brief Identifica o tipo de um ComponentData dentro de um objeto.
enum class ComponentType : uint8_t {
    TRANSFORM = 0,
    MESH_RENDERER = 1,
    SPRITE_RENDERER = 2,
    CAMERA = 3,
    LIGHT = 4,
    TEXT_RENDERER = 5,
    LOD_GROUP = 6,
    SCRIPT = 7
};

/// @brief Dados do componente de script: caminho do arquivo .ys.
struct ScriptComponentData {
    char scriptPath[256]; ///< Caminho do arquivo YumeScript (.ys) a executar.
};

/// @brief Tipo de fonte usada por um renderer de texto. MSDF = multi-channel signed distance field.
enum class FontType : uint8_t { MSDF = 0 };

/// @brief Fonte MSDF: caminhos do JSON do atlas e da textura.
struct MsdfFontData {
    char atlasJsonPath[256];
    char texturePath[256];
};

/// @brief Dados do componente de câmera no formato de disco.
struct CameraComponentData {
    float background_color[4]; ///< Cor de fundo (RGBA).
    float fov;                 ///< Campo de visão, em graus.
    float view_rect[2];        ///< Dimensões da viewport (largura, altura).
    bool orthographic;         ///< @c true para projeção ortográfica.
    float orthoSize;           ///< Tamanho da projeção ortográfica.
    bool hasSkybox;            ///< @c true se @c skybox é válido.
    SkyboxData skybox;         ///< Skybox opcional da câmera.
};

/// @brief Dados do componente de luz no formato de disco.
struct LightComponentData {
    uint8_t lightType; ///< 0=DIRECTIONAL, 1=POINT, 2=SPOT.
    Vector3 direction;
    float color[4];
    float intensity;
};

/// @brief Dados do componente de renderer de texto no formato de disco.
struct TextRendererComponentData {
    FontType fontType;     ///< Tipo da fonte.
    MsdfFontData font;     ///< Dados da fonte MSDF.
    MaterialData material; ///< Material usado para desenhar o texto.
};

/// @brief Um nível de LOD no formato de disco: malha e limiar de tela.
struct LodLevelData {
    MeshData mesh;
    float screenSpaceThreshold;
};

/// @brief Dados do componente de grupo de LOD no formato de disco.
struct LodGroupData {
    uint8_t levelCount;                  ///< Número de níveis em uso.
    LodLevelData levels[MAX_LOD_LEVELS]; ///< Níveis de detalhe.
    MaterialData material;               ///< Material compartilhado pelos níveis.
};

/**
 * @brief Um componente de objeto no formato de disco.
 *
 * O campo @c type diz qual membro da union está preenchido (padrão "tagged
 * union"): leia @c transform quando type==TRANSFORM, @c camera quando
 * type==CAMERA, e assim por diante.
 */
struct ComponentData {
    ComponentType type; ///< Discriminante: indica qual membro da union é válido.
    union {
        struct {
            Vector3 position;
            Vector3 rotation;
            Vector3 scale;
        } transform;

        struct {
            MeshData mesh;
            MaterialData material;
        } meshRenderer;

        struct {
            MaterialData material;
            TextureData texture;
        } spriteRenderer;

        CameraComponentData camera;
        LightComponentData light;
        TextRendererComponentData textRenderer;
        LodGroupData lodGroup;
        ScriptComponentData script;
    };
};

/// @brief Um objeto da cena: transformação inicial e seus componentes.
struct WorldObjectData {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    uint8_t componentCount; ///< Quantos elementos de @c components estão em uso.
    ComponentData components[MAX_COMPONENTS_PER_OBJECT];
};

/**
 * @brief Layout do arquivo de cena em disco (.scnb).
 *
 * O arquivo é: [SceneHeader][worldObjectCount x WorldObjectData]. Só os objetos
 * efetivamente usados são gravados, então o arquivo (e o buffer em RAM) crescem
 * conforme a contagem de objetos, não conforme MAX_WORLD_OBJECTS. Cada
 * WorldObjectData continua sendo um POD de tamanho fixo (strings char[] fixas,
 * array de componentes fixo), o que permite ler/escrever os objetos como um
 * único bloco de @c count * sizeof(WorldObjectData) bytes.
 *
 * MAX_WORLD_OBJECTS não dimensiona mais nada em disco; sobrevive apenas como
 * limite de sanidade ao validar o worldObjectCount lido de um arquivo
 * possivelmente corrompido ou incompatível.
 */
static constexpr uint32_t SCENE_MAGIC = 0x53434E45; ///< Assinatura 'SCNE' no início do arquivo.

/// @brief Cabeçalho do arquivo de cena: assinatura e número de objetos.
struct SceneHeader {
    uint32_t magic;            ///< Deve ser igual a ::SCENE_MAGIC.
    uint32_t worldObjectCount; ///< Quantidade de WorldObjectData que seguem.
};

/**
 * @brief Representação em memória de uma cena carregada.
 *
 * Não é serializável de uma vez (possui um vector); serialize o cabeçalho e o
 * bloco de objetos separadamente.
 */
struct CompiledScene {
    uint32_t worldObjectCount = 0;             ///< Número de objetos.
    std::vector<WorldObjectData> worldObjects; ///< Os objetos da cena.
};

#endif
