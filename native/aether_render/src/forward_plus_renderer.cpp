#include "aether/render/forward_plus_renderer.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <vector>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

namespace aether::render {
namespace {

#if defined(__ANDROID__)
constexpr const char* kLogTag = "Aether3D";
#define AETHER_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, kLogTag, __VA_ARGS__)
#else
#define AETHER_LOGE(...) ((void)0)
#endif

template <typename T> T vk_struct(VkStructureType type) {
    T value{};
    value.sType = type;
    return value;
}

bool has_extension(const std::vector<VkExtensionProperties>& extensions, const char* name) {
    return std::any_of(extensions.begin(), extensions.end(), [name](const VkExtensionProperties& ext) {
        return std::strcmp(ext.extensionName, name) == 0;
    });
}

} // namespace

ForwardPlusRenderer::ForwardPlusRenderer(const ForwardPlusConfig& config) : config_(config) {
    config_.tile_size = std::clamp(config_.tile_size, 8u, 64u);
    config_.max_lights = std::clamp(config_.max_lights, 1u, 4096u);
    config_.frames_in_flight = std::clamp(config_.frames_in_flight, 1u, 3u);
}

ForwardPlusRenderer::~ForwardPlusRenderer() {
    shutdown();
}

bool ForwardPlusRenderer::create_instance() {
    uint32_t count = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS) return false;

    std::vector<VkExtensionProperties> extensions(count);
    if (count > 0 && vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()) != VK_SUCCESS) {
        return false;
    }

    std::vector<const char*> required;
#if defined(__ANDROID__)
    if (!has_extension(extensions, VK_KHR_SURFACE_EXTENSION_NAME) ||
        !has_extension(extensions, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME)) {
        AETHER_LOGE("Android Vulkan surface extensions are unavailable");
        return false;
    }
    required.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    required.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif

    auto app = vk_struct<VkApplicationInfo>(VK_STRUCTURE_TYPE_APPLICATION_INFO);
    app.pApplicationName = "Aether-3D";
    app.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app.pEngineName = "Aether Mobile Renderer";
    app.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app.apiVersion = VK_API_VERSION_1_0;

    auto info = vk_struct<VkInstanceCreateInfo>(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
    info.pApplicationInfo = &app;
    info.enabledExtensionCount = static_cast<uint32_t>(required.size());
    info.ppEnabledExtensionNames = required.empty() ? nullptr : required.data();

#if defined(AETHER_VULKAN_VALIDATION)
    const char* validation = "VK_LAYER_KHRONOS_validation";
    info.enabledLayerCount = 1;
    info.ppEnabledLayerNames = &validation;
#endif

    return vkCreateInstance(&info, nullptr, &instance_) == VK_SUCCESS;
}

bool ForwardPlusRenderer::create_surface() {
#if defined(__ANDROID__)
    auto info = vk_struct<VkAndroidSurfaceCreateInfoKHR>(VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR);
    info.window = window_;
    return vkCreateAndroidSurfaceKHR(instance_, &info, nullptr, &surface_) == VK_SUCCESS;
#else
    return surface_ != VK_NULL_HANDLE;
#endif
}

bool ForwardPlusRenderer::select_physical_device() {
    uint32_t count = 0;
    if (vkEnumeratePhysicalDevices(instance_, &count, nullptr) != VK_SUCCESS || count == 0) return false;

    std::vector<VkPhysicalDevice> devices(count);
    if (vkEnumeratePhysicalDevices(instance_, &count, devices.data()) != VK_SUCCESS) return false;

    for (VkPhysicalDevice candidate : devices) {
        uint32_t queue_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queue_count, nullptr);
        std::vector<VkQueueFamilyProperties> queues(queue_count);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queue_count, queues.data());

        uint32_t graphics = UINT32_MAX;
        uint32_t present = UINT32_MAX;

        for (uint32_t i = 0; i < queue_count; ++i) {
            if ((queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u &&
                (queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0u) {
                graphics = i;
            }

            VkBool32 present_supported = VK_FALSE;
            if (surface_ != VK_NULL_HANDLE) {
                vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface_, &present_supported);
            }
            if (present_supported == VK_TRUE) present = i;
        }

        if (graphics != UINT32_MAX && present != UINT32_MAX) {
            physical_device_ = candidate;
            graphics_family_ = graphics;
            present_family_ = present;
            return true;
        }
    }
    return false;
}

bool ForwardPlusRenderer::create_device() {
    const float priority = 1.0f;
    std::array<VkDeviceQueueCreateInfo, 2> queues{};
    uint32_t queue_count = 1;

    queues[0] = vk_struct<VkDeviceQueueCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
    queues[0].queueFamilyIndex = graphics_family_;
    queues[0].queueCount = 1;
    queues[0].pQueuePriorities = &priority;

    if (present_family_ != graphics_family_) {
        queues[1] = vk_struct<VkDeviceQueueCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
        queues[1].queueFamilyIndex = present_family_;
        queues[1].queueCount = 1;
        queues[1].pQueuePriorities = &priority;
        queue_count = 2;
    }

    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &extension_count, extensions.data());

    if (!has_extension(extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
        AETHER_LOGE("VK_KHR_swapchain is unavailable");
        return false;
    }

    const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkPhysicalDeviceFeatures features{};

    auto info = vk_struct<VkDeviceCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
    info.queueCreateInfoCount = queue_count;
    info.pQueueCreateInfos = queues.data();
    info.enabledExtensionCount = 1;
    info.ppEnabledExtensionNames = device_extensions;
    info.pEnabledFeatures = &features;

    if (vkCreateDevice(physical_device_, &info, nullptr, &device_) != VK_SUCCESS) return false;

    vkGetDeviceQueue(device_, graphics_family_, 0, &graphics_queue_);
    vkGetDeviceQueue(device_, present_family_, 0, &present_queue_);
    return true;
}

bool ForwardPlusRenderer::create_swapchain() {
    VkSurfaceCapabilitiesKHR caps{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &caps) != VK_SUCCESS) return false;

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, nullptr);
    if (format_count == 0) return false;

    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, formats.data());

    VkSurfaceFormatKHR chosen = formats[0];
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_R8G8B8A8_UNORM ||
            format.format == VK_FORMAT_R8G8B8A8_SRGB) {
            chosen = format;
            break;
        }
    }

    uint32_t mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &mode_count, nullptr);
    if (mode_count == 0) return false;

    std::vector<VkPresentModeKHR> modes(mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &mode_count, modes.data());

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (VkPresentModeKHR mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = mode;
            break;
        }
    }

    VkExtent2D extent{};
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent = caps.currentExtent;
    } else {
        extent.width = std::clamp(width_, caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp(height_, caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    uint32_t image_count = caps.minImageCount + 1;
    if (caps.maxImageCount != 0) image_count = std::min(image_count, caps.maxImageCount);

    auto info = vk_struct<VkSwapchainCreateInfoKHR>(VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
    info.surface = surface_;
    info.minImageCount = image_count;
    info.imageFormat = chosen.format;
    info.imageColorSpace = chosen.colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = present_mode;
    info.clipped = VK_TRUE;

    const uint32_t families[] = {graphics_family_, present_family_};
    if (graphics_family_ != present_family_) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices = families;
    } else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    if (vkCreateSwapchainKHR(device_, &info, nullptr, &swapchain_) != VK_SUCCESS) return false;

    uint32_t actual_count = 0;
    vkGetSwapchainImagesKHR(device_, swapchain_, &actual_count, nullptr);
    swapchain_images_.resize(actual_count);
    vkGetSwapchainImagesKHR(device_, swapchain_, &actual_count, swapchain_images_.data());

    tile_count_x_ = (extent.width + config_.tile_size - 1) / config_.tile_size;
    tile_count_y_ = (extent.height + config_.tile_size - 1) / config_.tile_size;
    return true;
}

bool ForwardPlusRenderer::create_command_resources() {
    auto pool = vk_struct<VkCommandPoolCreateInfo>(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool.queueFamilyIndex = graphics_family_;

    if (vkCreateCommandPool(device_, &pool, nullptr, &command_pool_) != VK_SUCCESS) return false;

    command_buffers_.resize(config_.frames_in_flight);
    auto alloc = vk_struct<VkCommandBufferAllocateInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
    alloc.commandPool = command_pool_;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = static_cast<uint32_t>(command_buffers_.size());

    return vkAllocateCommandBuffers(device_, &alloc, command_buffers_.data()) == VK_SUCCESS;
}

bool ForwardPlusRenderer::create_sync_objects() {
    image_available_.resize(config_.frames_in_flight);
    render_finished_.resize(config_.frames_in_flight);
    in_flight_.resize(config_.frames_in_flight);

    auto semaphore = vk_struct<VkSemaphoreCreateInfo>(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
    auto fence = vk_struct<VkFenceCreateInfo>(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < config_.frames_in_flight; ++i) {
        if (vkCreateSemaphore(device_, &semaphore, nullptr, &image_available_[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device_, &semaphore, nullptr, &render_finished_[i]) != VK_SUCCESS ||
            vkCreateFence(device_, &fence, nullptr, &in_flight_[i]) != VK_SUCCESS) {
            return false;
        }
    }
    return true;
}

#if defined(__ANDROID__)
bool ForwardPlusRenderer::initialize(ANativeWindow* window, uint32_t width, uint32_t height) {
    if (window == nullptr || width == 0 || height == 0) return false;
    window_ = window;
#else
bool ForwardPlusRenderer::initialize(VkSurfaceKHR surface, uint32_t width, uint32_t height) {
    if (surface == VK_NULL_HANDLE || width == 0 || height == 0) return false;
    surface_ = surface;
#endif

    width_ = width;
    height_ = height;

    if (!create_instance()) return false;
#if defined(__ANDROID__)
    if (!create_surface()) return false;
#endif
    if (!select_physical_device()) return false;
    if (!create_device()) return false;
    if (!create_swapchain()) return false;
    if (!create_command_resources()) return false;
    if (!create_sync_objects()) return false;

    return true;
}

bool ForwardPlusRenderer::resize(uint32_t width, uint32_t height) {
    if (device_ == VK_NULL_HANDLE || width == 0 || height == 0) return false;
    width_ = width;
    height_ = height;
    vkDeviceWaitIdle(device_);
    destroy_swapchain();
    return create_swapchain();
}

bool ForwardPlusRenderer::begin_frame() {
    if (device_ == VK_NULL_HANDLE || swapchain_ == VK_NULL_HANDLE) return false;

    VkFence fence = in_flight_[frame_index_];
    if (vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) return false;

    VkResult result = vkAcquireNextImageKHR(
        device_, swapchain_, UINT64_MAX,
        image_available_[frame_index_], VK_NULL_HANDLE, &acquired_image_);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) return resize(width_, height_);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) return false;

    if (vkResetFences(device_, 1, &fence) != VK_SUCCESS) return false;

    VkCommandBuffer command_buffer = command_buffers_[frame_index_];
    if (vkResetCommandBuffer(command_buffer, 0) != VK_SUCCESS) return false;

    auto begin = vk_struct<VkCommandBufferBeginInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
    return vkBeginCommandBuffer(command_buffer, &begin) == VK_SUCCESS;
}

void ForwardPlusRenderer::submit_lights(const std::vector<PointLight>& lights) {
    const size_t count = std::min(lights.size(), static_cast<size_t>(config_.max_lights));
    (void)count;
    // The public API intentionally hides descriptor/SSBO details. The next
    // Forward+ pass uploads this bounded array to a GPU storage buffer.
}

bool ForwardPlusRenderer::end_frame() {
    if (device_ == VK_NULL_HANDLE || acquired_image_ == UINT32_MAX) return false;

    VkCommandBuffer command_buffer = command_buffers_[frame_index_];
    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) return false;

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    auto submit = vk_struct<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &image_available_[frame_index_];
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command_buffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &render_finished_[frame_index_];

    if (vkQueueSubmit(graphics_queue_, 1, &submit, in_flight_[frame_index_]) != VK_SUCCESS) return false;

    auto present = vk_struct<VkPresentInfoKHR>(VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &render_finished_[frame_index_];
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain_;
    present.pImageIndices = &acquired_image_;

    const VkResult result = vkQueuePresentKHR(present_queue_, &present);
    frame_index_ = (frame_index_ + 1) % config_.frames_in_flight;
    acquired_image_ = UINT32_MAX;

    return result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR;
}

void ForwardPlusRenderer::destroy_swapchain() {
    if (device_ == VK_NULL_HANDLE) return;

    for (VkImageView view : swapchain_views_) {
        if (view != VK_NULL_HANDLE) vkDestroyImageView(device_, view, nullptr);
    }
    swapchain_views_.clear();

    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
    swapchain_images_.clear();
}

void ForwardPlusRenderer::destroy_device_objects() {
    if (device_ == VK_NULL_HANDLE) return;

    for (VkFence fence : in_flight_) if (fence != VK_NULL_HANDLE) vkDestroyFence(device_, fence, nullptr);
    for (VkSemaphore semaphore : render_finished_) if (semaphore != VK_NULL_HANDLE) vkDestroySemaphore(device_, semaphore, nullptr);
    for (VkSemaphore semaphore : image_available_) if (semaphore != VK_NULL_HANDLE) vkDestroySemaphore(device_, semaphore, nullptr);

    in_flight_.clear();
    render_finished_.clear();
    image_available_.clear();

    if (command_pool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, command_pool_, nullptr);
        command_pool_ = VK_NULL_HANDLE;
    }

    destroy_swapchain();
    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
}

void ForwardPlusRenderer::shutdown() {
    if (instance_ == VK_NULL_HANDLE) return;

    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        destroy_device_objects();
    }

    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }

    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
    physical_device_ = VK_NULL_HANDLE;
    graphics_queue_ = VK_NULL_HANDLE;
    present_queue_ = VK_NULL_HANDLE;
    acquired_image_ = UINT32_MAX;
}

} // namespace aether::render
