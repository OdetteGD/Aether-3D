#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
struct ANativeWindow;
namespace aether {
class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();
    void set_window(ANativeWindow* window);
    void resize(uint32_t w,uint32_t h);
    void touch(float x,float y,int action);
    void draw();
private:
    bool init();
    void shutdown();
    bool create_instance();
    bool create_device();
    bool create_surface();
    bool create_swapchain();
    bool create_forward_plus_resources();
    void destroy_swapchain();
    ANativeWindow* window_{};
    VkInstance instance_{VK_NULL_HANDLE};
    VkPhysicalDevice gpu_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphics_queue_{VK_NULL_HANDLE};
    uint32_t queue_family_{0};
    VkSurfaceKHR surface_{VK_NULL_HANDLE};
    VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
    VkCommandPool command_pool_{VK_NULL_HANDLE};
    VkCommandBuffer command_buffer_{VK_NULL_HANDLE};
    VkSemaphore image_available_{VK_NULL_HANDLE};
    VkSemaphore render_finished_{VK_NULL_HANDLE};
    VkFence in_flight_{VK_NULL_HANDLE};
    VkPipelineLayout forward_layout_{VK_NULL_HANDLE};
    VkPipeline forward_pipeline_{VK_NULL_HANDLE};
    uint32_t width_{1},height_{1};
    bool initialized_{false};
    bool dirty_{true};
};
}
