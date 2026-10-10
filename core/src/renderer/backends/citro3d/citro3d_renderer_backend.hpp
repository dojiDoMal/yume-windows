#ifndef CITRO3D_RENDERER_BACKEND_HPP
#define CITRO3D_RENDERER_BACKEND_HPP

#include "assets/mesh.hpp"
#include "citro3d_gpu_target.hpp"
#include "components/transform.hpp"
#include "graphics_api.hpp"
#include "math/matrix4.hpp"
#include "renderer/renderer_backend.hpp"
#include "scene/world_object.hpp"
#include <citro3d.h>
#include <string>
#include <vector>

/**
 * @brief Backend de renderização para o Nintendo 3DS, usando Citro3D (PICA200).
 *
 * Equivalente ao OpenGLRendererBackend, mas falando a API citro3d/libctru em
 * vez de OpenGL. Baseado no exemplo oficial do cubo rotacionando: configura o
 * C3D_AttrInfo (posição + normal), envia as matrizes como uniforms do vertex
 * shader (C3D_FVUnifMtx4x4) e desenha com C3D_DrawArrays. O frame é delimitado
 * por C3D_FrameBegin/FrameDrawOn/FrameEnd.
 *
 * Diferenças de desenho frente ao PC estão deliberadamente simplificadas nesta
 * primeira versão: sem instancing (o PICA200 não tem SSBO), sem sprites/texto e
 * sem texturas — tudo marcado com @todo. O objetivo é desenhar uma malha (cubo)
 * com transform/câmera corretos.
 *
 * @see RendererBackend, Citro3DMeshBuffer, Citro3DShaderProgram, N3DSDisplayBackend
 */
class Citro3DRendererBackend : public RendererBackend {
  private:
    Citro3DGpuTarget* gpuTarget = nullptr; ///< Alvos de render (dono: DisplayBackend).

    C3D_Mtx projMtx; ///< Projeção atual (recalculada em bindCamera).
    C3D_Mtx viewMtx; ///< View atual (recalculada em bindCamera).
    bool haveCamera = false;

    // Envia projection + modelView (= view * model) como uniforms do vertex
    // shader. O model é montado com as funções Mtx_* do citro3d a partir do
    // Transform do objeto, na mesma convenção (row-major) da view/projeção.
    void applyMatrices(const Transform& transform, void* shaderProgramHandle);

  public:
    ~Citro3DRendererBackend() override;

    unsigned int loadTexture(const std::string& path, uint8_t filterType = 0) override;
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

    void renderSkybox(const Mesh& mesh, unsigned int shaderProgram,
                      unsigned int textureID) override;

    bool init(void* window, DisplayBackend& display) override;
};

#endif // CITRO3D_RENDERER_BACKEND_HPP
