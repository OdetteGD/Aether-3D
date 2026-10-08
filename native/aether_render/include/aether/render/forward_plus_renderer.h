#pragma once

#include <cstdint>
#include <vector>

#if defined(__ANDROID__)
#include <android/native_window.h>
#endif

#include <vulkan/vulkan.h>

namespace aether::render {

struct PointLight {
    float position[3]{0.0f, 0.0f, 0.0f};
    float radius{1.0f};
    float color[3]{1.0f, 1.0f, 1.0f};
    float intensity{1.0f};
};

struct ForwardPlusConfig {
    uint32_t tile_size = 16;
    uint32_t max_lights = 1024;
    uint32_t frames_in_flight = 2;
    bool prefer_subgroup_operations = true;
};

class ForwardPlusRenderer final {
public:
    explicit ForwardPlusRenderer(const ForwardPlusConfig& config = {});
    ~ForwardPlusRenderer();

    ForwardPlusRenderer(const ForwardPlusRenderer&) = delete;
    ForwardPlusRenderer& operator=(const ForwardPlusRenderer&) = delete;

#if defined(__ANDROID__)
    bool initialize(ANativeWindow* window, uint32_t width, uint32_t height);
#else
    bool initialize(VkSurfaceKHR surface, uint32_t width, uint32_t height);
#endif

    void shutdown();
    bool resize(uint32_t width, uint32_t height);
    bool begin_frame();
    void submit_lights(const std::vector<PointLight>& lights);
    bool end_frame();

    VkInstance instance() const { return instance_; }
    VkDevice device() const { return device_; }
    VkQueue graphics_queue() const { return graphics_queue_; }

    uint32_t tile_count_x() const { return tile_count_x_; }
    uint32_t tile_count_y() const { return tile_count_y_; }

private:
    bool create_instance();
    bool create_surface();
    bool select_physical_device();
    bool create_device();
    bool create_swapchain();
    bool create_sync_objects();
    bool create_command_resources();

    void destroy_swapchain();
    void destroy_device_objects();

    ForwardPlusConfig config_{};

    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphics_queue_ = VK_NULL_HANDLE;
    VkQueue present_queue_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;

    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_views_;
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers_;

    std::vector<VkSemaphore> image_available_;
    std::vector<VkSemaphore> render_finished_;
    std::vector<VkFence> in_flight_;

    uint32_t graphics_family_ = UINT32_MAX;
    uint32_t present_family_ = UINT32_MAX;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t tile_count_x_ = 0;
    uint32_t tile_count_y_ = 0;
    uint32_t frame_index_ = 0;
    uint32_t acquired_image_ = UINT32_MAX;

#if defined(__ANDROID__)
    ANativeWindow* window_ = nullptr;
#endif
};

} // namespace aether::render
