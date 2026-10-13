#ifndef OPEN_GL_RENDERER_BACKEND_HPP
#define OPEN_GL_RENDERER_BACKEND_HPP

#include "assets/mesh.hpp"
#include "graphics_api.hpp"
#include "math/matrix4.hpp"
#include "renderer/renderer_backend.hpp"
#include "scene/world_object.hpp"
#ifdef __SWITCH__
#include <glad/glad.h>
#else
#include <GL/glew.h>
#endif
#include <string>
#include <unordered_map>
#include <vector>

class OpenGLRendererBackend : public RendererBackend {
  private:
    void* glContext = nullptr; ///< Contexto GL opaco, criado pelo DisplayBackend.
    GLuint instanceSSBO = 0;
    GLuint spriteVAO = 0;
    GLuint spriteVBO = 0;
    GLuint matricesUBO = 0;
    GLuint materialDataUBO = 0;
    GLuint lightDataUBO = 0;
    GLuint skyboxUBO = 0;

    // Texturas 1x1 de fallback, usadas quando um material não fornece um mapa:
    // branco (albedo/roughness/metallic) e normal plana (0.5,0.5,1). Assim o
    // Sample() no shader PBR é sempre válido mesmo sem a textura.
    GLuint defaultWhiteTex = 0;
    GLuint defaultNormalTex = 0;
    GLuint defaultBlackTex = 0;
    std::unordered_map<std::string, GLuint> uniformBindings;

    // Adicionar nos campos privados:
    const FontAtlas* textAtlas = nullptr;
    unsigned int textTextureID = 0;
    GLuint textShaderProgram = 0;
    GLuint textVAO = 0;
    GLuint textVBO = 0;
    GLuint textUBOProjection = 0;
    GLuint textUBOColor = 0;

    GLuint compileTextShader(const std::string& path, GLenum type);

    struct RenderKey {
        GLuint vao;
        GLuint shader;
        bool operator==(const RenderKey& o) const { return vao == o.vao && shader == o.shader; }
    };
    struct RenderKeyHash {
        size_t operator()(const RenderKey& k) const {
            return std::hash<GLuint>()(k.vao) ^ (std::hash<GLuint>()(k.shader) << 16);
        }
    };
    struct InstanceGroup {
        std::vector<Matrix4> models;
        const Mesh* mesh = nullptr;
        Material* material = nullptr;
    };
    std::unordered_map<RenderKey, InstanceGroup, RenderKeyHash> instanceGroups;
    std::vector<WorldObject*> nonInstancedObjects;

    void initSpriteQuad();

    /**
     * @brief Vincula as texturas PBR de um material às unidades de textura 0..3
     *        e seta os samplers correspondentes no programa.
     * @param material Material com os handles de textura (0 = ausente).
     * @param program  Handle GL do programa de shader atualmente em uso.
     */
    void applyMaterialTextures(Material* material, GLuint program);

  public:
    ~OpenGLRendererBackend();

    unsigned int loadTexture(const std::string& path, uint8_t filterType = 0,
                             bool srgb = false) override;
    void drawSprite(const Sprite& sprite) override;
    bool init() override;
    void present(void* window) override;
    void bindCamera(Camera* camera) override;
    void applyMaterial(Material* material) override;
    void setBufferDataImpl(const std::string& name, const void* data, size_t size) override;
    void clear(Camera* camera) override;
    void draw(const Mesh&) override;
    void setUniforms(ShaderProgram* shaderProgram) override;
    unsigned int createCubemapTexture(const std::vector<std::string>& faces) override;
    std::unique_ptr<ShaderProgram> createShaderProgram() override;
    std::unique_ptr<ShaderCompiler> createShaderCompiler() override;
    std::unique_ptr<MeshBuffer> createMeshBuffer() override;
    void onCameraSet() override;
    GraphicsAPI getGraphicsAPI() const override;
    std::string getShaderExtension() const override;

    void renderWorldObjects(const std::vector<WorldObject*>& objects,
                            const std::vector<Light*>& lights) override;

    void deleteCubemapTexture(unsigned int textureID);
    void renderSkybox(const Mesh& mesh, unsigned int shaderProgram,
                      unsigned int textureID) override;

    bool init(void* window, DisplayBackend& display) override;

    bool initText(const FontAtlas& atlas, unsigned int textureID, const std::string& vertPath,
                  const std::string& fragPath) override;
    void drawText(const std::string& text, float x, float y, float scale, ColorRGBA color,
                  int screenWidth, int screenHeight) override;
};

#endif
