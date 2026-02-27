#ifndef OPEN_GL_RENDERER_BACKEND_HPP
#define OPEN_GL_RENDERER_BACKEND_HPP

#include "../../../graphics_api.hpp"
#include "../../../mesh.hpp"
#include "../../renderer_backend.hpp"
#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <vector>


class OpenGLRendererBackend : public RendererBackend {
  private:
    GLuint spriteVAO = 0;
    GLuint spriteVBO = 0;
    GLuint matricesUBO = 0;
    GLuint materialDataUBO = 0;
    GLuint lightDataUBO = 0;
    std::unordered_map<std::string, GLuint> uniformBindings;

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

    void renderGameObjects(std::vector<GameObject*>* gameObjects,
                           std::vector<Light>* lights) override;

    // Skybox management
    void deleteCubemapTexture(unsigned int textureID);
    void renderSkybox(const Mesh& mesh, unsigned int shaderProgram,
                      unsigned int textureID) override;

    unsigned int getRequiredWindowFlags() const override;
    bool init(SDL_Window* window) override;
};

#endif // OPENGLRENDERERBACKEND_HPP
