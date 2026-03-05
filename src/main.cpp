#include <cstdint>
#include <algorithm>
#include <limits>
#include <print>
#include <ranges>
#include <tuple>
#include <vector>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace {
  constexpr char const* kWindowTitle = "Learning Vulkan";
  constexpr int kWindowWidth = 800;
  constexpr int kWindowHeight = 600;

  const std::vector<const char*> gValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };

#if NDEBUG
  constexpr bool kEnableValidationLayers = false;
#else
  constexpr bool kEnableValidationLayers = true;
#endif
}

class HelloTriangleApplication {
public:
  void Run() {
    InitWindow();
    InitVulkan();
    MainLoop();
    Cleanup();
  }

private:
  void InitWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(kWindowWidth, kWindowHeight, kWindowTitle, nullptr, nullptr);
  }

  void InitVulkan() {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
  }

  void MainLoop() {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();
    }
  }

  void Cleanup() {
    glfwDestroyWindow(window_);
    glfwTerminate();
  }

  void CreateInstance() {
    constexpr vk::ApplicationInfo app_info{
      .pApplicationName   = "Hello, Triangle",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName        = "No Engine",
      .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion         = vk::ApiVersion14,
    };

    auto required_extensions = getRequiredInstanceExtensions();
    for (const auto& extension : required_extensions) {
      std::println("Required extension: {}", std::string{extension});
    }

    auto extension_properties = context_.enumerateInstanceExtensionProperties();
    for (const auto& extension_property : extension_properties) {
      std::println("Supported extension: {}", std::string(extension_property.extensionName));
    }

    auto unsupported_property_it = std::ranges::find_if(required_extensions, [&extension_properties](const auto& required_exension) {
      return std::ranges::none_of(extension_properties, [required_exension](const auto& extension_property) {
        return strcmp(extension_property.extensionName, required_exension) == 0;
      });
    });

    if (unsupported_property_it != required_extensions.end()) {
      throw std::runtime_error("Required extension not supported: " + std::string(*unsupported_property_it));
    }

    std::vector<const char*> required_layers;
    if (kEnableValidationLayers) {
      required_layers.assign(gValidationLayers.begin(), gValidationLayers.end());
    }

    for (const auto& layer : required_layers) {
      std::println("Requird layer: {}", std::string(layer));
    }

    auto layer_properties = context_.enumerateInstanceLayerProperties();
    for (const auto& layer_property : layer_properties) {
      std::println("Supported layer: {}", std::string(layer_property.layerName));
    }

    auto unsupported_layer_it = std::ranges::find_if(required_layers, [&layer_properties](const auto& required_layer) {
      return std::ranges::none_of(layer_properties, [required_layer](const auto& layer_property) {
        return strcmp(layer_property.layerName, required_layer) == 0;
      });
    });

    if (unsupported_layer_it != required_layers.end()) {
      throw std::runtime_error("Required layer not supported: " + std::string(*unsupported_layer_it));
    }


    vk::InstanceCreateInfo create_info{
      .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
      .pApplicationInfo = &app_info,
      .enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
      .ppEnabledExtensionNames = required_extensions.data(),
      .enabledLayerCount = static_cast<uint32_t>(required_layers.size()),
      .ppEnabledLayerNames = required_layers.data(),
    };

    instance_ = vk::raii::Instance(context_, create_info);
  }

  std::vector<const char*> getRequiredInstanceExtensions() {
    uint32_t glfw_extension_count = 0;
    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    // Required for MacOS
    extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);

    if (kEnableValidationLayers) {
      extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    return extensions;
  }

  void SetupDebugMessenger() {
    if (!kEnableValidationLayers) {
      return;
    }

    vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );

    vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(
      vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
    );

    vk::DebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info{
      .messageSeverity = severity_flags,
      .messageType = message_type_flags,
      .pfnUserCallback = &DebugCallback,
    };

    debug_messenger_ = instance_.createDebugUtilsMessengerEXT(debug_utils_messenger_create_info);
  }

  static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
      vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
      vk::DebugUtilsMessageTypeFlagsEXT type,
      const vk::DebugUtilsMessengerCallbackDataEXT* callback_data,
      void* user_Data)
  {
    std::println(stderr, "Validation layer: type {} msg: {}", to_string(type), callback_data->pMessage);
    return vk::False;
  }

  void PickPhysicalDevice() {
    auto devices = instance_.enumeratePhysicalDevices();
    const auto dev_it = std::ranges::find_if(devices, [&](const auto& device) {
      auto queue_families = device.getQueueFamilyProperties();
      bool is_suitable = device.getProperties().apiVersion >= VK_API_VERSION_1_3;
      const auto qfp_iter = std::ranges::find_if(queue_families, [](const vk::QueueFamilyProperties& qfp) {
        return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
      });

      is_suitable = is_suitable && (qfp_iter != queue_families.end());

      auto extensions = device.enumerateDeviceExtensionProperties();
      bool found = true;
      for (const auto& extension : device_extensions_) {
        auto extension_iter = std::ranges::find_if(extensions, [extension](const auto& ext) {
          return strcmp(ext.extensionName, extension) == 0;
        });
        found = found && (extension_iter != extensions.end());
      }

      return is_suitable && found;
    });

    if (dev_it == devices.end()) {
      throw std::runtime_error("Failed to find a suitable GPU");
    }

    physical_device_ = *dev_it;
  }

  std::tuple<uint32_t, uint32_t> FindQueueFamilies() {
    std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device_.getQueueFamilyProperties();

    auto graphics_queue_family_property = std::find_if(
      queue_family_properties.begin(),
      queue_family_properties.end(),
      [](const vk::QueueFamilyProperties& qfp) {
        return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
      }
    );

    auto graphics_index = static_cast<uint32_t>(std::distance(queue_family_properties.begin(), graphics_queue_family_property));

    auto present_index = physical_device_.getSurfaceSupportKHR(graphics_index, *surface_) ? graphics_index : static_cast<uint32_t>(queue_family_properties.size());

    if (present_index == queue_family_properties.size()) {
      for (size_t i = 0; i < queue_family_properties.size(); ++i) {
        if ((queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
            physical_device_.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface_))
        {
          graphics_index = static_cast<uint32_t>(i);
          present_index = graphics_index;
          break;
        }
      }
    }

    if (present_index == queue_family_properties.size()) {
      for (size_t i = 0; i < queue_family_properties.size(); ++i) {
        if (physical_device_.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface_)) {
          graphics_index = static_cast<uint32_t>(i);
          present_index = graphics_index;
          break;
        }
      }
    }

    if (graphics_index == queue_family_properties.size() || present_index == queue_family_properties.size()) {
      throw std::runtime_error("Could not find a queue for graphcis or present -> terminating");
    }

    return { graphics_index, present_index };
  }

  void CreateLogicalDevice() {
    auto [graphics_index, present_index] = FindQueueFamilies();

    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> feature_chain = {
      {},
      { .dynamicRendering = true },
      { .extendedDynamicState = true },
    };


    float queue_priority = 0.5f;
    vk::DeviceQueueCreateInfo device_queue_create_info{
      .queueFamilyIndex = graphics_index,
      .queueCount = 1,
      .pQueuePriorities = &queue_priority,
    };

    vk::DeviceCreateInfo device_create_info{
      .pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &device_queue_create_info,
      .enabledExtensionCount = static_cast<uint32_t>(device_extensions_.size()),
      .ppEnabledExtensionNames = device_extensions_.data(),
    };

    device_ = vk::raii::Device(physical_device_, device_create_info);
    graphics_queue_ = vk::raii::Queue(device_, graphics_index, 0);
    present_queue_ = vk::raii::Queue(device_, present_index, 0);
  }

  void CreateSurface() {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*instance_, window_, nullptr, &surface) != 0) {
      throw std::runtime_error("Failed to create window surface!");
    }
    surface_ = vk::raii::SurfaceKHR(instance_, surface);
  }

  void CreateSwapChain() {
    vk::SurfaceCapabilitiesKHR surface_capabilities = physical_device_.getSurfaceCapabilitiesKHR(surface_);
    std::vector<vk::SurfaceFormatKHR> available_formats = physical_device_.getSurfaceFormatsKHR(surface_);
    std::vector<vk::PresentModeKHR> available_present_modes = physical_device_.getSurfacePresentModesKHR(surface_);

    auto swap_chain_surface_format = ChooseSwapSurfaceFormat(available_formats);
    auto swap_chain_extent = ChooseSwapExtent(surface_capabilities);
    auto min_image_count = std::max(3u, surface_capabilities.minImageCount);
    min_image_count = (surface_capabilities.maxImageCount > 0 && min_image_count > surface_capabilities.maxImageCount)
        ? surface_capabilities.maxImageCount : min_image_count;

    vk::SwapchainCreateInfoKHR swap_chain_create_info{
      .flags = vk::SwapchainCreateFlagBitsKHR(),
      .surface = *surface_,
      .minImageCount = min_image_count,
      .imageFormat = swap_chain_surface_format.format,
      .imageColorSpace = swap_chain_surface_format.colorSpace,
      .imageExtent = swap_chain_extent,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
      .imageSharingMode = vk::SharingMode::eExclusive,
      .preTransform = surface_capabilities.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = ChooseSwapPresentMode(available_present_modes),
      .clipped = true,
      .oldSwapchain = nullptr,
    };

    // auto [graphics_index, present_index] = FindQueueFamilies();
    // uint32_t queue_family_indices[] = {graphics_index, present_index};
    // if (graphics_index != present_index) {
    //   swap_chain_create_info.imageSharingMode = vk::SharingMode::eConcurrent;
    //   swap_chain_create_info.queueFamilyIndexCount = 2;
    //   swap_chain_create_info.pQueueFamilyIndices = queue_family_indices;
    // } else {
    //   swap_chain_create_info.imageSharingMode = vk::SharingMode::eExclusive;
    //   swap_chain_create_info.queueFamilyIndexCount = 0;
    //   swap_chain_create_info.pQueueFamilyIndices = nullptr;
    // }

    swap_chain_ = vk::raii::SwapchainKHR(device_, swap_chain_create_info);
    swap_chain_images_ = swap_chain_.getImages();
    swap_chain_image_format_ = swap_chain_surface_format.format;
    swap_chain_extend_ = swap_chain_extent;
  }

  vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats) {
    for (const auto& available_format : available_formats) {
      if (available_format.format == vk::Format::eB8G8R8Srgb && available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
        return available_format;
      }
    }

    return available_formats[0];
  }

  vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes) {
    for (const auto& available_present_mode : available_present_modes) {
      if (available_present_mode == vk::PresentModeKHR::eMailbox) {
        return available_present_mode;
      }
    }
    return vk::PresentModeKHR::eFifo;
  }

  vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);

    return {
      std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
      std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };
  }

  void CreateImageViews() {
    swap_chain_image_views_.clear();

    vk::ImageViewCreateInfo image_view_create_info{
      .viewType = vk::ImageViewType::e2D,
      .format = swap_chain_image_format_,
      .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
    };

    for (auto image : swap_chain_images_) {
      image_view_create_info.image = image;
      swap_chain_image_views_.emplace_back(device_, image_view_create_info);
    }
  }

private:
  GLFWwindow* window_;
  vk::raii::Context context_;
  vk::raii::Instance instance_ = nullptr;
  vk::raii::DebugUtilsMessengerEXT debug_messenger_ = nullptr;
  vk::raii::PhysicalDevice physical_device_ = nullptr;
  vk::raii::Device device_ = nullptr;
  vk::raii::Queue graphics_queue_ = nullptr;
  vk::raii::SurfaceKHR surface_ = nullptr;
  vk::raii::Queue present_queue_ = nullptr;
  vk::raii::SwapchainKHR swap_chain_ = nullptr;
  vk::Format swap_chain_image_format_;
  vk::Extent2D swap_chain_extend_;
  std::vector<vk::Image> swap_chain_images_;
  std::vector<vk::raii::ImageView> swap_chain_image_views_;

  std::vector<const char*> device_extensions_ = {
    vk::KHRSwapchainExtensionName,
  };
};


auto main(int argc, char* argv[]) -> int {

  try {
    HelloTriangleApplication app;
    app.Run();
  } catch (const std::exception& e) {
    std::println(stderr, "Error: {}", e.what());
    return 1;
  }

  // std::println("Hello, Vulkan");

  // if (!glfwInit()) {
  //   std::println(stderr, "Failed to initialize GLFW");
  //   return 1;
  // }

  // GLFWwindow* window = glfwCreateWindow(kWindowWidth, kWindowHeight, kWindowTitle, nullptr, nullptr);
  // if (!window) {
  //   std::println(stderr, "Failed to create window");
  //   glfwTerminate();
  //   return 1;
  // }

  // glfwMakeContextCurrent(window);

  // while (!glfwWindowShouldClose(window)) {

  //   glfwSwapBuffers(window);
  //   glfwPollEvents();
  // }

  // glfwTerminate();

  // std::println("Terminating...");

  return 0;
}
