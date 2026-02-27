#ifndef D3D12_RENDERER_BACKEND_HPP
#define D3D12_RENDERER_BACKEND_HPP

#include "../../../game_object.hpp"
#include "../../renderer_backend.hpp"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <unordered_map>

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
    void renderGameObjects(std::vector<GameObject*>* gameObjects, std::vector<Light>* lights) override;
    void clear(Camera* camera) override;
    void draw(const Mesh&) override;
    void setUniforms(ShaderProgram* shaderProgram) override;
    void onCameraSet() override;
    GraphicsAPI getGraphicsAPI() const override;
    std::string getShaderExtension() const override;
    void renderSkybox(const Mesh& mesh, unsigned int shaderProgram, unsigned int textureID) override;
    void setBufferDataImpl(const std::string& name, const void* data, size_t size) override;
    unsigned int createCubemapTexture(const std::vector<std::string>& faces) override;
    std::unique_ptr<ShaderProgram> createShaderProgram() override;
    std::unique_ptr<ShaderCompiler> createShaderCompiler() override;
    std::unique_ptr<MeshBuffer> createMeshBuffer() override;
    void present(SDL_Window* window) override;
    
    ID3D12Device* getDevice() const { return device; }
    ID3D12GraphicsCommandList* getCommandList() const { return commandList; }
    void updateConstantBuffer(int binding, const void* data, size_t size);
    void setHwnd(void* hwnd) { this->hwnd = hwnd; }
    unsigned int getRequiredWindowFlags() const override;
    bool init(SDL_Window* window) override;
    
private:
    void* hwnd = nullptr;
};

#endif
