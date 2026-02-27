#ifndef SCENE_FORMAT_HPP
#define SCENE_FORMAT_HPP

#include "color.hpp"
#include "vector3.hpp"
#include <cstdint>

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
    MESH_RENDERER = 0,
    SPRITE_RENDERER = 1,
    TRANSFORM = 2,
    // Futuros: AUDIO_SOURCE, COLLIDER, RIGIDBODY, etc.
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
    };
};

struct GameObjectData {
    uint8_t componentCount;
    ComponentData components[8];
};

struct CompiledScene {
    uint32_t magic = 0x53434E45;
    SceneCameraData camera;
    uint32_t gameObjectCount;
    GameObjectData gameObjects[32];
    uint32_t lightCount;
    LightData lights[32];
};

#endif