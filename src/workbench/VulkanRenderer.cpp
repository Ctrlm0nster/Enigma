#include "VulkanRenderer.hpp"
#include "BuiltinShaders.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <utility>

#if defined(__linux__)
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace workbench {
namespace {

constexpr uint32_t fontMipLevels = 3;

#if defined(VK_KHR_swapchain_maintenance1)
using MaintenanceFeatures = VkPhysicalDeviceSwapchainMaintenance1FeaturesKHR;
using PresentFenceInfo = VkSwapchainPresentFenceInfoKHR;
constexpr auto maintenanceFeaturesType =
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_KHR;
constexpr auto presentFenceInfoType =
    VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_KHR;
#elif defined(VK_EXT_swapchain_maintenance1)
using MaintenanceFeatures = VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT;
using PresentFenceInfo = VkSwapchainPresentFenceInfoEXT;
constexpr auto maintenanceFeaturesType =
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
constexpr auto presentFenceInfoType =
    VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
#endif

bool hasExtension(const std::vector<VkExtensionProperties> &extensions,
                  const char *name) {
  return std::ranges::any_of(extensions, [name](const auto &extension) {
    return std::strcmp(extension.extensionName, name) == 0;
  });
}

VkPipelineShaderStageCreateInfo shaderStage(VkShaderStageFlagBits stage,
                                            VkShaderModule module) {
  return {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage = stage,
      .module = module,
      .pName = "main",
      .pSpecializationInfo = nullptr,
  };
}

VkRect2D framebufferClip(Rectangle clip, VkExtent2D extent, float scaleX,
                         float scaleY) {
  const float width = static_cast<float>(extent.width);
  const float height = static_cast<float>(extent.height);
  const float left = std::clamp(std::floor(clip.x * scaleX), 0.0F, width);
  const float top = std::clamp(std::floor(clip.y * scaleY), 0.0F, height);
  const float rightEdge = std::clamp(std::ceil(right(clip) * scaleX), left, width);
  const float bottomEdge = std::clamp(std::ceil(bottom(clip) * scaleY), top, height);
  return {{static_cast<int32_t>(left), static_cast<int32_t>(top)},
          {static_cast<uint32_t>(rightEdge - left),
           static_cast<uint32_t>(bottomEdge - top)}};
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void * /*pUserData*/) {
  if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::cerr << "[Vulkan Validation] " << pCallbackData->pMessage << '\n';
  }
  return VK_FALSE;
}

VkResult createDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  }
  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void destroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks *pAllocator) {
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

} // namespace

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() { shutdown(); }

bool VulkanRenderer::init(GLFWwindow *window, bool enableValidation) {
  window_ = window;
  if (!window_) {
    return false;
  }

  if (!createInstance(enableValidation)) {
    return false;
  }

  if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) !=
      VK_SUCCESS) {
    std::cerr << "Failed to create window surface" << '\n';
    shutdown();
    return false;
  }

  if (!selectPhysicalDevice()) {
    shutdown();
    return false;
  }

  if (!createLogicalDevice()) {
    shutdown();
    return false;
  }

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = graphicsQueueFamily_;
  if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) !=
      VK_SUCCESS) {
    shutdown();
    return false;
  }

  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window_, &width, &height);
  if (!createSwapchain(width, height)) {
    shutdown();
    return false;
  }

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  // Retain more stroke detail when filtering down to small text sizes.
  samplerInfo.mipLodBias = -0.35F;
  samplerInfo.maxLod = static_cast<float>(fontMipLevels - 1);
  if (vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_) != VK_SUCCESS) {
    shutdown();
    return false;
  }

  VkDescriptorSetLayoutBinding binding{};
  binding.binding = 0;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.descriptorCount = 1;
  binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &binding;
  if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr,
                                  &descriptorSetLayout_) != VK_SUCCESS) {
    shutdown();
    return false;
  }

  std::array<VkDescriptorPoolSize, 1> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[0].descriptorCount = 64;

  VkDescriptorPoolCreateInfo descriptorPoolInfo{};
  descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolInfo.maxSets = 64;
  descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  descriptorPoolInfo.pPoolSizes = poolSizes.data();
  if (vkCreateDescriptorPool(device_, &descriptorPoolInfo, nullptr,
                             &descriptorPool_) != VK_SUCCESS) {
    shutdown();
    return false;
  }

  if (!createPipelines()) {
    shutdown();
    return false;
  }

  if (!createFrameResources()) {
    shutdown();
    return false;
  }

  return true;
}

void VulkanRenderer::shutdown() {
  if (device_ != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device_);
    waitForPresentFences(swapchain_);
    for (const auto &chain : retiredSwapchains_) {
      waitForPresentFences(chain);
    }
  }

  for (auto &[id, tex] : textures_) {
    if (tex.view != VK_NULL_HANDLE) {
      vkDestroyImageView(device_, tex.view, nullptr);
    }
    if (tex.image != VK_NULL_HANDLE) {
      vkDestroyImage(device_, tex.image, nullptr);
    }
    if (tex.memory != VK_NULL_HANDLE) {
      vkFreeMemory(device_, tex.memory, nullptr);
    }
  }
  textures_.clear();

  destroyFrameResources();
  frames_ = {};

  destroyPipelines();
  if (solidPipelineLayout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_, solidPipelineLayout_, nullptr);
    solidPipelineLayout_ = VK_NULL_HANDLE;
  }
  if (sampledPipelineLayout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_, sampledPipelineLayout_, nullptr);
    sampledPipelineLayout_ = VK_NULL_HANDLE;
  }

  if (descriptorPool_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
    descriptorPool_ = VK_NULL_HANDLE;
  }
  if (descriptorSetLayout_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
    descriptorSetLayout_ = VK_NULL_HANDLE;
  }
  if (sampler_ != VK_NULL_HANDLE) {
    vkDestroySampler(device_, sampler_, nullptr);
    sampler_ = VK_NULL_HANDLE;
  }

  destroySwapchain(swapchain_);
  for (auto &chain : retiredSwapchains_) {
    destroySwapchain(chain);
  }
  retiredSwapchains_.clear();

  if (commandPool_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device_, commandPool_, nullptr);
    commandPool_ = VK_NULL_HANDLE;
  }

  if (device_ != VK_NULL_HANDLE) {
    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }

  if (surface_ != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    surface_ = VK_NULL_HANDLE;
  }

  if (debugMessenger_ != VK_NULL_HANDLE) {
    destroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
    debugMessenger_ = VK_NULL_HANDLE;
  }

  if (instance_ != VK_NULL_HANDLE) {
    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
  }
  failed_ = false;
  swapchainOutOfDate_ = false;
  requestedExtent_ = {};
  frameIndex_ = 0;
  nextGeneration_ = 1;
  retireBefore_ = 0;
  presentFenceExtension_ = nullptr;
  surfaceMaintenanceKHR_ = false;
  surfaceMaintenanceEXT_ = false;
}

bool VulkanRenderer::createInstance(bool enableValidation) {
  uint32_t loaderVersion = VK_API_VERSION_1_0;
  auto pfnEnumerateInstanceVersion =
      reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
          vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
  if (pfnEnumerateInstanceVersion != nullptr) {
    pfnEnumerateInstanceVersion(&loaderVersion);
  }
  std::clog << "[Vulkan] Loader API version: "
            << VK_API_VERSION_MAJOR(loaderVersion) << "."
            << VK_API_VERSION_MINOR(loaderVersion) << "."
            << VK_API_VERSION_PATCH(loaderVersion) << '\n';

  uint32_t glfwExtCount = 0;
  const char **glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtCount);
  if (!glfwExtensions || glfwExtCount == 0) {
    std::cerr << "GLFW could not provide required Vulkan extensions"
              << '\n';
    return false;
  }

  std::vector<const char *> extensions(glfwExtensions,
                                       glfwExtensions + glfwExtCount);
  const auto addExtension = [&](const char *name) {
    if (std::ranges::none_of(extensions, [name](const char *enabled) {
          return std::strcmp(enabled, name) == 0;
        })) {
      extensions.push_back(name);
    }
  };

  uint32_t extensionCount = 0;
  if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                             nullptr) == VK_SUCCESS) {
    std::vector<VkExtensionProperties> available(extensionCount);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                               available.data()) == VK_SUCCESS &&
        hasExtension(available, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME)) {
#if defined(VK_KHR_swapchain_maintenance1)
      surfaceMaintenanceKHR_ =
          hasExtension(available, VK_KHR_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
      if (surfaceMaintenanceKHR_) {
        addExtension(VK_KHR_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
      }
#endif
#if defined(VK_EXT_swapchain_maintenance1)
      surfaceMaintenanceEXT_ =
          hasExtension(available, VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
      if (surfaceMaintenanceEXT_) {
        addExtension(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
      }
#endif
      if (surfaceMaintenanceKHR_ || surfaceMaintenanceEXT_) {
        addExtension(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
      }
    }
  }
  std::vector<const char *> layers;

  if (enableValidation) {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    bool validationFound = false;
    for (const auto &layer : availableLayers) {
      if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
        validationFound = true;
        break;
      }
    }

    if (validationFound) {
      layers.push_back("VK_LAYER_KHRONOS_validation");
      addExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Enigma Workbench";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "Enigma";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();
  createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
  createInfo.ppEnabledLayerNames = layers.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (enableValidation && !layers.empty()) {
    debugCreateInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType =
        VkDebugUtilsMessageTypeFlagsEXT{VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT} |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    createInfo.pNext = &debugCreateInfo;
  }

  if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
    std::cerr << "Failed to create Vulkan instance" << '\n';
    return false;
  }

  if (enableValidation && !layers.empty()) {
    createDebugUtilsMessengerEXT(instance_, &debugCreateInfo, nullptr,
                                 &debugMessenger_);
  }

  return true;
}

bool VulkanRenderer::selectPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
  if (deviceCount == 0) {
    std::cerr << "No Vulkan physical devices found" << '\n';
    return false;
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

  for (const auto &dev : devices) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(dev, &props);

    if (props.apiVersion < VK_API_VERSION_1_3) {
      std::clog << "[Vulkan] Rejected device '" << props.deviceName
                << "': API version " << VK_API_VERSION_MAJOR(props.apiVersion)
                << "." << VK_API_VERSION_MINOR(props.apiVersion)
                << " is below required 1.3" << '\n';
      continue;
    }

    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &features13;
    vkGetPhysicalDeviceFeatures2(dev, &features2);

    if (!features13.dynamicRendering || !features13.synchronization2) {
      std::clog << "[Vulkan] Rejected device '" << props.deviceName
                << "': Missing required Vulkan 1.3 feature(s):"
                << (!features13.dynamicRendering ? " dynamicRendering" : "")
                << (!features13.synchronization2 ? " synchronization2" : "")
                << '\n';
      continue;
    }

    // Check device extensions
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extCount);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount,
                                         availableExtensions.data());
    if (!hasExtension(availableExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
      std::clog << "[Vulkan] Rejected device '" << props.deviceName
                << "': Missing device extension "
                << VK_KHR_SWAPCHAIN_EXTENSION_NAME << '\n';
      continue;
    }

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount,
                                             queueFamilies.data());

    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        if (!graphicsFamily.has_value()) {
          graphicsFamily = i;
        }
      }
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface_, &presentSupport);
      if (presentSupport) {
        if (!presentFamily.has_value()) {
          presentFamily = i;
        }
      }
      if (graphicsFamily.has_value() && presentFamily.has_value() &&
          graphicsFamily == presentFamily) {
        break; // Single shared family preferred
      }
    }

    if (!graphicsFamily.has_value() || !presentFamily.has_value()) {
      std::clog << "[Vulkan] Rejected device '" << props.deviceName
                << "': Lacks queue family supporting "
                << (!graphicsFamily.has_value() ? "graphics" : "presentation")
                << '\n';
      continue;
    }

    // Check surface formats and present modes
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface_, &formatCount, nullptr);
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface_, &presentModeCount,
                                              nullptr);
    if (formatCount == 0 || presentModeCount == 0) {
      std::clog << "[Vulkan] Rejected device '" << props.deviceName
                << "': Surface reports no compatible formats or present modes"
                << '\n';
      continue;
    }

    physicalDevice_ = dev;
    graphicsQueueFamily_ = *graphicsFamily;
    presentQueueFamily_ = *presentFamily;
#if defined(VK_KHR_swapchain_maintenance1) || defined(VK_EXT_swapchain_maintenance1)
    const char *maintenanceExtension = nullptr;
#if defined(VK_KHR_swapchain_maintenance1)
    if (surfaceMaintenanceKHR_ &&
        hasExtension(availableExtensions, VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME)) {
      maintenanceExtension = VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME;
    }
#endif
#if defined(VK_EXT_swapchain_maintenance1)
    if (!maintenanceExtension && surfaceMaintenanceEXT_ &&
        hasExtension(availableExtensions, VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME)) {
      maintenanceExtension = VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME;
    }
#endif
    if (maintenanceExtension) {
      MaintenanceFeatures maintenance{};
      maintenance.sType = maintenanceFeaturesType;
      features2.pNext = &maintenance;
      vkGetPhysicalDeviceFeatures2(dev, &features2);
      if (maintenance.swapchainMaintenance1) {
        presentFenceExtension_ = maintenanceExtension;
      }
    }
#endif
    std::clog << "[Vulkan] Selected physical device: '" << props.deviceName
              << "' (API version " << VK_API_VERSION_MAJOR(props.apiVersion)
              << "." << VK_API_VERSION_MINOR(props.apiVersion) << "."
              << VK_API_VERSION_PATCH(props.apiVersion)
              << ", driver version " << props.driverVersion
              << ", graphics queue: " << graphicsQueueFamily_
              << ", present queue: " << presentQueueFamily_ << ")"
              << '\n';
    return true;
  }

  std::cerr << "No suitable Vulkan 1.3 physical device found" << '\n';
  return false;
}

bool VulkanRenderer::createLogicalDevice() {
  std::set<uint32_t> uniqueQueueFamilies = {graphicsQueueFamily_,
                                            presentQueueFamily_};
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  float queuePriority = 1.0F;

  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceVulkan13Features features13{};
  features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  features13.dynamicRendering = VK_TRUE;
  features13.synchronization2 = VK_TRUE;

  std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#if defined(VK_KHR_swapchain_maintenance1) || defined(VK_EXT_swapchain_maintenance1)
  MaintenanceFeatures maintenance{};
  if (presentFenceExtension_) {
    maintenance.sType = maintenanceFeaturesType;
    maintenance.swapchainMaintenance1 = VK_TRUE;
    features13.pNext = &maintenance;
    deviceExtensions.push_back(presentFenceExtension_);
  }
#endif

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pNext = &features13;
  createInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) !=
      VK_SUCCESS) {
    std::cerr << "Failed to create logical device" << '\n';
    return false;
  }

  vkGetDeviceQueue(device_, graphicsQueueFamily_, 0, &graphicsQueue_);
  vkGetDeviceQueue(device_, presentQueueFamily_, 0, &presentQueue_);
  std::clog << "[Vulkan] Swapchain retirement: "
            << (presentFenceExtension_ ? presentFenceExtension_ : "image reacquisition")
            << '\n';
  return true;
}

bool VulkanRenderer::fail(const char *operation, VkResult result) {
  std::cerr << "[Vulkan] " << operation << " failed (" << result << ")\n";
  failed_ = true;
  return false;
}

bool VulkanRenderer::createSwapchain(int width, int height) {
  if (width <= 0 || height <= 0) {
    return false;
  }
  VkSurfaceCapabilitiesKHR capabilities{};
  auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      physicalDevice_, surface_, &capabilities);
  if (result != VK_SUCCESS) {
    return fail("Querying surface capabilities", result);
  }

  const VkExtent2D requested{static_cast<uint32_t>(width),
                             static_cast<uint32_t>(height)};
  auto extent = capabilities.currentExtent;
  if (extent.width == std::numeric_limits<uint32_t>::max()) {
    extent = {std::clamp(requested.width, capabilities.minImageExtent.width,
                         capabilities.maxImageExtent.width),
              std::clamp(requested.height, capabilities.minImageExtent.height,
                         capabilities.maxImageExtent.height)};
  }
  if (extent.width == 0 || extent.height == 0) {
    return false;
  }

  uint32_t formatCount = 0;
  std::vector<VkSurfaceFormatKHR> formats;
  do {
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_,
                                                  &formatCount, nullptr);
    if (result != VK_SUCCESS) {
      return fail("Counting surface formats", result);
    }
    if (formatCount == 0) {
      return fail("Finding a surface format", VK_ERROR_FORMAT_NOT_SUPPORTED);
    }
    formats.resize(formatCount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_,
                                                  &formatCount, formats.data());
  } while (result == VK_INCOMPLETE);
  if (result != VK_SUCCESS || formatCount == 0) {
    return fail("Reading surface formats",
                result == VK_SUCCESS ? VK_ERROR_FORMAT_NOT_SUPPORTED : result);
  }
  formats.resize(formatCount);

  auto chosenFormat = formats.front();
  if (formats.size() == 1 && chosenFormat.format == VK_FORMAT_UNDEFINED) {
    chosenFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
  } else {
    const auto srgb = std::ranges::find_if(formats, [](const auto &format) {
      return (format.format == VK_FORMAT_B8G8R8A8_SRGB ||
              format.format == VK_FORMAT_R8G8B8A8_SRGB) &&
             format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });
    if (srgb != formats.end()) {
      chosenFormat = *srgb;
    }
  }

  constexpr std::array alphaModes{
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};
  const auto alpha = std::ranges::find_if(alphaModes, [&](const auto mode) {
    return (capabilities.supportedCompositeAlpha & mode) != 0;
  });
  if (alpha == alphaModes.end() ||
      !(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
    return fail("Finding a supported swapchain configuration",
                VK_ERROR_INITIALIZATION_FAILED);
  }

  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0) {
    imageCount = std::min(imageCount, capabilities.maxImageCount);
  }

  const uint32_t queueFamilyIndices[] = {graphicsQueueFamily_, presentQueueFamily_};
  const bool separateQueues = graphicsQueueFamily_ != presentQueueFamily_;
  const VkSwapchainCreateInfoKHR createInfo{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext = nullptr,
      .flags = 0,
      .surface = surface_,
      .minImageCount = imageCount,
      .imageFormat = chosenFormat.format,
      .imageColorSpace = chosenFormat.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = separateQueues ? VK_SHARING_MODE_CONCURRENT
                                         : VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = separateQueues ? 2U : 0U,
      .pQueueFamilyIndices = separateQueues ? queueFamilyIndices : nullptr,
      .preTransform = capabilities.currentTransform,
      .compositeAlpha = *alpha,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR,
      .clipped = VK_TRUE,
      .oldSwapchain = swapchain_.handle,
  };

  Swapchain next;
  next.format = chosenFormat.format;
  next.extent = extent;
  next.generation = nextGeneration_++;
  const auto previousFormat = swapchain_.format;
  result = vkCreateSwapchainKHR(device_, &createInfo, nullptr, &next.handle);

  // Vulkan retires oldSwapchain even if creating its replacement fails.
  if (swapchain_.handle != VK_NULL_HANDLE) {
    retiredSwapchains_.push_back(std::exchange(swapchain_, {}));
  }
  if (result != VK_SUCCESS) {
    return fail("Creating swapchain", result);
  }

  uint32_t actualImageCount = 0;
  result = vkGetSwapchainImagesKHR(device_, next.handle, &actualImageCount, nullptr);
  if (result != VK_SUCCESS || actualImageCount == 0) {
    destroySwapchain(next);
    return fail("Counting swapchain images",
                result == VK_SUCCESS ? VK_ERROR_INITIALIZATION_FAILED : result);
  }
  std::vector<VkImage> images(actualImageCount);
  result = vkGetSwapchainImagesKHR(device_, next.handle, &actualImageCount,
                                    images.data());
  if (result != VK_SUCCESS || actualImageCount == 0) {
    destroySwapchain(next);
    return fail("Reading swapchain images",
                result == VK_SUCCESS ? VK_ERROR_INITIALIZATION_FAILED : result);
  }

  next.images.resize(actualImageCount);
  VkSemaphoreCreateInfo semInfo{};
  semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  for (std::size_t i = 0; i < next.images.size(); ++i) {
    auto &slot = next.images[i];
    slot.image = images[i];
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = slot.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = next.format;
    viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    result = vkCreateImageView(device_, &viewInfo, nullptr, &slot.view);
    if (result == VK_SUCCESS) {
      result = vkCreateSemaphore(device_, &semInfo, nullptr, &slot.renderFinished);
    }
    if (result == VK_SUCCESS && presentFenceExtension_) {
      result = vkCreateFence(device_, &fenceInfo, nullptr, &slot.presentFence);
    }
    if (result != VK_SUCCESS) {
      destroySwapchain(next);
      return fail("Creating swapchain image resources", result);
    }
  }
  swapchain_ = std::move(next);
  requestedExtent_ = requested;

  // Only an attachment format change requires a pipeline rebuild.
  if (previousFormat != VK_FORMAT_UNDEFINED && previousFormat != swapchain_.format) {
    for (const auto &frame : frames_) {
      result = vkWaitForFences(device_, 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX);
      if (result != VK_SUCCESS) {
        return fail("Waiting for the previous attachment format", result);
      }
    }
    destroyPipelines();
    if (!createPipelines()) {
      return fail("Rebuilding graphics pipelines", VK_ERROR_INITIALIZATION_FAILED);
    }
  }
  return true;
}

void VulkanRenderer::destroySwapchain(Swapchain &chain) {
  for (const auto &slot : chain.images) {
    if (slot.presentFence != VK_NULL_HANDLE) {
      vkDestroyFence(device_, slot.presentFence, nullptr);
    }
    if (slot.renderFinished != VK_NULL_HANDLE) {
      vkDestroySemaphore(device_, slot.renderFinished, nullptr);
    }
    if (slot.view != VK_NULL_HANDLE) {
      vkDestroyImageView(device_, slot.view, nullptr);
    }
  }
  if (chain.handle != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_, chain.handle, nullptr);
  }
  chain = {};
}

void VulkanRenderer::waitForPresentFences(const Swapchain &chain) {
  for (const auto &slot : chain.images) {
    if (slot.presentPending) {
      vkWaitForFences(device_, 1, &slot.presentFence, VK_TRUE, UINT64_MAX);
    }
  }
}

bool VulkanRenderer::collectRetiredSwapchains() {
  for (auto &frame : frames_) {
    if (frame.generation == 0) {
      continue;
    }
    const auto result = vkGetFenceStatus(device_, frame.inFlightFence);
    if (result == VK_SUCCESS) {
      retireBefore_ = std::max(retireBefore_, frame.retireBefore);
      frame.generation = 0;
      frame.retireBefore = 0;
    } else if (result != VK_NOT_READY) {
      return fail("Polling a graphics fence", result);
    }
  }

  for (auto &chain : retiredSwapchains_) {
    if (std::ranges::any_of(frames_, [&](const auto &frame) {
          return frame.generation == chain.generation;
        })) {
      continue;
    }

    if (!presentFenceExtension_) {
      if (!chain.presentQueued || chain.generation < retireBefore_) {
        destroySwapchain(chain);
      }
      continue;
    }

    bool presentationDone = true;
    for (const auto &slot : chain.images) {
      if (!slot.presentPending) {
        continue;
      }
      const auto result = vkGetFenceStatus(device_, slot.presentFence);
      if (result == VK_NOT_READY) {
        presentationDone = false;
      } else if (result != VK_SUCCESS) {
        return fail("Polling a presentation fence", result);
      }
    }
    if (presentationDone) {
      destroySwapchain(chain);
    }
  }
  std::erase_if(retiredSwapchains_, [](const auto &chain) {
    return chain.handle == VK_NULL_HANDLE;
  });
  return true;
}

void VulkanRenderer::destroyPipelines() {
  if (solidPipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_, solidPipeline_, nullptr);
    solidPipeline_ = VK_NULL_HANDLE;
  }
  if (sampledPipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_, sampledPipeline_, nullptr);
    sampledPipeline_ = VK_NULL_HANDLE;
  }
}

VkShaderModule VulkanRenderer::createShaderModule(const uint32_t *code,
                                                  std::size_t sizeInBytes) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = sizeInBytes;
  createInfo.pCode = code;

  VkShaderModule shaderModule = VK_NULL_HANDLE;
  if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }
  return shaderModule;
}

VkShaderModule VulkanRenderer::loadOrFallbackShader(
    const std::string &shaderFilename, const uint32_t *fallbackCode,
    std::size_t fallbackSizeInBytes) {
  namespace fs = std::filesystem;
  std::string exeDir;
#if defined(__linux__)
  char buf[1024];
  const ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (len > 0) {
    buf[len] = '\0';
    exeDir = fs::path(buf).parent_path().string();
  }
#elif defined(_WIN32)
  char buf[MAX_PATH];
  const DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
  if (len > 0) {
    exeDir = fs::path(buf).parent_path().string();
  }
#endif

  const std::vector<std::string> searchDirs = {
      exeDir.empty() ? "" : exeDir + "/shaders/",
      exeDir.empty() ? "" : exeDir + "/../shaders/",
      "shaders/",
      "./shaders/",
      "build/mise/shaders/",
      "build/sanitized/shaders/"};

  for (const auto &dir : searchDirs) {
    if (dir.empty()) {
      continue;
    }
    const std::string fullPath = dir + shaderFilename + ".spv";
    std::ifstream file(fullPath, std::ios::ate | std::ios::binary);
    if (file.is_open()) {
      const auto fileSize = static_cast<std::size_t>(file.tellg());
      if (fileSize > 0 && fileSize % 4 == 0) {
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
        file.seekg(0);
        file.read(reinterpret_cast<char *>(buffer.data()),
                  static_cast<std::streamsize>(fileSize));
        VkShaderModule mod = createShaderModule(buffer.data(), fileSize);
        if (mod != VK_NULL_HANDLE) {
          std::clog << "[Vulkan] Loaded compiled SPIR-V: " << fullPath
                    << '\n';
          return mod;
        }
      }
    }
  }

  if (fallbackCode != nullptr && fallbackSizeInBytes > 0) {
    return createShaderModule(fallbackCode, fallbackSizeInBytes);
  }

  std::cerr << "Shader unavailable: " << shaderFilename << '\n';
  return VK_NULL_HANDLE;
}

bool VulkanRenderer::createPipelines() {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(float) * 2;

  // Solid pipeline layout
  VkPipelineLayoutCreateInfo solidLayoutInfo{};
  solidLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  solidLayoutInfo.pushConstantRangeCount = 1;
  solidLayoutInfo.pPushConstantRanges = &pushConstantRange;
  if (solidPipelineLayout_ == VK_NULL_HANDLE &&
      vkCreatePipelineLayout(device_, &solidLayoutInfo, nullptr,
                             &solidPipelineLayout_) != VK_SUCCESS) {
    return false;
  }

  // Sampled pipeline layout
  VkPipelineLayoutCreateInfo sampledLayoutInfo{};
  sampledLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  sampledLayoutInfo.setLayoutCount = 1;
  sampledLayoutInfo.pSetLayouts = &descriptorSetLayout_;
  sampledLayoutInfo.pushConstantRangeCount = 1;
  sampledLayoutInfo.pPushConstantRanges = &pushConstantRange;
  if (sampledPipelineLayout_ == VK_NULL_HANDLE &&
      vkCreatePipelineLayout(device_, &sampledLayoutInfo, nullptr,
                             &sampledPipelineLayout_) != VK_SUCCESS) {
    return false;
  }

  // Vertex input format
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(Vertex, pos);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(Vertex, uv);

  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributeDescriptions[2].offset = offsetof(Vertex, color);

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0F;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

  const VkPipelineMultisampleStateCreateInfo multisampling{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 0.0F,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE,
  };

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VkColorComponentFlags{VK_COLOR_COMPONENT_R_BIT} | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  const std::array<VkDynamicState, 2> dynamicStates = {
      VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  VkPipelineRenderingCreateInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachmentFormats = &swapchain_.format;

  // Build Solid Pipeline
  VkShaderModule solidVert = loadOrFallbackShader(
      "solid.vert", builtin_shaders::solid_vert,
      sizeof(builtin_shaders::solid_vert));
  VkShaderModule solidFrag = loadOrFallbackShader(
      "solid.frag", builtin_shaders::solid_frag,
      sizeof(builtin_shaders::solid_frag));

  if (solidVert == VK_NULL_HANDLE || solidFrag == VK_NULL_HANDLE) {
    if (solidVert != VK_NULL_HANDLE) {
      vkDestroyShaderModule(device_, solidVert, nullptr);
    }
    if (solidFrag != VK_NULL_HANDLE) {
      vkDestroyShaderModule(device_, solidFrag, nullptr);
    }
    return false;
  }

  const VkPipelineShaderStageCreateInfo solidStages[]{
      shaderStage(VK_SHADER_STAGE_VERTEX_BIT, solidVert),
      shaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, solidFrag),
  };

  VkGraphicsPipelineCreateInfo solidPipelineInfo{};
  solidPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  solidPipelineInfo.pNext = &renderingInfo;
  solidPipelineInfo.stageCount = 2;
  solidPipelineInfo.pStages = solidStages;
  solidPipelineInfo.pVertexInputState = &vertexInputInfo;
  solidPipelineInfo.pInputAssemblyState = &inputAssembly;
  solidPipelineInfo.pViewportState = &viewportState;
  solidPipelineInfo.pRasterizationState = &rasterizer;
  solidPipelineInfo.pMultisampleState = &multisampling;
  solidPipelineInfo.pColorBlendState = &colorBlending;
  solidPipelineInfo.pDynamicState = &dynamicState;
  solidPipelineInfo.layout = solidPipelineLayout_;

  if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1,
                                &solidPipelineInfo, nullptr,
                                &solidPipeline_) != VK_SUCCESS) {
    vkDestroyShaderModule(device_, solidVert, nullptr);
    vkDestroyShaderModule(device_, solidFrag, nullptr);
    return false;
  }
  vkDestroyShaderModule(device_, solidVert, nullptr);
  vkDestroyShaderModule(device_, solidFrag, nullptr);

  // Build Sampled Pipeline
  VkShaderModule sampledVert = loadOrFallbackShader(
      "sampled.vert", builtin_shaders::sampled_vert,
      sizeof(builtin_shaders::sampled_vert));
  VkShaderModule sampledFrag = loadOrFallbackShader(
      "sampled.frag", builtin_shaders::sampled_frag,
      sizeof(builtin_shaders::sampled_frag));

  if (sampledVert == VK_NULL_HANDLE || sampledFrag == VK_NULL_HANDLE) {
    if (sampledVert != VK_NULL_HANDLE) {
      vkDestroyShaderModule(device_, sampledVert, nullptr);
    }
    if (sampledFrag != VK_NULL_HANDLE) {
      vkDestroyShaderModule(device_, sampledFrag, nullptr);
    }
    return false;
  }

  const VkPipelineShaderStageCreateInfo sampledStages[]{
      shaderStage(VK_SHADER_STAGE_VERTEX_BIT, sampledVert),
      shaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, sampledFrag),
  };

  VkGraphicsPipelineCreateInfo sampledPipelineInfo = solidPipelineInfo;
  sampledPipelineInfo.pStages = sampledStages;
  sampledPipelineInfo.layout = sampledPipelineLayout_;

  if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1,
                                &sampledPipelineInfo, nullptr,
                                &sampledPipeline_) != VK_SUCCESS) {
    vkDestroyShaderModule(device_, sampledVert, nullptr);
    vkDestroyShaderModule(device_, sampledFrag, nullptr);
    return false;
  }
  vkDestroyShaderModule(device_, sampledVert, nullptr);
  vkDestroyShaderModule(device_, sampledFrag, nullptr);

  return true;
}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter,
                                        VkMemoryPropertyFlags properties) {
  if (physicalDevice_ == VK_NULL_HANDLE) {
    return 0;
  }
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
    if ((typeFilter & (1U << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      return i;
    }
  }
  return 0;
}

bool VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                  VkMemoryPropertyFlags properties,
                                  VkBuffer &buffer,
                                  VkDeviceMemory &bufferMemory) {
  if (device_ == VK_NULL_HANDLE) {
    return false;
  }
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    return false;
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory) !=
      VK_SUCCESS) {
    return false;
  }

  vkBindBufferMemory(device_, buffer, bufferMemory, 0);
  return true;
}

bool VulkanRenderer::createFrameResources() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool_;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  constexpr VkDeviceSize mebibyte = VkDeviceSize{1024} * 1024;
  constexpr VkDeviceSize vertexBufferSize = 2 * mebibyte;
  constexpr VkDeviceSize indexBufferSize = mebibyte;

  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);
  uint32_t memType = findMemoryType(
      0xFFFFFFFF, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  isFrameBufferCoherent_ =
      (memProperties.memoryTypes[memType].propertyFlags &
       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;

  for (auto &frame : frames_) {
    if (vkAllocateCommandBuffers(device_, &allocInfo, &frame.commandBuffer) !=
        VK_SUCCESS) {
      return false;
    }
    if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr,
                          &frame.imageAvailableSemaphore) != VK_SUCCESS) {
      return false;
    }
    if (vkCreateFence(device_, &fenceInfo, nullptr, &frame.inFlightFence) !=
        VK_SUCCESS) {
      return false;
    }

    frame.vertexBufferSize = vertexBufferSize;
    if (!createBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      frame.vertexBuffer, frame.vertexBufferMemory)) {
      return false;
    }
    vkMapMemory(device_, frame.vertexBufferMemory, 0, vertexBufferSize, 0,
                &frame.mappedVertexBuffer);

    frame.indexBufferSize = indexBufferSize;
    if (!createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      frame.indexBuffer, frame.indexBufferMemory)) {
      return false;
    }
    vkMapMemory(device_, frame.indexBufferMemory, 0, indexBufferSize, 0,
                &frame.mappedIndexBuffer);
  }

  return true;
}

void VulkanRenderer::destroyFrameResources() {
  for (auto &frame : frames_) {
    if (frame.mappedVertexBuffer != nullptr) {
      vkUnmapMemory(device_, frame.vertexBufferMemory);
      frame.mappedVertexBuffer = nullptr;
    }
    if (frame.vertexBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(device_, frame.vertexBuffer, nullptr);
      frame.vertexBuffer = VK_NULL_HANDLE;
    }
    if (frame.vertexBufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(device_, frame.vertexBufferMemory, nullptr);
      frame.vertexBufferMemory = VK_NULL_HANDLE;
    }

    if (frame.mappedIndexBuffer != nullptr) {
      vkUnmapMemory(device_, frame.indexBufferMemory);
      frame.mappedIndexBuffer = nullptr;
    }
    if (frame.indexBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(device_, frame.indexBuffer, nullptr);
      frame.indexBuffer = VK_NULL_HANDLE;
    }
    if (frame.indexBufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(device_, frame.indexBufferMemory, nullptr);
      frame.indexBufferMemory = VK_NULL_HANDLE;
    }

    if (frame.imageAvailableSemaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(device_, frame.imageAvailableSemaphore, nullptr);
      frame.imageAvailableSemaphore = VK_NULL_HANDLE;
    }
    if (frame.inFlightFence != VK_NULL_HANDLE) {
      vkDestroyFence(device_, frame.inFlightFence, nullptr);
      frame.inFlightFence = VK_NULL_HANDLE;
    }
  }
}

uint32_t VulkanRenderer::createTexture(const uint8_t *data, int width,
                                       int height, bool isSrgb,
                                       bool isFontAtlas) {
  if (!data || width <= 0 || height <= 0 || device_ == VK_NULL_HANDLE) {
    return 0;
  }

  const VkFormat format =
      isFontAtlas ? VK_FORMAT_R8_UNORM
                  : (isSrgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);
  const auto pixelCount =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  const std::size_t baseSize = pixelCount * (isFontAtlas ? 1 : 4);
  const uint32_t mipLevels =
      isFontAtlas && width % 4 == 0 && height % 4 == 0 ? fontMipLevels : 1;

  std::array<VkBufferImageCopy, fontMipLevels> regions{};
  regions[0].imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  regions[0].imageExtent = {static_cast<uint32_t>(width),
                            static_cast<uint32_t>(height), 1};

  std::vector<uint8_t> mipPixels;
  if (mipLevels > 1) {
    // Average font coverage before downsampling to small UI sizes.
    mipPixels.reserve(baseSize + baseSize / 4 + baseSize / 16);
    mipPixels.assign(data, data + baseSize);
    for (uint32_t level = 1; level < mipLevels; ++level) {
      const auto &previous = regions[level - 1];
      auto &region = regions[level];
      region.bufferOffset = mipPixels.size();
      region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1};
      const std::size_t stride = previous.imageExtent.width;
      region.imageExtent = {previous.imageExtent.width / 2,
                            previous.imageExtent.height / 2, 1};
      for (std::size_t row = 0; row < region.imageExtent.height; ++row) {
        for (std::size_t column = 0; column < region.imageExtent.width; ++column) {
          const auto first =
              previous.bufferOffset + 2 * (row * stride + column);
          const unsigned sum = mipPixels[first] + mipPixels[first + 1] +
                               mipPixels[first + stride] +
                               mipPixels[first + stride + 1];
          mipPixels.push_back(static_cast<uint8_t>((sum + 2) / 4));
        }
      }
    }
  }
  const VkDeviceSize imageSize = mipPixels.empty() ? baseSize : mipPixels.size();

  VkBuffer stagingBuffer = VK_NULL_HANDLE;
  VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
  if (!createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    stagingBuffer, stagingBufferMemory)) {
    return 0;
  }

  void *mapped = nullptr;
  if (vkMapMemory(device_, stagingBufferMemory, 0, imageSize, 0, &mapped) !=
      VK_SUCCESS) {
    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);
    return 0;
  }
  std::memcpy(mapped, mipPixels.empty() ? data : mipPixels.data(),
              static_cast<std::size_t>(imageSize));
  vkUnmapMemory(device_, stagingBufferMemory);

  const VkImageCreateInfo imageInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
      .mipLevels = mipLevels,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory imageMemory = VK_NULL_HANDLE;
  if (vkCreateImage(device_, &imageInfo, nullptr, &image) != VK_SUCCESS) {
    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);
    return 0;
  }

  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(device_, image, &memReqs);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memReqs.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(device_, &allocInfo, nullptr, &imageMemory) !=
      VK_SUCCESS) {
    vkDestroyImage(device_, image, nullptr);
    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);
    return 0;
  }
  vkBindImageMemory(device_, image, imageMemory, 0);

  // Transition and copy command
  VkCommandBufferAllocateInfo cmdAllocInfo{};
  cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdAllocInfo.commandPool = commandPool_;
  cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdAllocInfo.commandBufferCount = 1;

  VkCommandBuffer cmd = VK_NULL_HANDLE;
  vkAllocateCommandBuffers(device_, &cmdAllocInfo, &cmd);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd, &beginInfo);

  VkImageMemoryBarrier2 barrierToDst{};
  barrierToDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  barrierToDst.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
  barrierToDst.srcAccessMask = 0;
  barrierToDst.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
  barrierToDst.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
  barrierToDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrierToDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrierToDst.image = image;
  barrierToDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrierToDst.subresourceRange.baseMipLevel = 0;
  barrierToDst.subresourceRange.levelCount = mipLevels;
  barrierToDst.subresourceRange.baseArrayLayer = 0;
  barrierToDst.subresourceRange.layerCount = 1;

  VkDependencyInfo depInfo1{};
  depInfo1.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo1.imageMemoryBarrierCount = 1;
  depInfo1.pImageMemoryBarriers = &barrierToDst;
  vkCmdPipelineBarrier2(cmd, &depInfo1);

  vkCmdCopyBufferToImage(cmd, stagingBuffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels,
                         regions.data());

  VkImageMemoryBarrier2 barrierToShader{};
  barrierToShader.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  barrierToShader.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
  barrierToShader.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
  barrierToShader.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
  barrierToShader.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
  barrierToShader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrierToShader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrierToShader.image = image;
  barrierToShader.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrierToShader.subresourceRange.baseMipLevel = 0;
  barrierToShader.subresourceRange.levelCount = mipLevels;
  barrierToShader.subresourceRange.baseArrayLayer = 0;
  barrierToShader.subresourceRange.layerCount = 1;

  VkDependencyInfo depInfo2{};
  depInfo2.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo2.imageMemoryBarrierCount = 1;
  depInfo2.pImageMemoryBarriers = &barrierToShader;
  vkCmdPipelineBarrier2(cmd, &depInfo2);

  vkEndCommandBuffer(cmd);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmd;
  vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue_);

  vkFreeCommandBuffers(device_, commandPool_, 1, &cmd);
  vkDestroyBuffer(device_, stagingBuffer, nullptr);
  vkFreeMemory(device_, stagingBufferMemory, nullptr);

  // Create image view
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  if (isFontAtlas) {
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_ONE;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_ONE;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_ONE;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_R;
  } else {
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  }
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = mipLevels;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView imageView = VK_NULL_HANDLE;
  if (vkCreateImageView(device_, &viewInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    vkDestroyImage(device_, image, nullptr);
    vkFreeMemory(device_, imageMemory, nullptr);
    return 0;
  }

  // Allocate descriptor set
  VkDescriptorSetAllocateInfo dsAllocInfo{};
  dsAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dsAllocInfo.descriptorPool = descriptorPool_;
  dsAllocInfo.descriptorSetCount = 1;
  dsAllocInfo.pSetLayouts = &descriptorSetLayout_;

  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  if (vkAllocateDescriptorSets(device_, &dsAllocInfo, &descriptorSet) !=
      VK_SUCCESS) {
    vkDestroyImageView(device_, imageView, nullptr);
    vkDestroyImage(device_, image, nullptr);
    vkFreeMemory(device_, imageMemory, nullptr);
    return 0;
  }

  VkDescriptorImageInfo descImageInfo{};
  descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  descImageInfo.imageView = imageView;
  descImageInfo.sampler = sampler_;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = descriptorSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &descImageInfo;

  vkUpdateDescriptorSets(device_, 1, &descriptorWrite, 0, nullptr);

  const uint32_t id = nextTextureId_++;
  textures_[id] = {image, imageMemory, imageView, descriptorSet, width, height};
  return id;
}

void VulkanRenderer::destroyTexture(uint32_t textureId) {
  auto it = textures_.find(textureId);
  if (it != textures_.end()) {
    if (device_ != VK_NULL_HANDLE) {
      vkDeviceWaitIdle(device_);
      if (it->second.view != VK_NULL_HANDLE) {
        vkDestroyImageView(device_, it->second.view, nullptr);
      }
      if (it->second.image != VK_NULL_HANDLE) {
        vkDestroyImage(device_, it->second.image, nullptr);
      }
      if (it->second.memory != VK_NULL_HANDLE) {
        vkFreeMemory(device_, it->second.memory, nullptr);
      }
    }
    textures_.erase(it);
  }
}

bool VulkanRenderer::beginFrame(int fbWidth, int fbHeight) {
  if (failed_ || fbWidth <= 0 || fbHeight <= 0) {
    return false;
  }

  if (swapchainOutOfDate_ ||
      static_cast<uint32_t>(fbWidth) != requestedExtent_.width ||
      static_cast<uint32_t>(fbHeight) != requestedExtent_.height) {
    if (!createSwapchain(fbWidth, fbHeight)) {
      return false;
    }
    swapchainOutOfDate_ = false;
  }

  auto &frame = frames_[frameIndex_];
  VkResult fenceWait =
      vkWaitForFences(device_, 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX);
  if (fenceWait != VK_SUCCESS) {
    return fail("Waiting for a frame fence", fenceWait);
  }
  if (!collectRetiredSwapchains()) {
    return false;
  }

  VkResult acquireRes = vkAcquireNextImageKHR(
      device_, swapchain_.handle, 1000000000ULL, frame.imageAvailableSemaphore,
      VK_NULL_HANDLE, &currentImageIndex_);

  if (acquireRes == VK_ERROR_OUT_OF_DATE_KHR) {
    swapchainOutOfDate_ = true;
    return false;
  }
  if (acquireRes == VK_SUBOPTIMAL_KHR) {
    swapchainOutOfDate_ = true;
  } else if (acquireRes == VK_TIMEOUT || acquireRes == VK_NOT_READY) {
    return false;
  } else if (acquireRes != VK_SUCCESS) {
    return fail("Acquiring a swapchain image", acquireRes);
  }

  auto &image = swapchain_.images[currentImageIndex_];
  if (image.presentPending) {
    auto result = vkWaitForFences(device_, 1, &image.presentFence, VK_TRUE, UINT64_MAX);
    if (result == VK_SUCCESS) {
      result = vkResetFences(device_, 1, &image.presentFence);
    }
    if (result != VK_SUCCESS) {
      return fail("Recycling a presentation fence", result);
    }
    image.presentPending = false;
  }

  auto commandResult = vkResetCommandBuffer(frame.commandBuffer, 0);
  if (commandResult != VK_SUCCESS) {
    return fail("Resetting a frame command buffer", commandResult);
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  commandResult = vkBeginCommandBuffer(frame.commandBuffer, &beginInfo);
  if (commandResult != VK_SUCCESS) {
    return fail("Starting a frame command buffer", commandResult);
  }

  // Transition swapchain image to COLOR_ATTACHMENT_OPTIMAL
  VkImageMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  barrier.srcAccessMask = 0;
  barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.image = swapchain_.images[currentImageIndex_].image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.imageMemoryBarrierCount = 1;
  depInfo.pImageMemoryBarriers = &barrier;
  vkCmdPipelineBarrier2(frame.commandBuffer, &depInfo);

  VkRenderingAttachmentInfo colorAttachment{};
  colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView = swapchain_.images[currentImageIndex_].view;
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  const auto bg = colorToLinear(background);
  colorAttachment.clearValue.color.float32[0] = bg[0];
  colorAttachment.clearValue.color.float32[1] = bg[1];
  colorAttachment.clearValue.color.float32[2] = bg[2];
  colorAttachment.clearValue.color.float32[3] = bg[3];

  VkRenderingInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea = {{0, 0}, swapchain_.extent};
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &colorAttachment;

  vkCmdBeginRendering(frame.commandBuffer, &renderingInfo);
  return true;
}

void VulkanRenderer::renderDrawList(const DrawList &drawList, int windowWidth,
                                    int windowHeight) {
  const auto &verts = drawList.vertices();
  const auto &indices = drawList.indices();
  const auto &batches = drawList.batches();

  if (windowWidth <= 0 || windowHeight <= 0 || verts.empty() || indices.empty() ||
      batches.empty()) {
    return;
  }

  auto &frame = frames_[frameIndex_];
  const VkDeviceSize vBytes = verts.size() * sizeof(Vertex);
  const VkDeviceSize iBytes = indices.size() * sizeof(uint32_t);

  if (vBytes > frame.vertexBufferSize || iBytes > frame.indexBufferSize) {
    std::cerr << "DrawList exceeded buffer capacity" << '\n';
    return;
  }

  std::memcpy(frame.mappedVertexBuffer, verts.data(),
              static_cast<std::size_t>(vBytes));
  std::memcpy(frame.mappedIndexBuffer, indices.data(),
              static_cast<std::size_t>(iBytes));

  if (!isFrameBufferCoherent_) {
    VkMappedMemoryRange ranges[2]{};
    ranges[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    ranges[0].memory = frame.vertexBufferMemory;
    ranges[0].offset = 0;
    ranges[0].size = VK_WHOLE_SIZE;
    ranges[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    ranges[1].memory = frame.indexBufferMemory;
    ranges[1].offset = 0;
    ranges[1].size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(device_, 2, ranges);
  }

  VkViewport viewport{};
  viewport.x = 0.0F;
  viewport.y = 0.0F;
  viewport.width = static_cast<float>(swapchain_.extent.width);
  viewport.height = static_cast<float>(swapchain_.extent.height);
  viewport.minDepth = 0.0F;
  viewport.maxDepth = 1.0F;
  vkCmdSetViewport(frame.commandBuffer, 0, 1, &viewport);

  const float screenSize[2] = {static_cast<float>(windowWidth),
                               static_cast<float>(windowHeight)};
  const float scaleX = viewport.width / screenSize[0];
  const float scaleY = viewport.height / screenSize[1];

  const VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(frame.commandBuffer, 0, 1, &frame.vertexBuffer,
                          offsets);
  vkCmdBindIndexBuffer(frame.commandBuffer, frame.indexBuffer, 0,
                       VK_INDEX_TYPE_UINT32);

  std::optional<PipelineType> boundPipeline;

  for (const auto &batch : batches) {
    if (batch.indexCount == 0) {
      continue;
    }

    if (batch.scissor.width > 0.0F && batch.scissor.height > 0.0F) {
      const auto scissor =
          framebufferClip(batch.scissor, swapchain_.extent, scaleX, scaleY);
      vkCmdSetScissor(frame.commandBuffer, 0, 1, &scissor);
    } else {
      VkRect2D scissor{{0, 0}, swapchain_.extent};
      vkCmdSetScissor(frame.commandBuffer, 0, 1, &scissor);
    }

    if (batch.pipeline == PipelineType::Solid) {
      if (boundPipeline != PipelineType::Solid) {
        vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          solidPipeline_);
        vkCmdPushConstants(frame.commandBuffer, solidPipelineLayout_,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(screenSize),
                           screenSize);
        boundPipeline = PipelineType::Solid;
      }
    } else {
      if (boundPipeline != PipelineType::Sampled) {
        vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          sampledPipeline_);
        vkCmdPushConstants(frame.commandBuffer, sampledPipelineLayout_,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(screenSize),
                           screenSize);
        boundPipeline = PipelineType::Sampled;
      }

      auto it = textures_.find(batch.textureId);
      if (it != textures_.end()) {
        vkCmdBindDescriptorSets(
            frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            sampledPipelineLayout_, 0, 1, &it->second.descriptorSet, 0,
            nullptr);
      }
    }

    vkCmdDrawIndexed(frame.commandBuffer, batch.indexCount, 1,
                     batch.firstIndex, 0, 0);
  }
}

bool VulkanRenderer::endFrame() {
  auto &frame = frames_[frameIndex_];
  auto &image = swapchain_.images[currentImageIndex_];
  vkCmdEndRendering(frame.commandBuffer);

  // Transition swapchain image to PRESENT_SRC_KHR
  VkImageMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
  barrier.dstAccessMask = 0;
  barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.image = swapchain_.images[currentImageIndex_].image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.imageMemoryBarrierCount = 1;
  depInfo.pImageMemoryBarriers = &barrier;
  vkCmdPipelineBarrier2(frame.commandBuffer, &depInfo);

  auto result = vkEndCommandBuffer(frame.commandBuffer);
  if (result != VK_SUCCESS) {
    return fail("Ending a frame command buffer", result);
  }

  VkSemaphoreSubmitInfo waitSem{};
  waitSem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  waitSem.semaphore = frame.imageAvailableSemaphore;
  waitSem.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkCommandBufferSubmitInfo cmdInfo{};
  cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  cmdInfo.commandBuffer = frame.commandBuffer;

  VkSemaphoreSubmitInfo sigSem{};
  sigSem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  sigSem.semaphore = image.renderFinished;
  sigSem.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

  VkSubmitInfo2 submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submitInfo.waitSemaphoreInfoCount = 1;
  submitInfo.pWaitSemaphoreInfos = &waitSem;
  submitInfo.commandBufferInfoCount = 1;
  submitInfo.pCommandBufferInfos = &cmdInfo;
  submitInfo.signalSemaphoreInfoCount = 1;
  submitInfo.pSignalSemaphoreInfos = &sigSem;

  result = vkResetFences(device_, 1, &frame.inFlightFence);
  if (result != VK_SUCCESS) {
    return fail("Resetting a frame fence", result);
  }

  VkResult submitRes =
      vkQueueSubmit2(graphicsQueue_, 1, &submitInfo, frame.inFlightFence);
  if (submitRes != VK_SUCCESS) {
    return fail("Submitting a frame", submitRes);
  }
  frame.generation = swapchain_.generation;
  if (!presentFenceExtension_ && image.presented && !retiredSwapchains_.empty()) {
    // The frame fence proves this submit waited on a reacquisition of an
    // already-presented image on the new chain. Earlier chains on the same
    // present queue can then be released.
    frame.retireBefore = swapchain_.generation;
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &image.renderFinished;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain_.handle;
  presentInfo.pImageIndices = &currentImageIndex_;
#if defined(VK_KHR_swapchain_maintenance1) || defined(VK_EXT_swapchain_maintenance1)
  PresentFenceInfo fenceInfo{};
  if (presentFenceExtension_) {
    fenceInfo.sType = presentFenceInfoType;
    fenceInfo.swapchainCount = 1;
    fenceInfo.pFences = &image.presentFence;
    presentInfo.pNext = &fenceInfo;
  }
#endif

  VkResult presentRes = vkQueuePresentKHR(presentQueue_, &presentInfo);
  const bool presented = presentRes == VK_SUCCESS || presentRes == VK_SUBOPTIMAL_KHR;
  if (presented || presentRes == VK_ERROR_OUT_OF_DATE_KHR ||
      presentRes == VK_ERROR_SURFACE_LOST_KHR) {
    swapchain_.presentQueued = true;
    image.presented = presented;
    image.presentPending = presentFenceExtension_ != nullptr;
  }
  if (presentRes == VK_ERROR_OUT_OF_DATE_KHR ||
      presentRes == VK_SUBOPTIMAL_KHR) {
    swapchainOutOfDate_ = true;
  } else if (presentRes != VK_SUCCESS) {
    return fail("Presenting a frame", presentRes);
  }

  frameIndex_ = (frameIndex_ + 1) % MaxFramesInFlight;
  return true;
}

} // namespace workbench
