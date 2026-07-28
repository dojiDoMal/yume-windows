#ifndef SCENE_FORMAT_HPP
#define SCENE_FORMAT_HPP

#include "color.hpp"
#include "vector3.hpp"
#include <cstdint>

#ifndef MAX_WORLD_OBJECTS
#define MAX_WORLD_OBJECTS 1024
#endif

#ifndef MAX_COMPONENTS_PER_OBJECT
#define MAX_COMPONENTS_PER_OBJECT 8
#endif

struct LightData {
    uint8_t type; // 0=DIRECTIONAL, 1=POINT, 2=SPOT
    Vector3 direction;
    float color[4];
    float intensity;
    // float position[3];
};

struct MaterialData {
    char vertexShaderPath[256];
    char fragmentShaderPath[256];
    ColorRGBA color;
};

struct TextureData {
    char path[256];
    float width;
    float height;
    float scaleFactor;
    uint8_t filterType; // 0=NEAREST, 1=LINEAR
};

struct MeshData {
    char path[256];
    bool shadeSmooth;
};

struct SkyboxData {
    char cubeMapTextures[6][256];
    MaterialData material;
};

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

enum class ComponentType : uint8_t {
    TRANSFORM = 0,
    MESH_RENDERER = 1,
    SPRITE_RENDERER = 2,
    CAMERA = 3,
    LIGHT = 4,
    TEXT_RENDERER = 5
};

enum class FontType : uint8_t {
    MSDF = 0
};

struct MsdfFontData {
    char atlasJsonPath[256];
    char texturePath[256];
};

struct CameraComponentData {
    float background_color[4];
    float fov;
    float view_rect[2];
    bool orthographic;
    float orthoSize;
    bool hasSkybox;
    SkyboxData skybox;
};

struct LightComponentData {
    uint8_t lightType; // 0=DIRECTIONAL, 1=POINT, 2=SPOT
    Vector3 direction;
    float color[4];
    float intensity;
};

struct TextRendererComponentData {
    FontType fontType;
    MsdfFontData font;
    MaterialData material;
};

struct ComponentData {
    ComponentType type;
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
    };
};

struct WorldObjectData {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    uint8_t componentCount;
    ComponentData components[MAX_COMPONENTS_PER_OBJECT];
};

struct CompiledScene {
    uint32_t magic = 0x53434E45;
    uint32_t worldObjectCount;
    WorldObjectData worldObjects[MAX_WORLD_OBJECTS];
};

#endif
