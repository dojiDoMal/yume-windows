#ifndef VULKAN_RENDERER_BACKEND_HPP
#define VULKAN_RENDERER_BACKEND_HPP

#include "../../../mesh.hpp"
#include "../../../world_object.hpp"
#include "../../renderer_backend.hpp"
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

struct SDL_Window;
class VulkanRendererBackend : public RendererBackend {
  private:
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    VkBuffer uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
    VkBuffer materialBuffer = VK_NULL_HANDLE;
    VkDeviceMemory materialBufferMemory = VK_NULL_HANDLE;
    VkBuffer lightDataBuffer = VK_NULL_HANDLE;
    VkDeviceMemory lightDataBufferMemory = VK_NULL_HANDLE;

    // Per-instance model matrices (binding 3, storage buffer). flat.vxs.spv
    // reads instanceModels[gl_InstanceID] from this; SPIRV-Cross mapped the
    // HLSL StructuredBuffer t0 to binding 3 (via -fvk-t-shift 3 0).
    //
    // Like the D3D12 backend, a Vulkan command buffer is recorded then executed
    // at present(); every draw that reads a distinct set of matrices needs a
    // distinct region. We use a host-visible buffer as a per-frame linear arena
    // and bind the storage buffer at each draw's offset via a dynamic offset.
    VkBuffer instanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory instanceBufferMemory = VK_NULL_HANDLE;
    void* instanceBufferMapped = nullptr;
    size_t instanceBufferCapacity = 0;  // in number of glm::mat4
    size_t instanceBufferCursor = 0;    // next free matrix slot, per frame
    VkDeviceSize instanceAlignment = 0; // min storage buffer offset alignment

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;

    uint32_t graphicsQueueFamily = 0;
    uint32_t presentQueueFamily = 0;
    uint32_t currentImageIndex = 0;
    VkFormat swapchainFormat;
    VkExtent2D swapchainExtent;
    SDL_Window* window;

    bool createInstance();
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapchain();
    bool createImageViews();
    bool createRenderPass();
    bool createDescriptorSetLayout();
    bool createFramebuffers();
    bool createCommandPool();
    bool createDepthResources();
    bool createUniformBuffer();
    bool createMaterialBuffer();
    bool createLightDataBuffer();
    bool createInstanceBuffer();
    bool createDescriptorPool();
    bool createCommandBuffers();
    bool createSyncObjects();

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // Instancing groups, mirroring the OpenGL/D3D12 backends: objects sharing
    // the same mesh + pipeline are batched into one instanced draw.
    struct RenderKey {
        void* mesh;
        void* pipeline;
        bool operator==(const RenderKey& o) const {
            return mesh == o.mesh && pipeline == o.pipeline;
        }
    };
    struct RenderKeyHash {
        size_t operator()(const RenderKey& k) const {
            return std::hash<void*>()(k.mesh) ^ (std::hash<void*>()(k.pipeline) << 1);
        }
    };
    struct InstanceGroup {
        std::vector<glm::mat4> models;
        const Mesh* mesh = nullptr;
        Material* material = nullptr;
    };
    std::unordered_map<RenderKey, InstanceGroup, RenderKeyHash> instanceGroups;
    std::vector<WorldObject*> nonInstancedObjects;

    // Resets the per-frame instance arena cursor (call once per frame).
    void beginInstanceFrame();
    // Appends `count` matrices to the arena; writes `outOffset` (byte offset,
    // aligned) for the dynamic descriptor binding. Returns false on overflow.
    bool appendInstanceData(const glm::mat4* models, size_t count, uint32_t& outOffset);

    // ---- Textures ------------------------------------------------------------
    // loadTexture returns a 1-based id into these entries (0 = none). The MSDF
    // font texture is stored here.
    struct TextureEntry {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };
    std::vector<TextureEntry> textures;
    VkSampler textureSampler = VK_NULL_HANDLE;

    // ---- Text rendering (MSDF) ----------------------------------------------
    // Bindings (from SPIR-V reflection of text.vxs/text.pxs):
    //   set 0: binding 0 = msdfTexture (sampled image)
    //          binding 1 = msdfSampler (sampler, shifted via -fvk-s-shift)
    //          binding 4 = TextUniforms (projection, VS)
    //          binding 5 = TextColor (color + distanceRange, FS)
    const FontAtlas* textAtlas = nullptr;
    unsigned int textTextureID = 0; // 1-based index into `textures`
    VkDescriptorSetLayout textDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool textDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet textDescriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout textPipelineLayout = VK_NULL_HANDLE;
    VkPipeline textPipeline = VK_NULL_HANDLE;

    // Per-frame arenas (host-visible, reset each frame), same rationale as the
    // instance buffer: each drawText records a draw into the frame's command
    // buffer, so it needs its own vertex range and its own projection/color CBs.
    VkBuffer textVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory textVertexMemory = VK_NULL_HANDLE;
    void* textVertexMapped = nullptr;
    size_t textVertexCapacity = 0; // in floats
    size_t textVertexCursor = 0;   // per frame

    VkBuffer textCB = VK_NULL_HANDLE;
    VkDeviceMemory textCBMemory = VK_NULL_HANDLE;
    void* textCBMapped = nullptr;
    VkDeviceSize textCBSlotSize = 0; // aligned slot (holds proj OR color)
    uint32_t textCBSlots = 0;
    uint32_t textCBCursor = 0; // per frame
    VkDeviceSize uboAlignment = 0;

    bool createTextureSampler();
    bool createTextPipeline(const std::string& vertPath, const std::string& fragPath);
    void beginTextFrame();
    // Reads SPIR-V bytecode from a .spv file into a shader module.
    VkShaderModule loadShaderModule(const std::string& path);

  public:
    ~VulkanRendererBackend();

    unsigned int loadTexture(const std::string& path, uint8_t filterType = 0) override;
    void drawSprite(const Sprite& sprite) override;
    bool init() override;
    bool initWindowContext() override;
    void bindCamera(Camera* camera) override;
    void applyMaterial(Material* material) override;
    void renderWorldObjects(const std::vector<WorldObject*>& objects,
                            const std::vector<Light*>& lights) override;
    void setBufferDataImpl(const std::string& name, const void* data, size_t size) override;
    void clear(Camera* camera) override;
    void draw(const Mesh&) override;
    void setUniforms(ShaderProgram* shaderProgram) override;
    void onCameraSet() override;
    GraphicsAPI getGraphicsAPI() const override;
    std::string getShaderExtension() const override;
    void renderSkybox(const Mesh& mesh, unsigned int shaderProgram,
                      unsigned int textureID) override;
    unsigned int createCubemapTexture(const std::vector<std::string>& faces) override;
    std::unique_ptr<ShaderProgram> createShaderProgram() override;
    std::unique_ptr<ShaderCompiler> createShaderCompiler() override;
    std::unique_ptr<MeshBuffer> createMeshBuffer() override;
    void present(SDL_Window* window) override;

    VkDevice getDevice() const { return device; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    VkCommandPool getCommandPool() const { return commandPool; }
    VkInstance getInstance() const { return instance; }
    VkDeviceMemory getMaterialBufferMemory() const { return materialBufferMemory; }
    VkDeviceMemory getLightDataBufferMemory() const { return lightDataBufferMemory; }
    VkExtent2D getSwapchainExtent() const { return swapchainExtent; }
    VkRenderPass getRenderPass() const { return renderPass; }
    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
    void setSurface(VkSurfaceKHR surf) { surface = surf; }
    void setWindow(SDL_Window* win) { window = win; }
    unsigned int getRequiredWindowFlags() const override;
    bool init(SDL_Window* window) override;

    bool initText(const FontAtlas& atlas, unsigned int textureID, const std::string& vertPath,
                  const std::string& fragPath) override;
    void drawText(const std::string& text, float x, float y, float scale, glm::vec4 color,
                  int screenWidth, int screenHeight) override;
};

#endif // VULKAN_RENDERER_BACKEND_HPP
