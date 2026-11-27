/**
 * @file VulkanRenderer.hpp
 * @brief Vulkan 1.3 dynamic rendering backend for Enigma Workbench.
 */

#pragma once

#include "DrawCommand.hpp"
#include "Geometry.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace workbench {

inline constexpr int MaxFramesInFlight = 2;

struct TextureResource {
  VkImage image{VK_NULL_HANDLE};
  VkDeviceMemory memory{VK_NULL_HANDLE};
  VkImageView view{VK_NULL_HANDLE};
  VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
  int width{0};
  int height{0};
};

struct FrameSlot {
  VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
  VkSemaphore imageAvailableSemaphore{VK_NULL_HANDLE};
  VkFence inFlightFence{VK_NULL_HANDLE};
  uint64_t generation{0};
  uint64_t retireBefore{0};

  VkBuffer vertexBuffer{VK_NULL_HANDLE};
  VkDeviceMemory vertexBufferMemory{VK_NULL_HANDLE};
  void *mappedVertexBuffer{nullptr};
  VkDeviceSize vertexBufferSize{0};

  VkBuffer indexBuffer{VK_NULL_HANDLE};
  VkDeviceMemory indexBufferMemory{VK_NULL_HANDLE};
  void *mappedIndexBuffer{nullptr};
  VkDeviceSize indexBufferSize{0};
};

class VulkanRenderer {
public:
  VulkanRenderer();
  ~VulkanRenderer();

  VulkanRenderer(const VulkanRenderer &) = delete;
  VulkanRenderer &operator=(const VulkanRenderer &) = delete;

  bool init(GLFWwindow *window, bool enableValidation = true);
  void shutdown();

  // Texture creation & destruction
  uint32_t createTexture(const uint8_t *data, int width, int height,
                         bool isSrgb, bool isFontAtlas = false);
  void destroyTexture(uint32_t textureId);

  // Frame lifecycle
  bool beginFrame(int fbWidth, int fbHeight);
  void renderDrawList(const DrawList &drawList, int windowWidth, int windowHeight);
  bool endFrame();

  bool isInitialized() const { return device_ != VK_NULL_HANDLE; }
  VkDevice device() const { return device_; }
  bool hasFailed() const { return failed_; }
  bool needsRedraw() const {
    return swapchainOutOfDate_ || !retiredSwapchains_.empty();
  }

private:
  struct SwapchainImage {
    VkImage image{VK_NULL_HANDLE};
    VkImageView view{VK_NULL_HANDLE};
    VkSemaphore renderFinished{VK_NULL_HANDLE};
    VkFence presentFence{VK_NULL_HANDLE};
    bool presentPending{false};
    bool presented{false};
  };

  struct Swapchain {
    VkSwapchainKHR handle{VK_NULL_HANDLE};
    VkFormat format{VK_FORMAT_UNDEFINED};
    VkExtent2D extent{};
    uint64_t generation{0};
    bool presentQueued{false};
    std::vector<SwapchainImage> images;
  };

  bool createInstance(bool enableValidation);
  bool selectPhysicalDevice();
  bool createLogicalDevice();
  bool createSwapchain(int width, int height);
  void destroySwapchain(Swapchain &chain);
  bool collectRetiredSwapchains();
  void waitForPresentFences(const Swapchain &chain);
  bool fail(const char *operation, VkResult result);
  bool createPipelines();
  void destroyPipelines();
  bool createFrameResources();
  void destroyFrameResources();

  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
  bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer &buffer,
                    VkDeviceMemory &bufferMemory);

  VkShaderModule createShaderModule(const uint32_t *code, std::size_t sizeInBytes);
  VkShaderModule loadOrFallbackShader(const std::string &shaderFilename,
                                      const uint32_t *fallbackCode,
                                      std::size_t fallbackSizeInBytes);

  GLFWwindow *window_{nullptr};
  VkInstance instance_{VK_NULL_HANDLE};
  VkDebugUtilsMessengerEXT debugMessenger_{VK_NULL_HANDLE};
  VkSurfaceKHR surface_{VK_NULL_HANDLE};

  VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
  uint32_t graphicsQueueFamily_{0};
  uint32_t presentQueueFamily_{0};
  VkDevice device_{VK_NULL_HANDLE};
  VkQueue graphicsQueue_{VK_NULL_HANDLE};
  VkQueue presentQueue_{VK_NULL_HANDLE};

  VkCommandPool commandPool_{VK_NULL_HANDLE};

  Swapchain swapchain_;
  std::vector<Swapchain> retiredSwapchains_;
  VkExtent2D requestedExtent_{};
  uint64_t nextGeneration_{1};
  uint64_t retireBefore_{0};
  bool surfaceMaintenanceKHR_{false};
  bool surfaceMaintenanceEXT_{false};
  const char *presentFenceExtension_{nullptr};

  VkSampler sampler_{VK_NULL_HANDLE};
  VkDescriptorSetLayout descriptorSetLayout_{VK_NULL_HANDLE};
  VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};

  VkPipelineLayout solidPipelineLayout_{VK_NULL_HANDLE};
  VkPipeline solidPipeline_{VK_NULL_HANDLE};

  VkPipelineLayout sampledPipelineLayout_{VK_NULL_HANDLE};
  VkPipeline sampledPipeline_{VK_NULL_HANDLE};

  std::array<FrameSlot, MaxFramesInFlight> frames_{};
  uint32_t frameIndex_{0};
  uint32_t currentImageIndex_{0};
  bool swapchainOutOfDate_{false};
  bool failed_{false};
  bool isFrameBufferCoherent_{true};

  std::unordered_map<uint32_t, TextureResource> textures_;
  uint32_t nextTextureId_{1};
};

} // namespace workbench
