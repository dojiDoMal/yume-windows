#ifndef D3D12_RENDERER_BACKEND_HPP
#define D3D12_RENDERER_BACKEND_HPP

#include "../../../mesh.hpp"
#include "../../../world_object.hpp"
#include "../../renderer_backend.hpp"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

class D3D12RendererBackend : public RendererBackend {
  private:
    ID3D12Device* device = nullptr;
    ID3D12CommandQueue* commandQueue = nullptr;
    IDXGISwapChain3* swapChain = nullptr;
    ID3D12DescriptorHeap* rtvHeap = nullptr;
    ID3D12DescriptorHeap* dsvHeap = nullptr;
    ID3D12Resource* renderTargets[2] = {};
    ID3D12Resource* depthStencil = nullptr;
    ID3D12CommandAllocator* commandAllocator = nullptr;
    ID3D12GraphicsCommandList* commandList = nullptr;
    ID3D12Fence* fence = nullptr;
    UINT64 fenceValue = 0;
    HANDLE fenceEvent = nullptr;
    UINT rtvDescriptorSize = 0;
    UINT frameIndex = 0;

    ID3D12Resource* constantBuffers[3] = {};
    void* constantBufferData[3] = {};
    std::unordered_map<std::string, int> uniformBindings;

    // Per-instance model-matrix buffer (upload heap), bound as SRV t0 so the
    // vertex shader can index instanceModels[SV_InstanceID].
    //
    // Because D3D12 records draws into a command list that only executes at
    // present time, every draw must reference a DISTINCT region of this buffer:
    // if two draws shared the same address, both would read whatever matrices
    // were written last. So we treat the buffer as a per-frame linear arena:
    // each group's matrices are appended at a growing cursor, and each draw
    // binds the SRV at its own offset. The cursor resets every frame.
    ID3D12Resource* instanceBuffer = nullptr;
    void* instanceBufferData = nullptr;
    size_t instanceBufferCapacity = 0; // capacity in number of glm::mat4
    size_t instanceBufferCursor = 0;   // next free slot (matrices), per frame

    // Instancing groups, mirroring the OpenGL backend: objects sharing the same
    // mesh + shader program are batched into a single DrawInstanced call.
    struct RenderKey {
        void* mesh;
        void* pso;
        bool operator==(const RenderKey& o) const { return mesh == o.mesh && pso == o.pso; }
    };
    struct RenderKeyHash {
        size_t operator()(const RenderKey& k) const {
            return std::hash<void*>()(k.mesh) ^ (std::hash<void*>()(k.pso) << 1);
        }
    };
    struct InstanceGroup {
        std::vector<glm::mat4> models;
        const Mesh* mesh = nullptr;
        Material* material = nullptr;
    };
    std::unordered_map<RenderKey, InstanceGroup, RenderKeyHash> instanceGroups;
    std::vector<WorldObject*> nonInstancedObjects;

    // ---- Texture storage -----------------------------------------------------
    // loadTexture returns an index into these parallel arrays; the SRV lives in
    // srvHeap at that slot. The MSDF text texture is one such entry.
    struct TextureEntry {
        ID3D12Resource* resource = nullptr;
        UINT srvIndex = 0; // slot in srvHeap
    };
    std::vector<TextureEntry> textures;
    ID3D12DescriptorHeap* srvHeap = nullptr; // shader-visible CBV/SRV/UAV heap
    UINT srvDescriptorSize = 0;
    UINT srvHeapCapacity = 0;
    UINT srvHeapUsed = 0;

    // Uploads pixel data into a new DEFAULT-heap texture and creates an SRV for
    // it in srvHeap. Returns the texture index (into `textures`), or 0 on error.
    unsigned int createTexture2D(const unsigned char* pixels, int width, int height, int channels,
                                 uint8_t filterType);
    bool ensureSrvHeap(UINT capacity);

    // ---- Text rendering (MSDF) ----------------------------------------------
    const FontAtlas* textAtlas = nullptr;
    unsigned int textTextureID = 0; // index into `textures`
    ID3D12RootSignature* textRootSignature = nullptr;
    ID3D12PipelineState* textPipelineState = nullptr;

    // Per-frame dynamic vertex buffer for glyph quads (upload heap, linear
    // arena reset each frame, same rationale as instanceBuffer).
    ID3D12Resource* textVertexBuffer = nullptr;
    void* textVertexBufferData = nullptr;
    size_t textVertexCapacity = 0; // capacity in floats
    size_t textVertexCursor = 0;   // next free float, per frame

    // Per-frame constant buffers for text (b4 projection, b5 color). Sub-region
    // per draw so multiple drawText calls in a frame don't clobber each other.
    ID3D12Resource* textCB = nullptr;
    void* textCBData = nullptr;
    UINT textCBCursor = 0; // next free 256-byte slot, per frame
    UINT textCBSlots = 0;

    bool createTextPipeline(const std::string& vertPath, const std::string& fragPath);

    // Reserves the instance buffer capacity for `totalMatrices` and resets the
    // per-frame cursor. Must be called once per frame before any
    // appendInstanceData call, since reallocation mid-frame would invalidate GPU
    // addresses already recorded.
    void beginInstanceFrame(size_t totalMatrices);

    // Appends `count` matrices at the current cursor and returns the GPU
    // virtual address of that region (for SetGraphicsRootShaderResourceView).
    D3D12_GPU_VIRTUAL_ADDRESS appendInstanceData(const glm::mat4* models, size_t count);

    // Resets the text per-frame arenas (vertex + constant buffer cursors).
    void beginTextFrame();

    bool createDevice();
    bool createCommandQueue();
    bool createSwapChain(void* hwnd);
    bool createDescriptorHeaps();
    bool createRenderTargets();
    bool createDepthStencil();
    bool createCommandObjects();
    bool createFence();
    bool createConstantBuffers();
    void waitForGPU();

  public:
    ~D3D12RendererBackend();

    unsigned int loadTexture(const std::string& path, uint8_t filterType = 0) override;
    void drawSprite(const Sprite& sprite) override;
    bool init() override;
    bool initWindowContext() override;
    void bindCamera(Camera* camera) override;
    void applyMaterial(Material* material) override;
    void renderWorldObjects(const std::vector<WorldObject*>& objects,
                            const std::vector<Light*>& lights) override;
    void clear(Camera* camera) override;
    void draw(const Mesh&) override;
    void setUniforms(ShaderProgram* shaderProgram) override;
    void onCameraSet() override;
    GraphicsAPI getGraphicsAPI() const override;
    std::string getShaderExtension() const override;
    void renderSkybox(const Mesh& mesh, unsigned int shaderProgram,
                      unsigned int textureID) override;
    void setBufferDataImpl(const std::string& name, const void* data, size_t size) override;
    unsigned int createCubemapTexture(const std::vector<std::string>& faces) override;
    std::unique_ptr<ShaderProgram> createShaderProgram() override;
    std::unique_ptr<ShaderCompiler> createShaderCompiler() override;
    std::unique_ptr<MeshBuffer> createMeshBuffer() override;
    void present(SDL_Window* window) override;

    ID3D12Device* getDevice() const { return device; }
    ID3D12GraphicsCommandList* getCommandList() const { return commandList; }

    // Render-target VIEW format. With the flip-model swapchain the buffer stays
    // UNORM; sRGB output is achieved by viewing it through an _SRGB RTV. The PSO
    // must declare the same RTV format, so shader programs query this.
    DXGI_FORMAT getRtvFormat() const {
        return srgbEnabled ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    void updateConstantBuffer(int binding, const void* data, size_t size);
    void setHwnd(void* hwnd) { this->hwnd = hwnd; }
    unsigned int getRequiredWindowFlags() const override;
    bool init(SDL_Window* window) override;

    bool initText(const FontAtlas& atlas, unsigned int textureID, const std::string& vertPath,
                  const std::string& fragPath) override;
    void drawText(const std::string& text, float x, float y, float scale, glm::vec4 color,
                  int screenWidth, int screenHeight) override;

  private:
    void* hwnd = nullptr;
};

#endif
