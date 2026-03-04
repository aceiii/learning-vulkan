#include <print>
#include <ranges>
#include <algorithm>
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
    PickPhysicalDevice();
    CreateLogicalDevice();
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

  uint32_t FindQueueFamilies(vk::raii::PhysicalDevice physical_device) {
    std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();

    auto graphics_queue_family_property = std::find_if(
      queue_family_properties.begin(),
      queue_family_properties.end(),
      [](const vk::QueueFamilyProperties& qfp) {
        return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
      }
    );

    return static_cast<uint32_t>(std::distance(queue_family_properties.begin(), graphics_queue_family_property));
  }

  void CreateLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device_.getQueueFamilyProperties();
    uint32_t graphics_index = FindQueueFamilies(physical_device_);

    float queue_priority = 0.5f;
    vk::DeviceQueueCreateInfo device_queue_create_info{
      .queueFamilyIndex = graphics_index,
      .queueCount = 1,
      .pQueuePriorities = &queue_priority,
    };

    vk::PhysicalDeviceFeatures device_features;

    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> feature_chain = {
      {},
      { .dynamicRendering = true },
      { .extendedDynamicState = true },
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
  }

private:
  GLFWwindow* window_;
  vk::raii::Context context_;
  vk::raii::Instance instance_ = nullptr;
  vk::raii::DebugUtilsMessengerEXT debug_messenger_ = nullptr;
  vk::raii::PhysicalDevice physical_device_ = nullptr;
  vk::raii::Device device_ = nullptr;
  vk::raii::Queue graphics_queue_ = nullptr;

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
