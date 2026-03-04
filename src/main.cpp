#include <print>
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

  GLFWwindow* window_;
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
