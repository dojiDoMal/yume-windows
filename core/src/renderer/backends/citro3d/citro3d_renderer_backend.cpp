#define CLASS_NAME "Citro3DRendererBackend"
#include "log_macros.hpp"

#include "citro3d_renderer_backend.hpp"

#include "assets/material.hpp"
#include "assets/mesh_buffer_factory.hpp"
#include "assets/shader_compiler_factory.hpp"
#include "assets/shader_program_factory.hpp"
#include "citro3d_mesh_buffer.hpp"
#include "color.hpp"
#include "components/light.hpp"
#include "components/mesh_renderer.hpp"
#include "math/math.hpp"

#include <cmath>

namespace {

/// Cor de limpeza quando não há câmera. ColorRGBA (0..1) -> RGBA8 do 3DS.
constexpr u32 kFallbackClearColor = 0x000000FF;

/// Converte uma ColorRGBA (componentes 0..1) para o inteiro RGBA8 que o
/// C3D_RenderTargetClear espera (0xRRGGBBAA).
u32 toRGBA8(const ColorRGBA& c) {
    auto clamp8 = [](float v) -> u32 {
        if (v < 0.0f)
            v = 0.0f;
        if (v > 1.0f)
            v = 1.0f;
        return static_cast<u32>(v * 255.0f + 0.5f);
    };
    return (clamp8(c.r) << 24) | (clamp8(c.g) << 16) | (clamp8(c.b) << 8) | clamp8(c.a);
}

} // namespace

GraphicsAPI Citro3DRendererBackend::getGraphicsAPI() const { return GraphicsAPI::CITRO3D; }

std::string Citro3DRendererBackend::getShaderExtension() const {
    // Binário de shader PICA200 montado pelo picasso em build-time.
    // TODO: adicionar o passo picasso (*.pica -> *.shbin) ao pipeline do CMake
    //       para o 3DS; hoje o pipeline só gera GLSL (dxc/spirv-cross).
    return ".shbin";
}

Citro3DRendererBackend::~Citro3DRendererBackend() {
    // Os render targets e o próprio C3D (C3D_Init/C3D_Fini, gfxInitDefault/
    // gfxExit) são criados e destruídos pela camada de display/multimídia do
    // 3DS, que é dona do ciclo de vida da GPU. Aqui não há recursos próprios.
}

std::unique_ptr<ShaderProgram> Citro3DRendererBackend::createShaderProgram() {
    return ShaderProgramFactory::create(getGraphicsAPI());
}

std::unique_ptr<MeshBuffer> Citro3DRendererBackend::createMeshBuffer() {
    return MeshBufferFactory::create(getGraphicsAPI());
}

std::unique_ptr<ShaderCompiler> Citro3DRendererBackend::createShaderCompiler() {
    return ShaderCompilerFactory::create(getGraphicsAPI());
}

bool Citro3DRendererBackend::init(void* window, DisplayBackend& display) {
    if (!window) {
        LOG_ERROR("Window (GPU target) is null!");
        return false;
    }
    displayBackend = &display;

    // No 3DS o "window" é o Citro3DGpuTarget criado pelo N3DSDisplayBackend, que
    // já fez gfxInitDefault/C3D_Init e C3D_RenderTargetCreate. Guardamos para
    // usar em clear()/present().
    gpuTarget = static_cast<Citro3DGpuTarget*>(window);
    if (!gpuTarget->top) {
        LOG_ERROR("GPU target has no top render target");
        return false;
    }

    return init();
}

bool Citro3DRendererBackend::init() {
    // Teste de profundidade. No PICA200, Mtx_PerspTilt gera clip-space z em
    // [-1, 0] (near=0, far=-1) e o C3D_DepthMap default (-1, 0) mapeia isso para
    // o depth buffer de forma que o fragmento MAIS PRÓXIMO tem o MAIOR valor de
    // depth. Por isso a função correta é GPU_GREATER, não GPU_LESS -- é o mesmo
    // default que C3D_Init usa. Com GPU_LESS, o clear de depth em 0 rejeitava
    // todos os fragmentos e o cubo ficava invisível.
    C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);

    // Culling de back-face com winding CCW (default do citro3d). O .obj do cubo
    // (exportado do Blender) tem faces CCW; sem culling, as faces de trás eram
    // desenhadas por cima das da frente, deixando o cubo com aparência
    // "invertida"/chapado. GPU_CULL_BACK_CCW descarta as faces de trás.
    C3D_CullFace(GPU_CULL_BACK_CCW);

    // Layout de atributos de vértice: v0 = posição (vec3), v1 = normal (vec3).
    // Precisa casar com o stride do Citro3DMeshBuffer (6 floats) e com os inputs
    // do vertex shader .shbin.
    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0 = position
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 3); // v1 = normal

    // TexEnv: sem fragment shader programável, o estágio de fragmento é
    // configurado aqui. Como o cubo ainda não tem textura, a cor final é
    // simplesmente a cor do vértice (PRIMARY_COLOR), que o vertex shader
    // escreveu em 'outclr'. Sem isto o fragmento não emite a cor do vértice e o
    // cubo sai invisível/preto mesmo sendo desenhado.
    // TODO (texturas): trocar para modular TEXTURE0 * PRIMARY_COLOR quando
    //       houver textura.
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    return true;
}

void Citro3DRendererBackend::onCameraSet() {}

void Citro3DRendererBackend::clear(Camera* camera) {
    if (!gpuTarget || !gpuTarget->top)
        return;

    const u32 clearColor = camera ? toRGBA8(camera->getBackgroundColor()) : kFallbackClearColor;

    // A engine chama clear() no início do frame e present() no fim. Mapeamos
    // isso para o ciclo de frame do citro3d: begin + clear + draw-on aqui; o
    // C3D_FrameEnd correspondente fica em present().
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C3D_RenderTargetClear(gpuTarget->top, C3D_CLEAR_ALL, clearColor, 0);
    C3D_FrameDrawOn(gpuTarget->top);

    // Viewport. No PICA200 a viewport (e o scissor implícito) ficam indefinidos
    // até C3D_SetViewport ser chamado -- sem isto a geometria é transformada e
    // submetida (C3D_DrawArrays roda), mas rasteriza para uma viewport
    // degenerada e NADA aparece na tela. Precisa ser definido a cada frame,
    // depois de C3D_FrameDrawOn. A tela de cima é desenhada "de lado", então as
    // dimensões no espaço girado da GPU são 400x240 (as mesmas usadas para criar
    // o render target: 240 de "largura", 400 de "altura").
    C3D_SetViewport(0, 0, static_cast<u32>(gpuTarget->top->frameBuf.width),
                    static_cast<u32>(gpuTarget->top->frameBuf.height));
}

void Citro3DRendererBackend::present(void* window) {
    (void)window; // O alvo já é conhecido (gpuTarget); fecha o frame começado em clear().
    C3D_FrameEnd(0);
}

void Citro3DRendererBackend::bindCamera(Camera* camera) {
    if (!camera) {
        LOG_ERROR("Camera is null");
        return;
    }

    WorldObject* cameraObj = camera->getOwner();
    if (!cameraObj) {
        LOG_ERROR("Camera has no owner WorldObject");
        return;
    }

    const auto camPos = cameraObj->getTransform().getPosition();
    const auto camRot = cameraObj->getTransform().getRotation();

    // View matrix construída a partir do MESMO vetor forward (yaw/pitch) que os
    // demais backends (OpenGL/Vulkan/D3D12/WebGL) usam, seguido de um lookAt.
    //
    // A versão anterior montava a view encadeando Mtx_RotateX/Y/Z com os ângulos
    // negados mas na ordem direta (X,Y,Z). Isso NÃO é a inversa da rotação da
    // câmera: a transform do engine aplica as rotações como Rx*Ry*Rz, cuja
    // inversa é Rz^-1*Ry^-1*Rx^-1 (ordem reversa). O resultado era um forward com
    // o Y invertido -- a câmera apontava para baixo em vez de para o alvo, e o
    // cubo caía bem acima do topo do frustum (view-space y ~= 4.5 contra um
    // limite de ~2.8 no FOV de 60°), ficando invisível.
    //
    // Convenção do engine (ver open_gl_renderer_backend.cpp): pitch = rot.x,
    // yaw = rot.y, forward padrão = -Z.
    const float pitch = Yume::Math::radians(camRot.x);
    const float yaw = Yume::Math::radians(camRot.y);

    Vector3 forward{std::cos(pitch) * std::sin(yaw), std::sin(pitch),
                    std::cos(pitch) * std::cos(yaw)};
    forward = Yume::Math::normalize(forward);
    forward = forward * -1.0f; // forward padrão é -Z

    const Vector3 target{camPos.x + forward.x, camPos.y + forward.y, camPos.z + forward.z};

    // isLeftHanded=false para casar com a projeção RH (Mtx_PerspTilt abaixo
    // também usa isLeftHanded=false): câmera olhando para -Z.
    Mtx_LookAt(&viewMtx, FVec3_New(camPos.x, camPos.y, camPos.z),
               FVec3_New(target.x, target.y, target.z), FVec3_New(0.0f, 1.0f, 0.0f), false);

    // Projeção. A tela de cima do 3DS é desenhada "de lado" (framebuffer girado
    // 90°), por isso o idiomático é Mtx_PerspTilt, que já embute essa rotação.
    if (camera->isOrthographic()) {
        // TODO: Mtx_OrthoTilt quando a engine precisar de câmera ortográfica no
        //       3DS; por ora caímos na perspectiva para manter um caminho único.
    }
    Mtx_PerspTilt(&projMtx, Yume::Math::radians(camera->getFov()), C3D_AspectRatioTop,
                  camera->getNearDistance(), camera->getFarDistance(), false);

    haveCamera = true;
}

void Citro3DRendererBackend::applyMatrices(const Transform& transform, void* shaderProgramHandle) {
    auto* program = static_cast<shaderProgram_s*>(shaderProgramHandle);
    if (!program || !program->vertexShader)
        return;

    int uProjection = shaderInstanceGetUniformLocation(program->vertexShader, "projection");
    int uModelView = shaderInstanceGetUniformLocation(program->vertexShader, "modelView");

    if (uProjection >= 0)
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProjection, &projMtx);

    if (uModelView >= 0) {
        // Monta a model matrix com as funções do citro3d (mesma convenção da
        // view/projeção), a partir do Transform do objeto: escala -> rotação
        // (X,Y,Z) -> translação. Depois modelView = view * model.
        const auto pos = transform.getPosition();
        const auto rot = transform.getRotation();
        const auto scl = transform.getScale();

        C3D_Mtx modelMtx;
        Mtx_Identity(&modelMtx);
        Mtx_Translate(&modelMtx, pos.x, pos.y, pos.z, true);
        Mtx_RotateX(&modelMtx, Yume::Math::radians(rot.x), true);
        Mtx_RotateY(&modelMtx, Yume::Math::radians(rot.y), true);
        Mtx_RotateZ(&modelMtx, Yume::Math::radians(rot.z), true);
        Mtx_Scale(&modelMtx, scl.x, scl.y, scl.z);

        C3D_Mtx modelView;
        Mtx_Multiply(&modelView, &viewMtx, &modelMtx);
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uModelView, &modelView);
    }
}

void Citro3DRendererBackend::setUniforms(ShaderProgram* shaderProgram) {
    if (!shaderProgram || !shaderProgram->isValid())
        return;
    shaderProgram->use();
}

void Citro3DRendererBackend::setBufferDataImpl(const std::string& name, const void* data,
                                               size_t size) {
    // No PICA200 não há Uniform Buffer Objects: as matrizes são enviadas por
    // uniform do vertex shader em applyMatrices(), durante o desenho de cada
    // objeto. Blocos de material/luz ainda não são consumidos pelo shader
    // mínimo do cubo.
    (void)name;
    (void)data;
    (void)size;
    // TODO: traduzir "LightData"/"MaterialData" para C3D_FVUnifSet quando o
    //       shader de produção existir.
}

void Citro3DRendererBackend::applyMaterial(Material* material) {
    if (!material)
        return;
    auto* program = material->getShaderProgram();
    if (!program || !program->isValid())
        return;
    setUniforms(program);
}

void Citro3DRendererBackend::draw(const Mesh& mesh) {
    auto* buffer = static_cast<Citro3DMeshBuffer*>(mesh.getMeshBufferHandle());
    if (!buffer || !buffer->getVertexData())
        return;

    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    // Um único buffer intercalado: 2 atributos (v0 posição, v1 normal), ordem
    // 0x10 significa "atributo 0 depois atributo 1". O stride vem do buffer.
    BufInfo_Add(bufInfo, buffer->getVertexData(), buffer->getStride(), 2, 0x10);

    C3D_DrawArrays(GPU_TRIANGLES, 0, buffer->getVertexCount());
}

void Citro3DRendererBackend::renderWorldObjects(const std::vector<WorldObject*>& objects,
                                                const std::vector<Light*>& lights) {
    drawnObjects = 0;
    drawnVerts = 0;
    drawnTris = 0;

    // Vetor "para a luz" em VIEW space, para o shader (uniform toLightView).
    // Convenção idêntica à dos demais backends (flat.pxs):
    //   diffuse = max(dot(N, -normalize(lightDir)), 0.4)
    // O shader de referência calcula em world space; aqui a normal está em view
    // space (modelView), então transformamos a direção da luz pela ROTAÇÃO da
    // view (as 3 primeiras linhas da viewMtx, componentes x/y/z; a .w é
    // translação e não se aplica a um vetor direção). Como a view é ortonormal,
    // dot(N_view, L_view) == dot(N_world, L_world), dando o MESMO N·L.
    // Default: luz vinda de cima (-Y) caso a cena não tenha luz direcional.
    float toLightView[3] = {0.0f, 1.0f, 0.0f};
    for (auto* light : lights) {
        if (!light || light->getType() != LightType::DIRECTIONAL)
            continue;
        const Vector3 d = light->getDirection(); // direção que a luz viaja (world)
        // L_view = view_rotation * d  (row-major: usa .x/.y/.z de cada linha)
        float lx = viewMtx.r[0].x * d.x + viewMtx.r[0].y * d.y + viewMtx.r[0].z * d.z;
        float ly = viewMtx.r[1].x * d.x + viewMtx.r[1].y * d.y + viewMtx.r[1].z * d.z;
        float lz = viewMtx.r[2].x * d.x + viewMtx.r[2].y * d.y + viewMtx.r[2].z * d.z;
        // toLight = -normalize(L_view)
        float len = std::sqrt(lx * lx + ly * ly + lz * lz);
        if (len > 1e-6f) {
            toLightView[0] = -lx / len;
            toLightView[1] = -ly / len;
            toLightView[2] = -lz / len;
        }
        break; // primeira luz direcional, como os outros backends (lights[0])
    }

    if (!haveCamera)
        return;

    // Caminho único e simples: sem instancing (o PICA200 não tem SSBO) e sem
    // sprites/texto nesta primeira versão. Cada objeto com malha e material é
    // desenhado individualmente, enviando sua model matrix como uniform.
    for (auto* obj : objects) {
        if (!obj->hasMesh())
            continue;

        auto* meshRenderer = obj->getComponent<MeshRenderer>();
        if (!meshRenderer || !meshRenderer->getMaterial())
            continue;

        auto* mesh = obj->getMesh();
        auto* mat = meshRenderer->getMaterial();
        auto* program = mat->getShaderProgramSingle();
        if (!mesh || !program || !program->isValid())
            continue;

        program->use();
        applyMatrices(obj->getTransform(), program->getHandle());

        // Envia a cor base do material (matColor) e o vetor para a luz em view
        // space (toLightView) como uniforms do vertex shader. O shader modula a
        // iluminação (max(N·toLightView, 0.4)) pela cor do material, igualando a
        // aparência dos demais backends.
        {
            auto* sp = static_cast<shaderProgram_s*>(program->getHandle());
            if (sp && sp->vertexShader) {
                int uMatColor = shaderInstanceGetUniformLocation(sp->vertexShader, "matColor");
                if (uMatColor >= 0) {
                    const ColorRGBA c = mat->getBaseColor();
                    C3D_FVUnifSet(GPU_VERTEX_SHADER, uMatColor, c.r, c.g, c.b, c.a);
                }
                int uToLight = shaderInstanceGetUniformLocation(sp->vertexShader, "toLightView");
                if (uToLight >= 0) {
                    C3D_FVUnifSet(GPU_VERTEX_SHADER, uToLight, toLightView[0], toLightView[1],
                                  toLightView[2], 0.0f);
                }
            }
        }

        auto* buffer = static_cast<Citro3DMeshBuffer*>(mesh->getMeshBufferHandle());
        if (!buffer || !buffer->getVertexData())
            continue;

        C3D_BufInfo* bufInfo = C3D_GetBufInfo();
        BufInfo_Init(bufInfo);
        BufInfo_Add(bufInfo, buffer->getVertexData(), buffer->getStride(), 2, 0x10);

        const int vertCount = buffer->getVertexCount();
        C3D_DrawArrays(GPU_TRIANGLES, 0, vertCount);

        drawnObjects++;
        drawnVerts += vertCount;
        drawnTris += vertCount / 3;
    }

    // TODO (sprites/texto): o backend OpenGL também desenha sprites e texto MSDF
    //       aqui. No 3DS isso depende de texturas (TexEnv) e de um shader de
    //       sprite próprios, deixados para uma etapa futura.
}

// ---------------------------------------------------------------------------
// Recursos ainda não suportados no backend 3DS. Mantidos como stubs seguros
// para cumprir a interface RendererBackend sem quebrar o build/execução.
// ---------------------------------------------------------------------------

unsigned int Citro3DRendererBackend::loadTexture(const std::string& path, uint8_t filterType) {
    (void)path;
    (void)filterType;
    // TODO (texturas): carregar .t3x via Tex3DS_TextureImport e devolver um id.
    LOG_WARN("loadTexture not implemented on Citro3D backend yet");
    return 0;
}

void Citro3DRendererBackend::drawSprite(const Sprite& sprite) {
    (void)sprite;
    // TODO (sprites): requer textura + TexEnv + quad; não implementado ainda.
}

unsigned int Citro3DRendererBackend::createCubemapTexture(const std::vector<std::string>& faces) {
    (void)faces;
    // TODO (skybox/cubemap): sem suporte a texturas ainda.
    return 0;
}

void Citro3DRendererBackend::renderSkybox(const Mesh& mesh, unsigned int shaderProgram,
                                          unsigned int textureID) {
    (void)mesh;
    (void)shaderProgram;
    (void)textureID;
    // TODO (skybox): depende de cubemap/texturas.
}
