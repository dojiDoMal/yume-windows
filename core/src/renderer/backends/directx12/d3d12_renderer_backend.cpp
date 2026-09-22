#define CLASS_NAME "D3D12RendererBackend"
#include "../../../log_macros.hpp"

#include "../../../components/mesh_renderer.hpp"
#include "../../../mesh_buffer_factory.hpp"
#include "../../../shader_compiler_factory.hpp"
#include "../../../shader_program_factory.hpp"
#include "../../../stb_image.h"
#include "d3d12_mesh_buffer.hpp"
#include "d3d12_renderer_backend.hpp"
#include "d3d12_shader_program.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

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
    if (instanceBuffer) {
        instanceBuffer->Unmap(0, nullptr);
        instanceBuffer->Release();
    }
    if (textVertexBuffer) {
        textVertexBuffer->Unmap(0, nullptr);
        textVertexBuffer->Release();
    }
    if (textCB) {
        textCB->Unmap(0, nullptr);
        textCB->Release();
    }
    if (textPipelineState)
        textPipelineState->Release();
    if (textRootSignature)
        textRootSignature->Release();
    for (auto& tex : textures)
        if (tex.resource)
            tex.resource->Release();
    if (srvHeap)
        srvHeap->Release();
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

    // Explicit RTV desc so we can view the UNORM swapchain buffer through an
    // _SRGB RTV when srgb is enabled (flip-model swapchains can't be _SRGB
    // themselves). With srgb off this equals the buffer's own UNORM format.
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = getRtvFormat();
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    for (UINT i = 0; i < 2; i++) {
        if (FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]))))
            return false;
        device->CreateRenderTargetView(renderTargets[i], &rtvDesc, rtvHandle);
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

    // New frame: reset the text per-frame arenas (vertex + constant buffers).
    beginTextFrame();

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

    if (mvpAddr)
        commandList->SetGraphicsRootConstantBufferView(0, mvpAddr);
    else
        commandList->SetGraphicsRootConstantBufferView(0,
                                                       constantBuffers[0]->GetGPUVirtualAddress());

    if (matAddr)
        commandList->SetGraphicsRootConstantBufferView(1, matAddr);
    else
        commandList->SetGraphicsRootConstantBufferView(1,
                                                       constantBuffers[1]->GetGPUVirtualAddress());

    if (lightAddr)
        commandList->SetGraphicsRootConstantBufferView(2, lightAddr);
    else
        commandList->SetGraphicsRootConstantBufferView(2,
                                                       constantBuffers[2]->GetGPUVirtualAddress());
}

void D3D12RendererBackend::bindCamera(Camera* camera) {
    if (!camera)
        return;

    WorldObject* cameraObj = camera->getOwner();
    if (!cameraObj)
        return;

    glm::mat4 model = glm::mat4(1.0f);

    const auto camPos = cameraObj->getTransform().getPosition();
    const auto camRot = cameraObj->getTransform().getRotation();

    // Forward vector from yaw/pitch. Kept identical to the OpenGL backend so
    // camera controls (main.cpp WASD) behave the same across both APIs.
    glm::vec3 forward;
    float yawRad = glm::radians(camRot.y);
    float pitchRad = glm::radians(camRot.x);
    forward.x = cos(pitchRad) * sin(yawRad);
    forward.y = sin(pitchRad);
    forward.z = cos(pitchRad) * cos(yawRad);
    forward = glm::normalize(forward);
    forward = -forward; // match OpenGL's -Z forward convention

    glm::vec3 camPosVec(camPos.x, camPos.y, camPos.z);
    glm::vec3 target = camPosVec + forward;

    // Convention: match OpenGL exactly (right-handed, -Z forward). The ONLY
    // thing D3D12 needs differently is the clip-space depth range: [0,1]
    // instead of OpenGL's [-1,1]. So we keep the same right-handed view and
    // projection as the OpenGL backend and only switch to the *_ZO
    // ("zero-to-one" depth) projection variants. This keeps the X axis,
    // triangle winding and camera controls identical across both backends.
    glm::mat4 view = glm::lookAt(camPosVec, target, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection;
    if (camera->isOrthographic()) {
        float orthoSize = camera->getOrthoSize();
        float aspect = camera->getAspectRatio();
        projection = glm::orthoRH_ZO(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize,
                                     camera->getNearDistance(), camera->getFarDistance());
    } else {
        projection = glm::perspectiveRH_ZO(glm::radians(camera->getFov()), camera->getAspectRatio(),
                                           camera->getNearDistance(), camera->getFarDistance());
    }

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

    // Vsync from project.conf: sync interval 1 caps to refresh, 0 = uncapped.
    swapChain->Present(vsyncEnabled ? 1 : 0, 0);
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

void D3D12RendererBackend::beginInstanceFrame(size_t totalMatrices) {
    instanceBufferCursor = 0;
    if (totalMatrices == 0)
        return;

    // Grow (never shrink) so the whole frame fits contiguously. Reallocating
    // here — before any draw is recorded — is safe; doing it mid-frame would
    // invalidate GPU addresses already bound to earlier draws.
    if (!instanceBuffer || totalMatrices > instanceBufferCapacity) {
        if (instanceBuffer) {
            instanceBuffer->Unmap(0, nullptr);
            instanceBuffer->Release();
            instanceBuffer = nullptr;
            instanceBufferData = nullptr;
        }

        size_t newCapacity = instanceBufferCapacity ? instanceBufferCapacity : 64;
        while (newCapacity < totalMatrices)
            newCapacity *= 2;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = newCapacity * sizeof(glm::mat4);
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                                   D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                   IID_PPV_ARGS(&instanceBuffer)))) {
            LOG_ERROR("Failed to (re)create instance buffer");
            return;
        }
        instanceBuffer->Map(0, nullptr, &instanceBufferData);
        instanceBufferCapacity = newCapacity;
    }
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12RendererBackend::appendInstanceData(const glm::mat4* models,
                                                                   size_t count) {
    if (count == 0 || !instanceBuffer || !instanceBufferData)
        return 0;

    if (instanceBufferCursor + count > instanceBufferCapacity) {
        LOG_ERROR("Instance buffer overflow (cursor beyond reserved capacity)");
        return 0;
    }

    size_t offsetMatrices = instanceBufferCursor;
    auto* dst = static_cast<glm::mat4*>(instanceBufferData) + offsetMatrices;
    memcpy(dst, models, count * sizeof(glm::mat4));
    instanceBufferCursor += count;

    return instanceBuffer->GetGPUVirtualAddress() + offsetMatrices * sizeof(glm::mat4);
}

void D3D12RendererBackend::renderWorldObjects(const std::vector<WorldObject*>& objects,
                                              const std::vector<Light*>& lights) {
    // Reset per-frame draw statistics (frustumCulledObjects is set upstream by
    // Renderer::render, so it is intentionally left untouched here).
    drawnObjects = 0;
    drawnVerts = 0;
    drawnTris = 0;

    nonInstancedObjects.clear();
    for (auto& [key, group] : instanceGroups)
        group.models.clear();

    // Bucket objects into instanced groups (mesh + PSO) vs. non-instanced,
    // exactly like the OpenGL backend.
    for (auto* obj : objects) {
        if (!obj->hasMesh())
            continue;
        auto* meshRenderer = obj->getComponent<MeshRenderer>();
        if (!meshRenderer || !meshRenderer->getMaterial())
            continue;

        auto* mat = meshRenderer->getMaterial();
        if (!mat->isInstancingEnabled()) {
            nonInstancedObjects.push_back(obj);
            continue;
        }

        auto* mesh = obj->getMesh();
        auto* program = mat->getShaderProgram();
        if (!program)
            continue;

        RenderKey key{mesh->getMeshBuffer(), program->getHandle()};
        auto& group = instanceGroups[key];
        if (!group.mesh) {
            group.mesh = mesh;
            group.material = mat;
        }
        group.models.push_back(obj->getTransform().getModelMatrix());
    }

    // Reserve the whole frame's instance matrices up front (all instanced
    // groups + one matrix per non-instanced object) so no reallocation happens
    // between recorded draws.
    size_t totalMatrices = nonInstancedObjects.size();
    for (auto& [key, group] : instanceGroups)
        totalMatrices += group.models.size();
    beginInstanceFrame(totalMatrices);

    // Instanced draws.
    for (auto& [key, group] : instanceGroups) {
        if (group.models.empty())
            continue;

        auto* mat = group.material;
        mat->use();
        applyMaterial(mat); // sets PSO, root signature and CBVs

        if (!lights.empty())
            mat->applyLight(*lights[0]);

        D3D12_GPU_VIRTUAL_ADDRESS instanceAddr =
            appendInstanceData(group.models.data(), group.models.size());
        if (instanceAddr)
            commandList->SetGraphicsRootShaderResourceView(3, instanceAddr);

        auto* d3d12Buffer = static_cast<D3D12MeshBuffer*>(group.mesh->getMeshBuffer());
        D3D12_VERTEX_BUFFER_VIEW views[2] = {*d3d12Buffer->getVertexBufferView(),
                                             *d3d12Buffer->getNormalBufferView()};
        commandList->IASetVertexBuffers(0, 2, views);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        const UINT vertsPerInstance = static_cast<UINT>(group.mesh->getVertices().size() / 3);
        const UINT instanceCount = static_cast<UINT>(group.models.size());
        commandList->DrawInstanced(vertsPerInstance, instanceCount, 0, 0);

        drawnObjects += static_cast<int>(instanceCount);
        drawnVerts += static_cast<int>(vertsPerInstance) * static_cast<int>(instanceCount);
        drawnTris += static_cast<int>(vertsPerInstance / 3) * static_cast<int>(instanceCount);
    }

    // Non-instanced draws: one model matrix per object via the instance buffer
    // (a single-element StructuredBuffer, indexed at SV_InstanceID == 0).
    for (auto* obj : nonInstancedObjects) {
        auto* meshRenderer = obj->getComponent<MeshRenderer>();
        auto* mat = meshRenderer->getMaterial();
        auto* mesh = obj->getMesh();

        mat->use();
        applyMaterial(mat);
        if (!lights.empty())
            mat->applyLight(*lights[0]);

        glm::mat4 model = obj->getTransform().getModelMatrix();
        D3D12_GPU_VIRTUAL_ADDRESS instanceAddr = appendInstanceData(&model, 1);
        if (instanceAddr)
            commandList->SetGraphicsRootShaderResourceView(3, instanceAddr);

        auto* d3d12Buffer = static_cast<D3D12MeshBuffer*>(mesh->getMeshBuffer());
        D3D12_VERTEX_BUFFER_VIEW views[2] = {*d3d12Buffer->getVertexBufferView(),
                                             *d3d12Buffer->getNormalBufferView()};
        commandList->IASetVertexBuffers(0, 2, views);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        const UINT vertCount = static_cast<UINT>(mesh->getVertices().size() / 3);
        commandList->DrawInstanced(vertCount, 1, 0, 0);

        drawnObjects++;
        drawnVerts += static_cast<int>(vertCount);
        drawnTris += static_cast<int>(vertCount / 3);
    }
}

// ---------------------------------------------------------------------------
// Texture loading
// ---------------------------------------------------------------------------

bool D3D12RendererBackend::ensureSrvHeap(UINT capacity) {
    if (srvHeap && srvHeapCapacity >= capacity)
        return true;
    if (srvHeap) {
        // Growing an existing heap would invalidate live SRV handles; for this
        // engine's needs a fixed heap created up front is enough.
        LOG_WARN("SRV heap already created; cannot grow past initial capacity");
        return srvHeapCapacity >= capacity;
    }

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = capacity;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap)))) {
        LOG_ERROR("Failed to create SRV descriptor heap");
        return false;
    }
    srvDescriptorSize =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    srvHeapCapacity = capacity;
    srvHeapUsed = 0;
    return true;
}

unsigned int D3D12RendererBackend::createTexture2D(const unsigned char* pixels, int width,
                                                   int height, int channels, uint8_t filterType) {
    if (!ensureSrvHeap(16))
        return 0;
    if (srvHeapUsed >= srvHeapCapacity) {
        LOG_ERROR("SRV heap is full");
        return 0;
    }

    // Always upload as RGBA8 (stb gives us the requested channel count; we force
    // 4 channels at load time, so `channels` is expected to be 4 here).
    const DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    const UINT bytesPerPixel = 4;

    // Default-heap destination texture.
    D3D12_HEAP_PROPERTIES defaultHeap = {};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = format;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    ID3D12Resource* texResource = nullptr;
    if (FAILED(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc,
                                               D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                               IID_PPV_ARGS(&texResource)))) {
        LOG_ERROR("Failed to create texture resource");
        return 0;
    }

    // Upload-heap staging buffer. Row pitch must be aligned to
    // D3D12_TEXTURE_DATA_PITCH_ALIGNMENT (256).
    const UINT rowPitch = (width * bytesPerPixel + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) &
                          ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    const UINT uploadSize = rowPitch * height;

    D3D12_HEAP_PROPERTIES uploadHeap = {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC uploadDesc = {};
    uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadDesc.Width = uploadSize;
    uploadDesc.Height = 1;
    uploadDesc.DepthOrArraySize = 1;
    uploadDesc.MipLevels = 1;
    uploadDesc.SampleDesc.Count = 1;
    uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ID3D12Resource* uploadBuffer = nullptr;
    if (FAILED(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                               IID_PPV_ARGS(&uploadBuffer)))) {
        LOG_ERROR("Failed to create texture upload buffer");
        texResource->Release();
        return 0;
    }

    // Copy pixel rows into the staging buffer honoring the aligned row pitch.
    uint8_t* mapped = nullptr;
    uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
    for (int y = 0; y < height; y++) {
        memcpy(mapped + y * rowPitch, pixels + y * width * bytesPerPixel, width * bytesPerPixel);
    }
    uploadBuffer->Unmap(0, nullptr);

    // Record the copy on a one-shot command list (loadTexture runs at scene-load
    // time, outside the per-frame clear()/present() command list lifecycle).
    ID3D12CommandAllocator* uploadAlloc = nullptr;
    ID3D12GraphicsCommandList* uploadList = nullptr;
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&uploadAlloc));
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, uploadAlloc, nullptr,
                              IID_PPV_ARGS(&uploadList));

    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = texResource;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = uploadBuffer;
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Footprint.Format = format;
    src.PlacedFootprint.Footprint.Width = width;
    src.PlacedFootprint.Footprint.Height = height;
    src.PlacedFootprint.Footprint.Depth = 1;
    src.PlacedFootprint.Footprint.RowPitch = rowPitch;

    uploadList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texResource;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    uploadList->ResourceBarrier(1, &barrier);

    uploadList->Close();
    ID3D12CommandList* lists[] = {uploadList};
    commandQueue->ExecuteCommandLists(1, lists);
    waitForGPU(); // block until the copy is done, then we can free the staging buffer

    uploadBuffer->Release();
    uploadList->Release();
    uploadAlloc->Release();

    // Create the SRV in the shader-visible heap.
    UINT srvIndex = srvHeapUsed++;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += srvIndex * srvDescriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(texResource, &srvDesc, cpuHandle);

    TextureEntry entry;
    entry.resource = texResource;
    entry.srvIndex = srvIndex;
    textures.push_back(entry);

    // Index 0 is reserved as "invalid" so callers can treat 0 as "no texture";
    // return a 1-based id that maps to textures[id-1].
    return static_cast<unsigned int>(textures.size());
}

unsigned int D3D12RendererBackend::loadTexture(const std::string& path, uint8_t filterType) {
    int width, height, channels;
    // Force 4 channels (RGBA) so the upload format is always R8G8B8A8_UNORM.
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        LOG_ERROR("Failed to load texture: " + path);
        return 0;
    }

    LOG_INFO("Texture loaded: " + path + " (" + std::to_string(width) + "x" +
             std::to_string(height) + ")");

    unsigned int id = createTexture2D(data, width, height, 4, filterType);
    stbi_image_free(data);
    return id;
}

void D3D12RendererBackend::drawSprite(const Sprite& sprite) {}

// ---------------------------------------------------------------------------
// Text rendering (MSDF), mirroring the OpenGL backend.
// ---------------------------------------------------------------------------

bool D3D12RendererBackend::createTextPipeline(const std::string& vertPath,
                                              const std::string& fragPath) {
    // Load precompiled DXIL for the text shaders (same .cso pipeline as meshes).
    auto readFile = [](const std::string& p, std::vector<char>& out) -> bool {
        std::ifstream f(p, std::ios::ate | std::ios::binary);
        if (!f.is_open())
            return false;
        std::streamsize size = f.tellg();
        out.resize(static_cast<size_t>(size));
        f.seekg(0);
        return static_cast<bool>(f.read(out.data(), size));
    };

    std::vector<char> vs, ps;
    if (!readFile(vertPath, vs) || !readFile(fragPath, ps)) {
        LOG_ERROR("Failed to read text shaders: " + vertPath + " / " + fragPath);
        return false;
    }

    // Root signature: b4 (projection, VS), b5 (color, PS), t0 (MSDF texture, PS)
    // via a descriptor table, and a static sampler s0.
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0; // t0
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER params[3] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].Descriptor.ShaderRegister = 4; // b4
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[1].Descriptor.ShaderRegister = 5; // b5
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[2].DescriptorTable.NumDescriptorRanges = 1;
    params[2].DescriptorTable.pDescriptorRanges = &srvRange;
    params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ShaderRegister = 0; // s0
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
    rootDesc.NumParameters = 3;
    rootDesc.pParameters = params;
    rootDesc.NumStaticSamplers = 1;
    rootDesc.pStaticSamplers = &sampler;
    rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* sig = nullptr;
    ID3DBlob* err = nullptr;
    if (FAILED(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err))) {
        LOG_ERROR("Failed to serialize text root signature");
        if (err)
            err->Release();
        return false;
    }
    if (FAILED(device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(),
                                           IID_PPV_ARGS(&textRootSignature)))) {
        LOG_ERROR("Failed to create text root signature");
        sig->Release();
        return false;
    }
    sig->Release();

    // 2D input layout: float2 position, float2 uv (interleaved, single buffer).
    D3D12_INPUT_ELEMENT_DESC layout[] = {{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
                                          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                         {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8,
                                          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = textRootSignature;
    pso.VS = {vs.data(), vs.size()};
    pso.PS = {ps.data(), ps.size()};
    pso.InputLayout = {layout, 2};
    pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    // Alpha blending, matching the OpenGL text path (SRC_ALPHA, ONE_MINUS_SRC_ALPHA).
    pso.BlendState.RenderTarget[0].BlendEnable = TRUE;
    pso.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    pso.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    pso.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    pso.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    pso.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    pso.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    pso.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // Text is an overlay: no depth test/write (mirrors glDisable(GL_DEPTH_TEST)).
    pso.DepthStencilState.DepthEnable = FALSE;
    pso.SampleMask = UINT_MAX;
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = getRtvFormat(); // match the RTV (UNORM / UNORM_SRGB)
    pso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pso.SampleDesc.Count = 1;

    if (FAILED(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&textPipelineState)))) {
        LOG_ERROR("Failed to create text pipeline state");
        return false;
    }
    return true;
}

bool D3D12RendererBackend::initText(const FontAtlas& atlas, unsigned int textureID,
                                    const std::string& vertPath, const std::string& fragPath) {
    textAtlas = &atlas;
    textTextureID = textureID; // 1-based index into `textures`

    if (!createTextPipeline(vertPath, fragPath))
        return false;

    // Per-frame dynamic vertex buffer: 6 verts * 4 floats per glyph, generous
    // upper bound for on-screen debug text.
    textVertexCapacity = 6 * 4 * 4096;
    D3D12_HEAP_PROPERTIES uploadHeap = {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC vbDesc = {};
    vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vbDesc.Width = textVertexCapacity * sizeof(float);
    vbDesc.Height = 1;
    vbDesc.DepthOrArraySize = 1;
    vbDesc.MipLevels = 1;
    vbDesc.SampleDesc.Count = 1;
    vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &vbDesc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                               IID_PPV_ARGS(&textVertexBuffer)))) {
        LOG_ERROR("Failed to create text vertex buffer");
        return false;
    }
    textVertexBuffer->Map(0, nullptr, &textVertexBufferData);

    // Per-frame constant buffer arena: 256-byte slots, two per drawText call
    // (projection b4 + color b5). Reserve enough for many lines of text.
    textCBSlots = 128;
    D3D12_RESOURCE_DESC cbDesc = {};
    cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbDesc.Width = textCBSlots * 256;
    cbDesc.Height = 1;
    cbDesc.DepthOrArraySize = 1;
    cbDesc.MipLevels = 1;
    cbDesc.SampleDesc.Count = 1;
    cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &cbDesc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                               IID_PPV_ARGS(&textCB)))) {
        LOG_ERROR("Failed to create text constant buffer");
        return false;
    }
    textCB->Map(0, nullptr, &textCBData);

    return true;
}

void D3D12RendererBackend::beginTextFrame() {
    textVertexCursor = 0;
    textCBCursor = 0;
}

void D3D12RendererBackend::drawText(const std::string& text, float x, float y, float scale,
                                    ColorRGBA color, int screenWidth, int screenHeight) {
    if (!textAtlas || !textPipelineState || textTextureID == 0)
        return;
    if (textCBCursor + 2 > textCBSlots) {
        LOG_WARN("Text constant buffer arena exhausted this frame");
        return;
    }

    // Build glyph quads (same layout/order as the OpenGL path).
    std::vector<float> vertices;
    vertices.reserve(text.size() * 6 * 4);

    float cursorX = x;
    uint32_t prevChar = 0;
    for (char c : text) {
        uint32_t unicode = (uint32_t)(unsigned char)c;
        auto it = textAtlas->glyphs.find(unicode);
        if (it == textAtlas->glyphs.end()) {
            prevChar = unicode;
            continue;
        }

        const GlyphInfo& g = it->second;
        cursorX += textAtlas->getKerning(prevChar, unicode) * scale;

        if (g.hasGeometry) {
            float x0 = cursorX + g.planeLeft * scale, x1 = cursorX + g.planeRight * scale;
            float y0 = y - g.planeTop * scale, y1 = y - g.planeBottom * scale;
            float u0 = g.atlasLeft / textAtlas->atlasWidth,
                  u1 = g.atlasRight / textAtlas->atlasWidth;
            float v0 = 1.0f - (g.atlasBottom / textAtlas->atlasHeight);
            float v1 = 1.0f - (g.atlasTop / textAtlas->atlasHeight);

            float quad[6][4] = {
                {x0, y0, u0, v1}, {x0, y1, u0, v0}, {x1, y1, u1, v0},
                {x0, y0, u0, v1}, {x1, y1, u1, v0}, {x1, y0, u1, v1},
            };
            for (auto& v : quad)
                vertices.insert(vertices.end(), v, v + 4);
        }
        cursorX += g.advance * scale;
        prevChar = unicode;
    }

    if (vertices.empty())
        return;
    if (textVertexCursor + vertices.size() > textVertexCapacity) {
        LOG_WARN("Text vertex arena exhausted this frame");
        return;
    }

    // Append vertices into the per-frame arena at a distinct offset.
    size_t vtxOffsetFloats = textVertexCursor;
    memcpy(static_cast<float*>(textVertexBufferData) + vtxOffsetFloats, vertices.data(),
           vertices.size() * sizeof(float));
    textVertexCursor += vertices.size();

    // Projection (b4) matches OpenGL: top-left origin ortho. Use RH_ZO so depth
    // is valid for D3D12 (depth is unused here anyway since depth test is off).
    glm::mat4 proj =
        glm::orthoRH_ZO(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);

    UINT projSlot = textCBCursor++;
    UINT colorSlot = textCBCursor++;
    uint8_t* cbBase = static_cast<uint8_t*>(textCBData);
    memcpy(cbBase + projSlot * 256, glm::value_ptr(proj), sizeof(glm::mat4));

    struct ColorBlock {
        ColorRGBA color;
        float distRange;
        float pad[3];
    } colorData{color, textAtlas->distanceRange * (scale / textAtlas->atlasSize), {0, 0, 0}};
    memcpy(cbBase + colorSlot * 256, &colorData, sizeof(ColorBlock));

    // Record the draw. The main command list is open (clear() reset it), and
    // drawText runs before present().
    commandList->SetPipelineState(textPipelineState);
    commandList->SetGraphicsRootSignature(textRootSignature);

    ID3D12DescriptorHeap* heaps[] = {srvHeap};
    commandList->SetDescriptorHeaps(1, heaps);

    commandList->SetGraphicsRootConstantBufferView(0,
                                                   textCB->GetGPUVirtualAddress() + projSlot * 256);
    commandList->SetGraphicsRootConstantBufferView(1, textCB->GetGPUVirtualAddress() +
                                                          colorSlot * 256);

    const TextureEntry& tex = textures[textTextureID - 1];
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += tex.srvIndex * srvDescriptorSize;
    commandList->SetGraphicsRootDescriptorTable(2, gpuHandle);

    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation = textVertexBuffer->GetGPUVirtualAddress() + vtxOffsetFloats * sizeof(float);
    vbv.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(float));
    vbv.StrideInBytes = 4 * sizeof(float);
    commandList->IASetVertexBuffers(0, 1, &vbv);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->DrawInstanced(static_cast<UINT>(vertices.size() / 4), 1, 0, 0);
}
