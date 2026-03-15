#include <cstdint>
#include <algorithm>
#include <fstream>
#include <limits>
#include <print>
#include <string>
#include <string_view>
#include <ranges>
#include <tuple>
#include <vector>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>


namespace {
  constexpr char const* kWindowTitle = "Learning Vulkan";
  constexpr int kWindowWidth = 800;
  constexpr int kWindowHeight = 600;
  constexpr int kMaxFramesInFlight = 2;

  const std::vector<const char*> gValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };

  const std::vector<const char*> gDeviceExensions = {
    vk::KHRSwapchainExtensionName,
  };

#if NDEBUG
  constexpr bool kEnableValidationLayers = false;
#else
  constexpr bool kEnableValidationLayers = true;
#endif
}

static std::vector<char> ReadFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + std::string(strerror(errno)));
  }

  std::vector<char> buffer(file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
  file.close();

  return buffer;
}


struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static vk::VertexInputBindingDescription GetBindingDescription() {
    return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
  }

  static std::array<vk::VertexInputAttributeDescription, 2> GetAttributeDescriptions() {
    return {
      vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
      vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
    };
  }
};

namespace {
  const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
  };
};

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
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(kWindowWidth, kWindowHeight, kWindowTitle, nullptr, nullptr);

    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, FramebufferResizeCallback);
  }

  static void FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->framebuffer_resized_ = true;
  }

  void InitVulkan() {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    CreateGraphicsPipeline();
    CreateCommandPool();
    CreateVertexBuffer();
    CreateCommandBuffers();
    CreateSyncObjects();
  }

  void MainLoop() {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();
      DrawFrame();
    }
    device_.waitIdle();
  }

  void Cleanup() {
    CleanupSwapChain();
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
      for (const auto& extension : gDeviceExensions) {
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
      .enabledExtensionCount = static_cast<uint32_t>(gDeviceExensions.size()),
      .ppEnabledExtensionNames = gDeviceExensions.data(),
    };

    device_ = vk::raii::Device(physical_device_, device_create_info);
    graphics_queue_ = vk::raii::Queue(device_, graphics_index, 0);
    present_queue_ = vk::raii::Queue(device_, present_index, 0);
    graphics_index_ = graphics_index;
    present_index_ = present_index;
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
    swap_chain_extent_ = swap_chain_extent;
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

  void CreateGraphicsPipeline() {
    vk::raii::ShaderModule shader_module = CreateShaderModule(ReadFile("shaders/slang.spv"));

    vk::PipelineShaderStageCreateInfo vert_shader_stage_info{
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = shader_module,
      .pName = "vertMain",
    };

    vk::PipelineShaderStageCreateInfo frag_shader_stage_info{
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = shader_module,
      .pName = "fragMain",
    };

    vk::PipelineShaderStageCreateInfo shader_stages[] = {
      vert_shader_stage_info,
      frag_shader_stage_info,
    };

    std::vector dynamic_states = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state{
      .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
      .pDynamicStates = dynamic_states.data(),
    };

    auto binding_description = Vertex::GetBindingDescription();
    auto attribute_descriptions = Vertex::GetAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertex_input_info{
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &binding_description,
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size()),
      .pVertexAttributeDescriptions = attribute_descriptions.data(),
    };

    vk::PipelineInputAssemblyStateCreateInfo input_assembly{
      .topology = vk::PrimitiveTopology::eTriangleList,
    };

    vk::Viewport viewport{0.0f, 0.0f, static_cast<float>(swap_chain_extent_.width), static_cast<float>(swap_chain_extent_.height), 0.0f, 1.0f};

    vk::Rect2D rect{vk::Offset2D{0, 0}, swap_chain_extent_ };

    vk::PipelineViewportStateCreateInfo viewport_state{
      .viewportCount = 1,
      .scissorCount = 1,
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
      .depthClampEnable = vk::False,
      .rasterizerDiscardEnable = vk::False,
      .polygonMode = vk::PolygonMode::eFill,
      .cullMode = vk::CullModeFlagBits::eBack,
      .frontFace = vk::FrontFace::eClockwise,
      .depthBiasClamp = vk::False,
      .depthBiasSlopeFactor = 1.0f,
      .lineWidth = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
      .sampleShadingEnable = vk::False,
    };

    vk::PipelineColorBlendAttachmentState color_blend_attachment{
      .blendEnable = vk::False,
      .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineColorBlendStateCreateInfo color_blending{
      .logicOpEnable = vk::False,
      .logicOp = vk::LogicOp::eCopy,
      .attachmentCount = 1,
      .pAttachments = &color_blend_attachment,
    };

    vk::PipelineLayoutCreateInfo pipeline_layout_info{
      .setLayoutCount = 0,
      .pushConstantRangeCount = 0,
    };

    pipepline_layout_ = vk::raii::PipelineLayout(device_, pipeline_layout_info);

    vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &swap_chain_image_format_,
    };

    vk::GraphicsPipelineCreateInfo pipeline_info{
      .pNext = &pipeline_rendering_create_info,
      .stageCount = 2,
      .pStages = shader_stages,
      .pVertexInputState = &vertex_input_info,
      .pInputAssemblyState = &input_assembly,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pColorBlendState = &color_blending,
      .pDynamicState = &dynamic_state,
      .layout = pipepline_layout_,
      .renderPass = nullptr,
    };

    graphics_pipeline_ = vk::raii::Pipeline(device_, nullptr, pipeline_info);
  }

  [[nodiscard]] vk::raii::ShaderModule CreateShaderModule(const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo create_info{
      .codeSize = code.size() * sizeof(char),
      .pCode = reinterpret_cast<const uint32_t*>(code.data()),
    };

    vk::raii::ShaderModule shader_module{device_, create_info};

    return shader_module;
  }

  void CreateCommandPool() {
    vk::CommandPoolCreateInfo pool_info{
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = graphics_index_,
    };

    command_pool_ = vk::raii::CommandPool(device_, pool_info);
  }

  void CreateCommandBuffers() {
    vk::CommandBufferAllocateInfo alloc_info{
      .commandPool = command_pool_,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = kMaxFramesInFlight,
    };

    command_buffers_ = vk::raii::CommandBuffers(device_, alloc_info);
  }

  void RecordCommandBuffer(uint32_t image_index) {
    auto& command_buffer = command_buffers_[frame_index_];

    command_buffer.begin({});

    TransitionImageLayout(
      image_index,
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::eColorAttachmentOptimal,
      {},
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );

    vk::ClearValue clear_color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::RenderingAttachmentInfo attachment_info{
      .imageView = swap_chain_image_views_[image_index],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = clear_color,
    };

    vk::RenderingInfo rendering_info{
      .renderArea = { .offset = {0, 0}, .extent = swap_chain_extent_ },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &attachment_info,
    };

    command_buffer.beginRendering(rendering_info);
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphics_pipeline_);
    command_buffer.bindVertexBuffers(0, *vertex_buffer_, {0});
    command_buffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swap_chain_extent_.width), static_cast<float>(swap_chain_extent_.height), 0.0f, 1.0f));
    command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swap_chain_extent_));
    command_buffer.draw(3, 1, 0, 0);
    command_buffer.endRendering();

    TransitionImageLayout(
      image_index,
      vk::ImageLayout::eColorAttachmentOptimal,
      vk::ImageLayout::ePresentSrcKHR,
      vk::AccessFlagBits2::eColorAttachmentWrite,
      {},
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    command_buffer.end();
  }

  void TransitionImageLayout(
    uint32_t image_index,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask
  ) {
    vk::ImageMemoryBarrier2 barrier{
      .srcStageMask = src_stage_mask,
      .srcAccessMask = src_access_mask,
      .dstStageMask = dst_stage_mask,
      .dstAccessMask = dst_access_mask,
      .oldLayout = old_layout,
      .newLayout = new_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = swap_chain_images_[image_index],
      .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
    };

    vk::DependencyInfo dependency_info{
      .dependencyFlags = {},
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &barrier,
    };

    command_buffers_[frame_index_].pipelineBarrier2(dependency_info);
  }

  void CreateSyncObjects() {
    assert(present_complete_semaphores_.empty() && render_finished_semaphores_.empty() && in_flight_fences_.empty());

    for (size_t i = 0; i < swap_chain_images_.size(); ++i) {
      render_finished_semaphores_.emplace_back(device_, vk::SemaphoreCreateInfo{});
    }

    for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
      present_complete_semaphores_.emplace_back(device_, vk::SemaphoreCreateInfo{});
      in_flight_fences_.emplace_back(device_, vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
    }
  }

  void DrawFrame() {
    auto fence_result = device_.waitForFences(*in_flight_fences_[frame_index_], vk::True, UINT64_MAX);
    if (fence_result != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to wait for fence!");
    }

    auto [result, image_index] = swap_chain_.acquireNextImage(UINT64_MAX, *present_complete_semaphores_[frame_index_], nullptr);
    if (result == vk::Result::eErrorOutOfDateKHR) {
      RecreateSwapChain();
      return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
      assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
      throw std::runtime_error("Failed to acquire swap chain image!");
    }

    device_.resetFences(*in_flight_fences_[frame_index_]);

    command_buffers_[frame_index_].reset();
    RecordCommandBuffer(image_index);

    vk::PipelineStageFlags wait_destination_stage_mask(vk::PipelineStageFlagBits::eColorAttachmentOutput);

    const vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &*present_complete_semaphores_[frame_index_],
      .pWaitDstStageMask = &wait_destination_stage_mask,
      .commandBufferCount = 1,
      .pCommandBuffers = &*command_buffers_[frame_index_],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &*render_finished_semaphores_[frame_index_],
    };

    graphics_queue_.submit(submit_info, *in_flight_fences_[frame_index_]);

    const vk::PresentInfoKHR present_info{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &*render_finished_semaphores_[frame_index_],
      .swapchainCount = 1,
      .pSwapchains = &*swap_chain_,
      .pImageIndices = &image_index,
    };

    result = present_queue_.presentKHR(present_info);
    if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR || framebuffer_resized_) {
      framebuffer_resized_ = false;
      RecreateSwapChain();
    } else {
      assert(result == vk::Result::eSuccess);
    }

    frame_index_ = (frame_index_ + 1) % kMaxFramesInFlight;
  }

  void CleanupSwapChain() {
    swap_chain_images_.clear();
    swap_chain_ = nullptr;
  }

  void RecreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(window_, &width, &height);
      glfwWaitEvents();
    }

    device_.waitIdle();
    CleanupSwapChain();
    CreateSwapChain();
    CreateImageViews();
  }

  void CreateVertexBuffer() {
    vk::BufferCreateInfo buffer_info{
      .size = sizeof(vertices[0]) * vertices.size(),
      .usage = vk::BufferUsageFlagBits::eVertexBuffer,
      .sharingMode = vk::SharingMode::eExclusive,
    };
    vertex_buffer_ = vk::raii::Buffer(device_, buffer_info);

    vk::MemoryRequirements mem_requirements = vertex_buffer_.getMemoryRequirements();

    vk::MemoryAllocateInfo memory_allocate_info{
      .allocationSize = mem_requirements.size,
      .memoryTypeIndex = findMemoryType(mem_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent),
    };

    vertex_buffer_memory_ = vk::raii::DeviceMemory(device_, memory_allocate_info);
    vertex_buffer_.bindMemory(*vertex_buffer_memory_, 0);

    void* data = vertex_buffer_memory_.mapMemory(0, buffer_info.size);
    memcpy(data, vertices.data(), buffer_info.size);
    vertex_buffer_memory_.unmapMemory();
  }

  uint32_t findMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties mem_properties = physical_device_.getMemoryProperties();
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
      if (type_filter & (1  << i)) {
        return 1;
      }
    }
    throw std::runtime_error("Failed to find a suitable memory type");
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
  vk::Extent2D swap_chain_extent_;
  std::vector<vk::Image> swap_chain_images_;
  std::vector<vk::raii::ImageView> swap_chain_image_views_;
  vk::raii::PipelineLayout pipepline_layout_ = nullptr;
  vk::raii::Pipeline graphics_pipeline_ = nullptr;
  vk::raii::CommandPool command_pool_ = nullptr;
  vk::raii::Buffer vertex_buffer_ = nullptr;
  vk::raii::DeviceMemory vertex_buffer_memory_ = nullptr;

  std::vector<vk::raii::CommandBuffer> command_buffers_;
  std::vector<vk::raii::Semaphore> present_complete_semaphores_;
  std::vector<vk::raii::Semaphore> render_finished_semaphores_;
  std::vector<vk::raii::Fence> in_flight_fences_;

  uint32_t graphics_index_;
  uint32_t present_index_;
  uint32_t frame_index_ = 0;

  bool framebuffer_resized_ = false;
};


auto main(int argc, char* argv[]) -> int {

  try {
    HelloTriangleApplication app;
    app.Run();
  } catch (const std::exception& e) {
    std::println(stderr, "Error: {}", e.what());
    return 1;
  }

  return 0;
}
