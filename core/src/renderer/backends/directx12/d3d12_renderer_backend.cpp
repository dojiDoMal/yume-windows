#define CLASS_NAME "D3D12RendererBackend"
#include "../../../log_macros.hpp"

#include "../../../mesh_buffer_factory.hpp"
#include "../../../shader_compiler_factory.hpp"
#include "../../../shader_program_factory.hpp"
#include "d3d12_mesh_buffer.hpp"
#include "d3d12_renderer_backend.hpp"
#include "d3d12_shader_program.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

GraphicsAPI D3D12RendererBackend::getGraphicsAPI() const { return GraphicsAPI::DIRECTX12; }

std::string D3D12RendererBackend::getShaderExtension() const { return ".cso"; }

unsigned int D3D12RendererBackend::getRequiredWindowFlags() const { return 0; }

std::unique_ptr<ShaderProgram> D3D12RendererBackend::createShaderProgram() {
    return ShaderProgramFactory::create(getGraphicsAPI(), this);
}

std::unique_ptr<MeshBuffer> D3D12RendererBackend::createMeshBuffer() {
    return MeshBufferFactory::create(getGraphicsAPI(), this);
}

std::unique_ptr<ShaderCompiler> D3D12RendererBackend::createShaderCompiler() {
    return ShaderCompilerFactory::create(getGraphicsAPI(), this);
}

D3D12RendererBackend::~D3D12RendererBackend() {
    waitForGPU();

    for (int i = 0; i < 3; i++) {
        if (constantBuffers[i])
            constantBuffers[i]->Release();
    }
    if (fence)
        fence->Release();
    if (fenceEvent)
        CloseHandle(fenceEvent);
    if (commandList)
        commandList->Release();
    if (commandAllocator)
        commandAllocator->Release();
    if (depthStencil)
        depthStencil->Release();
    for (auto& rt : renderTargets)
        if (rt)
            rt->Release();
    if (dsvHeap)
        dsvHeap->Release();
    if (rtvHeap)
        rtvHeap->Release();
    if (swapChain)
        swapChain->Release();
    if (commandQueue)
        commandQueue->Release();
    if (device)
        device->Release();
}

bool D3D12RendererBackend::init() { return true; }

bool D3D12RendererBackend::initWindowContext() { return true; }

bool D3D12RendererBackend::init(SDL_Window* window) {

    if (!window) {
        LOG_ERROR("Window is null!");
        return false;
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(window, &wmInfo)) {
        LOG_ERROR("Failed to get window info!");
        return false;
    }
    setHwnd(wmInfo.info.win.window);

    if (!hwnd) {
        LOG_ERROR("HWND is null!");
        return false;
    }

    if (!createDevice()) {
        LOG_ERROR("Failed to create device");
        return false;
    }
    if (!createCommandQueue()) {
        LOG_ERROR("Failed to create command queue");
        return false;
    }
    if (!createSwapChain(hwnd)) {
        LOG_ERROR("Failed to create swap chain");
        return false;
    }
    if (!createDescriptorHeaps()) {
        LOG_ERROR("Failed to create descriptor heaps");
        return false;
    }
    if (!createRenderTargets()) {
        LOG_ERROR("Failed to create render targets");
        return false;
    }
    if (!createDepthStencil()) {
        LOG_ERROR("Failed to create depth stencil");
        return false;
    }
    if (!createCommandObjects()) {
        LOG_ERROR("Failed to create command objects");
        return false;
    }
    if (!createFence()) {
        LOG_ERROR("Failed to create fence");
        return false;
    }
    if (!createConstantBuffers()) {
        LOG_ERROR("Failed to create constant buffers");
        return false;
    }
    LOG_INFO("D3D12 backend initialized successfully");
    return true;
}

bool D3D12RendererBackend::createDevice() {
    return SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
}

bool D3D12RendererBackend::createCommandQueue() {
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    return SUCCEEDED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
}

bool D3D12RendererBackend::createSwapChain(void* hwnd) {
    IDXGIFactory4* factory;
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Width = 800;
    swapChainDesc.Height = 600;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    IDXGISwapChain1* swapChain1;
    HRESULT hr = factory->CreateSwapChainForHwnd(commandQueue, (HWND)hwnd, &swapChainDesc, nullptr,
                                                 nullptr, &swapChain1);
    factory->Release();

    if (FAILED(hr))
        return false;
    swapChain1->QueryInterface(IID_PPV_ARGS(&swapChain));
    swapChain1->Release();

    frameIndex = swapChain->GetCurrentBackBufferIndex();
    return true;
}

bool D3D12RendererBackend::createDescriptorHeaps() {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 2;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    if (FAILED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap))))
        return false;

    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    return SUCCEEDED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));
}

bool D3D12RendererBackend::createRenderTargets() {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < 2; i++) {
        if (FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]))))
            return false;
        device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
        rtvHandle.ptr += rtvDescriptorSize;
    }
    return true;
}

bool D3D12RendererBackend::createDepthStencil() {
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = 800;
    depthDesc.Height = 600;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;

    if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
                                               D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
                                               IID_PPV_ARGS(&depthStencil))))
        return false;

    device->CreateDepthStencilView(depthStencil, nullptr,
                                   dsvHeap->GetCPUDescriptorHandleForHeapStart());
    return true;
}

bool D3D12RendererBackend::createCommandObjects() {
    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                              IID_PPV_ARGS(&commandAllocator))))
        return false;
    return SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator,
                                               nullptr, IID_PPV_ARGS(&commandList)));
}

bool D3D12RendererBackend::createFence() {
    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
        return false;
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    return fenceEvent != nullptr;
}

bool D3D12RendererBackend::createConstantBuffers() {
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = 256;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    for (int i = 0; i < 3; i++) {
        if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                                   D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                   IID_PPV_ARGS(&constantBuffers[i]))))
            return false;
        constantBuffers[i]->Map(0, nullptr, &constantBufferData[i]);
    }
    
    uniformBindings["ModelViewProjection"] = 0;
    uniformBindings["MaterialData"] = 1;
    uniformBindings["LightData"] = 2;
    
    return true;
}


void D3D12RendererBackend::waitForGPU() {
    commandQueue->Signal(fence, ++fenceValue);
    fence->SetEventOnCompletion(fenceValue, fenceEvent);
    WaitForSingleObject(fenceEvent, INFINITE);
}

void D3D12RendererBackend::onCameraSet() {}

void D3D12RendererBackend::clear(Camera* camera) {
    commandAllocator->Reset();
    commandList->Reset(commandAllocator, nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = renderTargets[frameIndex];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += frameIndex * rtvDescriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    if (camera) {
        auto bg = camera->getBackgroundColor();
        clearColor[0] = bg.r;
        clearColor[1] = bg.g;
        clearColor[2] = bg.b;
        clearColor[3] = bg.a;
    }

    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    D3D12_VIEWPORT viewport = {0, 0, 800, 600, 0, 1};
    D3D12_RECT scissor = {0, 0, 800, 600};
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);
}

void D3D12RendererBackend::draw(const Mesh& mesh) {
    auto* d3d12Buffer = static_cast<D3D12MeshBuffer*>(mesh.getMeshBuffer());
    D3D12_VERTEX_BUFFER_VIEW views[2] = {*d3d12Buffer->getVertexBufferView(),
                                         *d3d12Buffer->getNormalBufferView()};
    commandList->IASetVertexBuffers(0, 2, views);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(mesh.getVertices().size() / 3, 1, 0, 0);
}

void D3D12RendererBackend::setUniforms(ShaderProgram* shaderProgram) {
    if (!shaderProgram)
        return;

    auto* program = static_cast<D3D12ShaderProgram*>(shaderProgram);
    if (!program || !program->isValid())
        return;

    auto pipelineState = program->getPipelineState();
    auto rootSignature = program->getRootSignature();

    if (!pipelineState || !rootSignature) {
        LOG_ERROR("Pipeline state or root signature is null");
        return;
    }

    commandList->SetPipelineState(pipelineState);
    commandList->SetGraphicsRootSignature(rootSignature);

    auto mvpAddr = program->getConstantBufferAddress("ModelViewProjection");
    auto matAddr = program->getConstantBufferAddress("MaterialData");
    auto lightAddr = program->getConstantBufferAddress("LightData");
    
    if (mvpAddr) commandList->SetGraphicsRootConstantBufferView(0, mvpAddr);
    else commandList->SetGraphicsRootConstantBufferView(0, constantBuffers[0]->GetGPUVirtualAddress());
    
    if (matAddr) commandList->SetGraphicsRootConstantBufferView(1, matAddr);
    else commandList->SetGraphicsRootConstantBufferView(1, constantBuffers[1]->GetGPUVirtualAddress());
    
    if (lightAddr) commandList->SetGraphicsRootConstantBufferView(2, lightAddr);
    else commandList->SetGraphicsRootConstantBufferView(2, constantBuffers[2]->GetGPUVirtualAddress());
}

void D3D12RendererBackend::bindCamera(Camera* camera) {
    glm::mat4 model =
        glm::rotate(glm::mat4(1.0f), SDL_GetTicks() / 1000.0f, glm::vec3(0.5f, 1.0f, 0.0f));

    auto& camPos = camera->getPosition();
    glm::mat4 view = glm::lookAt({camPos.x, camPos.y, camPos.z}, glm::vec3(0.0f, 0.0f, 0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection =
        glm::perspective(glm::radians(camera->getFov()), camera->getAspectRatio(),
                         camera->getNearDistance(), camera->getFarDistance());

    struct {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    } matrices = {model, view, projection};

    memcpy(constantBufferData[0], &matrices, sizeof(matrices));
}

void D3D12RendererBackend::setBufferDataImpl(const std::string& name, const void* data,
                                             size_t size) {
    auto it = uniformBindings.find(name);
    if (it != uniformBindings.end()) {
        updateConstantBuffer(it->second, data, size);
    }
}


void D3D12RendererBackend::updateConstantBuffer(int binding, const void* data, size_t size) {
    if (binding >= 0 && binding < 3 && constantBufferData[binding]) {
        memcpy(constantBufferData[binding], data, size);
    }
}

unsigned int D3D12RendererBackend::createCubemapTexture(const std::vector<std::string>& faces) {
    return 0;
}

void D3D12RendererBackend::renderSkybox(const Mesh& mesh, unsigned int shaderProgram,
                                        unsigned int textureID) {}

void D3D12RendererBackend::present(SDL_Window* window) {
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = renderTargets[frameIndex];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList->ResourceBarrier(1, &barrier);

    commandList->Close();
    ID3D12CommandList* cmdLists[] = {commandList};
    commandQueue->ExecuteCommandLists(1, cmdLists);

    swapChain->Present(1, 0);
    waitForGPU();
    frameIndex = swapChain->GetCurrentBackBufferIndex();
}

void D3D12RendererBackend::applyMaterial(Material* material) {
    auto program = material->getShaderProgram();
    if (!program || !program->isValid()) {
        return;
    }

    setUniforms(program);
}

void D3D12RendererBackend::renderGameObjects(std::vector<GameObject*>* gameObjects,
                                             std::vector<Light>* lights = nullptr) {
    for (const auto go : *gameObjects) {
        auto mesh = go->getMesh();
        if (!mesh) {
            LOG_INFO("Mesh is null!");
            return;
        }

        auto meshRenderer = go->getMeshRenderer();
        if (!meshRenderer) {
            LOG_INFO("MeshRenderer is null!");
            return;
        }

        auto mat = meshRenderer->getMaterial();
        if (!mat) {
            LOG_INFO("Material is null!");
            return;
        }

        mat->use();
        applyMaterial(mat);

        if (lights && !lights->empty()) {
            mat->applyLight((*lights)[0]);
        }

        draw(*mesh);
    }
}

unsigned int D3D12RendererBackend::loadTexture(const std::string& path, uint8_t filterType) { return 0; };
    
void D3D12RendererBackend::drawSprite(const Sprite& sprite) {};
