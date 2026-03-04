#include <print>
#include <ranges>
#include <algorithm>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace {
  constexpr char const* kWindowTitle = "Learning Vulkan";
  constexpr int kWindowWidth = 800;
  constexpr int kWindowHeight = 600;
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

    auto extension_properties = context_.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < glfw_extension_count; ++i) {
      if (std::ranges::none_of(extension_properties, [glfw_extension = glfw_extensions[i]](auto const& extension_property) {
        return strcmp(extension_property.extensionName, glfw_extension) == 0;
      })) {
        throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfw_extensions[i]));
      }
    }

    vk::InstanceCreateInfo create_info{
      .pApplicationInfo = &app_info,
      .enabledExtensionCount = glfw_extension_count,
      .ppEnabledExtensionNames = glfw_extensions,
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
