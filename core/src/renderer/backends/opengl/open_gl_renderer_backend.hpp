#ifndef OPEN_GL_RENDERER_BACKEND_HPP
#define OPEN_GL_RENDERER_BACKEND_HPP

#include "../../../graphics_api.hpp"
#include "../../../mesh.hpp"
#include "../../../world_object.hpp"
#include "../../renderer_backend.hpp"
#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <vector>

class OpenGLRendererBackend : public RendererBackend {
  private:
    GLuint instanceSSBO = 0;
    GLuint spriteVAO = 0;
    GLuint spriteVBO = 0;
    GLuint matricesUBO = 0;
    GLuint materialDataUBO = 0;
    GLuint lightDataUBO = 0;
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
        std::vector<glm::mat4> models;
        const Mesh* mesh = nullptr;
        Material* material = nullptr;
    };
    std::unordered_map<RenderKey, InstanceGroup, RenderKeyHash> instanceGroups;
    std::vector<WorldObject*> nonInstancedObjects;

    void initSpriteQuad();

  public:
    ~OpenGLRendererBackend();

    unsigned int loadTexture(const std::string& path, uint8_t filterType = 0) override;
    void drawSprite(const Sprite& sprite) override;
    bool init() override;
    void present(SDL_Window* window) override;
    bool initWindowContext() override;
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

    unsigned int getRequiredWindowFlags() const override;
    bool init(SDL_Window* window) override;

    bool initText(const FontAtlas& atlas, unsigned int textureID, const std::string& vertPath,
                  const std::string& fragPath) override;
    void drawText(const std::string& text, float x, float y, float scale, glm::vec4 color,
                  int screenWidth, int screenHeight) override;
};

#endif
