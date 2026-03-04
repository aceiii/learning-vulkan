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

private:
  GLFWwindow* window_;
  vk::raii::Context context_;
  vk::raii::Instance instance_ = nullptr;
  vk::raii::DebugUtilsMessengerEXT debug_messenger_ = nullptr;
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
