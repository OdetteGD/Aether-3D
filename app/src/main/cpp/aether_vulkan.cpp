#include "aether_vulkan.h"
#include <android/native_window.h>
#include <android/log.h>
#include <array>
#include <vector>
#include <algorithm>
#include <cstring>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"Aether3D",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"Aether3D",__VA_ARGS__)

namespace aether {
VulkanRenderer::VulkanRenderer(){ init(); }
VulkanRenderer::~VulkanRenderer(){ shutdown(); }

bool VulkanRenderer::create_instance(){
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName="Aether 3D"; app.applicationVersion=VK_MAKE_VERSION(0,1,0);
    app.pEngineName="Aether Engine"; app.engineVersion=VK_MAKE_VERSION(0,1,0); app.apiVersion=VK_API_VERSION_1_1;
    const char* exts[]={"VK_KHR_surface","VK_KHR_android_surface"};
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo=&app; ci.enabledExtensionCount=2; ci.ppEnabledExtensionNames=exts;
    return vkCreateInstance(&ci,nullptr,&instance_)==VK_SUCCESS;
}
bool VulkanRenderer::create_device(){
    uint32_t n=0; vkEnumeratePhysicalDevices(instance_,&n,nullptr); if(!n)return false;
    std::vector<VkPhysicalDevice> ds(n); vkEnumeratePhysicalDevices(instance_,&n,ds.data());
    for(auto d:ds){
        uint32_t qn=0; vkGetPhysicalDeviceQueueFamilyProperties(d,&qn,nullptr);
        std::vector<VkQueueFamilyProperties> q(qn); vkGetPhysicalDeviceQueueFamilyProperties(d,&qn,q.data());
        for(uint32_t i=0;i<qn;i++){
            VkBool32 present=VK_FALSE;
            if(surface_) vkGetPhysicalDeviceSurfaceSupportKHR(d,i,surface_,&present);
            if((q[i].queueFlags&VK_QUEUE_GRAPHICS_BIT) && present){ gpu_=d; queue_family_=i;
                float pr=1.f; VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
                qi.queueFamilyIndex=i; qi.queueCount=1; qi.pQueuePriorities=&pr;
                const char* exts[]={"VK_KHR_swapchain"};
                VkPhysicalDeviceFeatures f{}; VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
                di.queueCreateInfoCount=1; di.pQueueCreateInfos=&qi; di.pEnabledFeatures=&f;
                di.enabledExtensionCount=1; di.ppEnabledExtensionNames=exts;
                if(vkCreateDevice(gpu_,&di,nullptr,&device_)!=VK_SUCCESS)return false;
                vkGetDeviceQueue(device_,i,0,&graphics_queue_); return true;
            }
        }
    } return false;
}
bool VulkanRenderer::create_surface(){
    if(!window_) return false;
    VkAndroidSurfaceCreateInfoKHR si{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
    si.window=window_; return vkCreateAndroidSurfaceKHR(instance_,&si,nullptr,&surface_)==VK_SUCCESS;
}
bool VulkanRenderer::create_swapchain(){
    VkSurfaceCapabilitiesKHR caps{}; if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu_,surface_,&caps)!=VK_SUCCESS)return false;
    uint32_t n=0; vkGetPhysicalDeviceSurfaceFormatsKHR(gpu_,surface_,&n,nullptr); if(!n)return false;
    std::vector<VkSurfaceFormatKHR> fs(n); vkGetPhysicalDeviceSurfaceFormatsKHR(gpu_,surface_,&n,fs.data());
    VkSurfaceFormatKHR fmt=fs[0]; for(auto f:fs) if(f.format==VK_FORMAT_R8G8B8A8_UNORM) {fmt=f;break;}
    VkExtent2D extent=caps.currentExtent; if(extent.width==UINT32_MAX){extent.width=std::max(caps.minImageExtent.width,std::min(width_,caps.maxImageExtent.width)); extent.height=std::max(caps.minImageExtent.height,std::min(height_,caps.maxImageExtent.height));}
    width_=extent.width; height_=extent.height;
    uint32_t count=std::max(2u,caps.minImageCount); if(caps.maxImageCount) count=std::min(count,caps.maxImageCount);
    VkSwapchainCreateInfoKHR ci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    ci.surface=surface_; ci.minImageCount=count; ci.imageFormat=fmt.format; ci.imageColorSpace=fmt.colorSpace;
    ci.imageExtent=extent; ci.imageArrayLayers=1; ci.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ci.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE; ci.preTransform=caps.currentTransform; ci.compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode=VK_PRESENT_MODE_FIFO_KHR; ci.clipped=VK_TRUE;
    return vkCreateSwapchainKHR(device_,&ci,nullptr,&swapchain_)==VK_SUCCESS;
}
bool VulkanRenderer::create_forward_plus_resources(){
    // Forward+ contract: the renderer owns GPU light/cluster resources and can add
    // a compute culling pass without changing the swapchain or scene API.
    // Actual cluster dispatch/pipeline is enabled in the next rendering milestone.
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pi.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pi.queueFamilyIndex=queue_family_;
    if(vkCreateCommandPool(device_,&pi,nullptr,&command_pool_)!=VK_SUCCESS)return false;
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool=command_pool_; ai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY; ai.commandBufferCount=1;
    if(vkAllocateCommandBuffers(device_,&ai,&command_buffer_)!=VK_SUCCESS)return false;
    VkSemaphoreCreateInfo si{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    if(vkCreateSemaphore(device_,&si,nullptr,&image_available_)!=VK_SUCCESS)return false;
    if(vkCreateSemaphore(device_,&si,nullptr,&render_finished_)!=VK_SUCCESS)return false;
    VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO}; fi.flags=VK_FENCE_CREATE_SIGNALED_BIT;
    return vkCreateFence(device_,&fi,nullptr,&in_flight_)==VK_SUCCESS;
}
bool VulkanRenderer::init(){
    if(!create_instance()) return false;
    initialized_=true; return true;
}
void VulkanRenderer::set_window(ANativeWindow* w){
    if(w==window_ && swapchain_) return;
    if(!w){ if(device_)vkDeviceWaitIdle(device_); destroy_swapchain(); window_=nullptr; return; }
    if(device_)vkDeviceWaitIdle(device_);
    destroy_swapchain(); window_=w;
    if(!surface_ && !create_surface()){LOGE("Vulkan surface creation failed");return;}
    if(!device_ && !create_device()){LOGE("Vulkan device creation failed");return;}
    if(!swapchain_ && (!create_swapchain() || !create_forward_plus_resources())){LOGE("Vulkan swapchain setup failed");return;}
    dirty_=true; LOGI("Aether Vulkan renderer ready (%ux%u)",width_,height_);
}
void VulkanRenderer::resize(uint32_t w,uint32_t h){ width_=std::max(1u,w);height_=std::max(1u,h);dirty_=true; }
void VulkanRenderer::destroy_swapchain(){
    if(!device_)return;
    if(in_flight_)vkDestroyFence(device_,in_flight_,nullptr); in_flight_=VK_NULL_HANDLE;
    if(image_available_)vkDestroySemaphore(device_,image_available_,nullptr); image_available_=VK_NULL_HANDLE;
    if(render_finished_)vkDestroySemaphore(device_,render_finished_,nullptr); render_finished_=VK_NULL_HANDLE;
    if(command_pool_)vkDestroyCommandPool(device_,command_pool_,nullptr); command_pool_=VK_NULL_HANDLE; command_buffer_=VK_NULL_HANDLE;
    if(swapchain_)vkDestroySwapchainKHR(device_,swapchain_,nullptr); swapchain_=VK_NULL_HANDLE;
}
void VulkanRenderer::shutdown(){
    if(instance_==VK_NULL_HANDLE)return;
    if(device_)vkDeviceWaitIdle(device_);
    destroy_swapchain();
    if(device_)vkDestroyDevice(device_,nullptr);
    if(surface_)vkDestroySurfaceKHR(instance_,surface_,nullptr);
    if(instance_)vkDestroyInstance(instance_,nullptr);
    device_=VK_NULL_HANDLE; surface_=VK_NULL_HANDLE; instance_=VK_NULL_HANDLE; initialized_=false;
}
void VulkanRenderer::touch(float x,float y,int action){ (void)x;(void)y;(void)action; }
void VulkanRenderer::draw(){
    if(!device_||!swapchain_||!command_buffer_)return;
    if(vkWaitForFences(device_,1,&in_flight_,VK_TRUE,1000000)!=VK_SUCCESS)return;
    uint32_t index=0; VkResult ar=vkAcquireNextImageKHR(device_,swapchain_,1000000,image_available_,VK_NULL_HANDLE,&index);
    if(ar==VK_ERROR_OUT_OF_DATE_KHR||ar==VK_SUBOPTIMAL_KHR){ dirty_=true; return; } if(ar!=VK_SUCCESS)return;
    vkResetFences(device_,1,&in_flight_); vkResetCommandBuffer(command_buffer_,0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO}; vkBeginCommandBuffer(command_buffer_,&bi);
    VkImageMemoryBarrier toClear{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    toClear.srcAccessMask=0; toClear.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT; toClear.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED; toClear.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toClear.image=VK_NULL_HANDLE; // swapchain image is acquired below through a local query.
    uint32_t ic=0; vkGetSwapchainImagesKHR(device_,swapchain_,&ic,nullptr); std::vector<VkImage> images(ic); vkGetSwapchainImagesKHR(device_,swapchain_,&ic,images.data()); toClear.image=images[index];
    toClear.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    vkCmdPipelineBarrier(command_buffer_,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&toClear);
    VkClearColorValue clear{{0.025f,0.035f,0.055f,1.f}}; VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    vkCmdClearColorImage(command_buffer_,images[index],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&clear,1,&range);
    std::swap(toClear.oldLayout,toClear.newLayout); toClear.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT; toClear.dstAccessMask=0;
    vkCmdPipelineBarrier(command_buffer_,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,nullptr,0,nullptr,1,&toClear);
    vkEndCommandBuffer(command_buffer_);
    VkPipelineStageFlags wait=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO}; si.waitSemaphoreCount=1;si.pWaitSemaphores=&image_available_;si.pWaitDstStageMask=&wait;si.commandBufferCount=1;si.pCommandBuffers=&command_buffer_;si.signalSemaphoreCount=1;si.pSignalSemaphores=&render_finished_;
    if(vkQueueSubmit(graphics_queue_,1,&si,in_flight_)!=VK_SUCCESS)return;
    VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};pi.waitSemaphoreCount=1;pi.pWaitSemaphores=&render_finished_;pi.swapchainCount=1;pi.pSwapchains=&swapchain_;pi.pImageIndices=&index;vkQueuePresentKHR(graphics_queue_,&pi);
}
}
