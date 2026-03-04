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

    uint32_t glfw_extension_count = 0;
    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> extensions{&glfw_extensions[0], &glfw_extensions[glfw_extension_count]};
    extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
    for (const auto& extension : extensions) {
      std::println("Required extension: {}", std::string{extension});
    }

    auto extension_properties = context_.enumerateInstanceExtensionProperties();
    for (const auto& extension_property : extension_properties) {
      std::println("Supported extension: {}", std::string(extension_property.extensionName));
    }

    for (const auto& extension : extensions) {
      if (std::ranges::none_of(extension_properties, [extension](auto const& extension_property) {
        return strcmp(extension_property.extensionName, extension) == 0;
      })) {
        throw std::runtime_error("Required extension not supported: " + std::string(extension));
      }
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
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data(),
    };

    instance_ = vk::raii::Instance(context_, create_info);
  }

private:
  GLFWwindow* window_;
  vk::raii::Context context_;
  vk::raii::Instance instance_ = nullptr;
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
